#include "SurveyReview.h"
#include "grain.h"
#include "skycoverage.h"

#include <orsa/debug.h>
#include <orsa/print.h>
#include <orsa/statistic.h>

#include <orsaSolarSystem/datetime.h>
#include <orsaSolarSystem/observatory.h>

#include <orsaUtil/observatory.h>

#include <orsaSPICE/spice.h>
#include <orsaSPICE/spiceBodyRotationalCallback.h>
#include <orsaSPICE/spiceBodyTranslationalCallback.h>

// CSPICE prototypes and definitions.      
#include <SpiceUsr.h>

// BOINC API
#include <boinc_api.h>
#include <filesys.h>

// SQLite3
#include "sqlite3.h"

int main() {
    
    orsa::Debug::instance()->initTimer();
    
    boinc_init();
    
    ORSA_DEBUG("process ID: %i",getpid());
    
    /* 
       ORSA_DEBUG("NEO_max_q: %g = %g [AU]",
       OrbitID::NEO_max_q,
       orsa::FromUnits(OrbitID::NEO_max_q,orsa::Unit::AU,-1));
    */
    
    std::string resolvedFileName;
  
    // needed to work with SQLite database
    sqlite3     * db;
    char        * zErr;
    int           rc;
    std::string   sql;
  
    {
        // open database
        boinc_resolve_filename_s("results.db",resolvedFileName);
        rc = sqlite3_open(resolvedFileName.c_str(),&db);
        //
        if (rc) {
            fprintf(stderr,"Can't open db: %s\n",sqlite3_errmsg(db));
            sqlite3_close(db);
            boinc_finish(0); 
        }
    }
  
    {
        // get list of tables, to see if some results have been obtained already
        // if no table is present, create it
        char **result;
        int nrows, ncols;
        sql = "SELECT name FROM sqlite_master";
        rc = sqlite3_get_table(db,sql.c_str(),&result,&nrows,&ncols,&zErr);
        //
        if (rc != SQLITE_OK) {
            if (zErr != NULL) {
                fprintf(stderr,"SQL error: %s\n",zErr);
                sqlite3_free(zErr);
                boinc_finish(0); 
            }
        }
        // ORSA_DEBUG("nrows: %i  ncols: %i",nrows, ncols);
        //
        /* for (int i=0; i<nrows; ++i) {
           for (int j=0; j<ncols; ++j) {
           // i=0 is the header
           const int index = (i+1)*ncols+j;
           ORSA_DEBUG("result[%i] = %s",index, result[index]);
           }
           }
        */
        //
        bool createTable=true;
        //
        if (nrows==2) {
            createTable=false;
        }
        //
        sqlite3_free_table(result);
    
        if (createTable) {
            // create results table
            sql = "CREATE TABLE grid(z_a_min INTEGER, z_a_max INTEGER, z_e_min INTEGER, z_e_max INTEGER, z_i_min INTEGER, z_i_max INTEGER, z_node_min INTEGER, z_node_max INTEGER, z_peri_min INTEGER, z_peri_max INTEGER, z_M_min INTEGER, z_M_max INTEGER, z_H_min INTEGER, z_H_max INTEGER, eta_NEO REAL, sigma_eta_NEO REAL, eta_PHO REAL, sigma_eta_PHO REAL)";
            rc = sqlite3_exec(db,sql.c_str(),NULL,NULL,&zErr);
            //
            if (rc != SQLITE_OK) {
                if (zErr != NULL) {
                    fprintf(stderr,"SQL error: %s\n",zErr);
                    sqlite3_free(zErr);
                    boinc_finish(0); 
                }
            }
      
            // create random number state table
            sql = "CREATE TABLE rng(binary_state BLOB)";
            rc = sqlite3_exec(db,sql.c_str(),NULL,NULL,&zErr);
            //
            if (rc != SQLITE_OK) {
                if (zErr != NULL) {
                    fprintf(stderr,"SQL error: %s\n",zErr);
                    sqlite3_free(zErr);
                    boinc_finish(0); 
                }
            }
        }
    }
  
    osg::ref_ptr<orsaInputOutput::MPCObsCodeFile> obsCodeFile = new orsaInputOutput::MPCObsCodeFile;
    boinc_resolve_filename_s("obscode.dat",resolvedFileName);
    obsCodeFile->setFileName(resolvedFileName);
    obsCodeFile->read();
  
    osg::ref_ptr<orsaUtil::StandardObservatoryPositionCallback> obsPosCB =
        new orsaUtil::StandardObservatoryPositionCallback(obsCodeFile.get());
  
    {
        // spice error file (should resolve filename?)
        SpiceChar spiceError[1024];
        sprintf(spiceError,"cspice.error");
        ::errdev_c("SET", 4096, spiceError);
    
        orsaSPICE::SPICE::instance()->setDefaultObserver("SSB");
    
        boinc_resolve_filename_s("de405.bsp",resolvedFileName);
        // FILE * fp_dummy = boinc_fopen(resolvedFileName.c_str(),"r"); // little trick, is it helping?
        orsaSPICE::SPICE::instance()->loadKernel(resolvedFileName);
        // fclose(fp_dummy);
    }

    osg::ref_ptr<SkyCoverageFile> skyCoverageFile = new SkyCoverageFile;
    boinc_resolve_filename_s("field.dat",resolvedFileName);
    skyCoverageFile->setFileName(resolvedFileName);
    skyCoverageFile->read();
    
    osg::ref_ptr<SkyCoverage> skyCoverage = skyCoverageFile->_data;

    {
        // read fit.dat file
        // global, bad but straightforward
        double JD, year;
        double V_limit, eta0_V, c_V, w_V;
        double U_limit, w_U;
        /* double beta;
           double GB_limit, w_GB;
           double Gmix;
        */
        double chisq_dof;
        unsigned int Nobs, Ndsc, Ntot;
        double degSq;
        double V0;
        char jobID[1024];
        
        // read fit.dat file
        
        boinc_resolve_filename_s("fit.dat",resolvedFileName);
        FILE * fp = boinc_fopen(resolvedFileName.c_str(),"r"); 
        if (!fp) {
            ORSA_DEBUG("cannot open file [%s]",resolvedFileName.c_str());
            boinc_finish(0); 
        }
        
        char line[1024];
        while (fgets(line,1024,fp)) {
            if (line[0]=='#') continue; // comment
            // UPDATE THIS NUMBER
            if (15 == sscanf(line,
                             // "%lf %lf %lf %*s %lf %*s %lf %*s %lf %*s %lf %*s %lf %*s %lf %*s %lf %*s %lf %*s %lf %*s %lf %i %i %i %lf %lf %s",
                             "%lf %lf %lf %*s %lf %*s %lf %*s %lf %*s %lf %*s %lf %*s %lf %i %i %i %lf %lf %s",
                             &JD,
                             &year,
                             &V_limit,
                             &eta0_V,
                             &c_V,
                             &w_V,
                             &U_limit,
                             &w_U,
                             /* &beta,
                                &GB_limit,
                                &w_GB,
                                &Gmix,
                             */
                             &chisq_dof,
                             &Nobs,
                             &Ndsc,
                             &Ntot,
                             &degSq,
                             &V0,
                             jobID)) {
                // conversion
                U_limit   = orsa::FromUnits(U_limit*orsa::arcsecToRad(),orsa::Unit::HOUR,-1);
                w_U       = orsa::FromUnits(    w_U*orsa::arcsecToRad(),orsa::Unit::HOUR,-1);
                /* beta     *= orsa::degToRad();
                   GB_limit *= orsa::degToRad();
                   w_GB     *= orsa::degToRad();
                */
                
                std::string obsCode;
                orsa::Time epoch;
                int int_year;
                int int_dayOfYear;
                if (!SkyCoverage::processFilename(jobID,
                                                  obsCodeFile.get(),
                                                  obsCode,
                                                  epoch,
                                                  int_year,
                                                  int_dayOfYear)) {
                    ORSA_DEBUG("problems...");
                    boinc_finish(0); 
                }
                
                // update skyCoverage
                skyCoverage->obscode = obsCode;
                skyCoverage->epoch   = epoch;
                //
                skyCoverage->V_limit = V_limit;
                skyCoverage->eta0_V  = eta0_V;
                skyCoverage->V0      = V0;
                skyCoverage->c_V     = c_V;
                skyCoverage->w_V     = w_V;
                //
                skyCoverage->U_limit = U_limit;
                skyCoverage->w_U     = w_U;
                
                break;
            } else {
                ORSA_DEBUG("empty fit.dat file");
                boinc_finish(0); 
            }
        }
        
        fclose(fp);
    }
    
    // orsa::Vector sunPosition, earthPosition, moonPosition;
    // orsa::Vector sunVelocity, earthVelocity, moonVelocity;
    //
  
    osg::ref_ptr<orsa::BodyGroup> bg = new orsa::BodyGroup;
  
    // SUN
    osg::ref_ptr<orsa::Body> sun   = SPICEBody("SUN",orsaSolarSystem::Data::MSun());
    bg->addBody(sun.get());
  
    // EARTH
    osg::ref_ptr<orsa::Body> earth = SPICEBody("EARTH",orsaSolarSystem::Data::MEarth());
    bg->addBody(earth.get());
  
    // MOON
    osg::ref_ptr<orsa::Body> moon  = SPICEBody("MOON",orsaSolarSystem::Data::MMoon());
    bg->addBody(moon.get());
  
    orsa::Orbit earthOrbit;
    // earthOrbit.compute(earth.get(),sun.get(),bg.get(),skyCoverage->epoch.getRef());
    earthOrbit.compute(earth.get(),sun.get(),bg.get(),orsaSolarSystem::J2000());
    
    int rs;
    //
    {
        boinc_resolve_filename_s("randomSeed.dat",resolvedFileName);
        FILE * fp = boinc_fopen(resolvedFileName.c_str(),"r"); 
        if (fp) { 
            gmp_fscanf(fp,"%i",&rs);
            fclose(fp);
        } else {
            ORSA_DEBUG("cannot open randomSeed file");
            boinc_finish(0);   
        }
    }
    //
    const int randomSeed = rs;
    //
    ORSA_DEBUG("randomSeed: %i",randomSeed);
    //
    osg::ref_ptr<orsa::RNG> rnd = new orsa::RNG(randomSeed);
    //
    {
    
        // check if a state is available
        char **result;
        int nrows, ncols;
        sql = "select binary_state from rng";
        rc = sqlite3_get_table(db,sql.c_str(),&result,&nrows,&ncols,&zErr);
        //
        if (nrows==1) {
            ORSA_DEBUG("restoring rnd state...");
            // read last rng state
            std::string sqlselect = "select binary_state from rng";
            sqlite3_stmt * selectstmt;
            rc=sqlite3_prepare(db, sqlselect.c_str(), strlen(sqlselect.c_str()), &selectstmt, NULL);
            sqlite3_step(selectstmt);
            FILE * fp_rnd = fopen("rng.bin", "wb");
            fwrite (sqlite3_column_blob(selectstmt, 0), sqlite3_column_bytes(selectstmt, 0), 1, fp_rnd);
            fclose (fp_rnd);
            sqlite3_finalize(selectstmt);
            //
            fp_rnd = fopen("rng.bin", "rb");
            rnd->gsl_rng_fread(fp_rnd);
            fclose (fp_rnd);
        }
    }
  
    // osg::ref_ptr<OrbitFactory> orbitFactory;
    //
    int z_a_min, z_a_max, z_a_delta;
    int z_e_min, z_e_max, z_e_delta;
    int z_i_min, z_i_max, z_i_delta;
    int z_node_min, z_node_max, z_node_delta;
    int z_peri_min, z_peri_max, z_peri_delta;
    int z_M_min, z_M_max, z_M_delta;
    int z_H_min, z_H_max, z_H_delta;
    //
    unsigned int numGen;
    //
    {
        boinc_resolve_filename_s("grid.dat",resolvedFileName);
        FILE * fp = boinc_fopen(resolvedFileName.c_str(),"r"); 
        if (fp) {
            bool done=false;
            char line[1024];
            while (fgets(line,1024,fp)) {
                if (strlen(line) > 0) {
                    if (line[0] == '#') {
                        continue;
                    }
                }
                if (22 == gmp_sscanf(line,
                                     "%i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i",
                                     &z_a_min, &z_a_max, &z_a_delta,
                                     &z_e_min, &z_e_max, &z_e_delta,
                                     &z_i_min, &z_i_max, &z_i_delta,
                                     &z_node_min, &z_node_max, &z_node_delta,
                                     &z_peri_min, &z_peri_max, &z_peri_delta,
                                     &z_M_min, &z_M_max, &z_M_delta,
                                     &z_H_min, &z_H_max, &z_H_delta,
                                     &numGen)) {
                    done=true;
                }
            }
            fclose(fp);
            if (!done) {
                ORSA_DEBUG("cannot parse file");
                boinc_finish(0);   
            }
        }
    }
  
    const orsa::Time apparentMotion_dt_T = orsa::Time(0,0,1,0,0);
    //
    const double apparentMotion_dt = apparentMotion_dt_T.get_d();
  
    osg::ref_ptr<OrbitID> orbit;
  
    orsa::Vector r;
  
    obsPosCB->getPosition(r,
                          skyCoverage->obscode.getRef(),
                          skyCoverage->epoch.getRef());
    const orsa::Vector observerPosition_epoch = r;
  
    obsPosCB->getPosition(r,
                          skyCoverage->obscode.getRef(),
                          skyCoverage->epoch.getRef()+apparentMotion_dt_T);
    const orsa::Vector observerPosition_epoch_plus_dt = r;
    
    bg->getInterpolatedPosition(r,
                                sun.get(),
                                skyCoverage->epoch.getRef());
    const orsa::Vector sunPosition_epoch = r;
    
    bg->getInterpolatedPosition(r,
                                sun.get(),
                                skyCoverage->epoch.getRef()+apparentMotion_dt_T);
    const orsa::Vector sunPosition_epoch_plus_dt = r;
    
    // double V_nightStart, V_nightStop;
    //
    double V_field;
    
    for (int z_a=z_a_min; z_a<z_a_max; z_a+=z_a_delta) {
        for (int z_e=z_e_min; z_e<z_e_max; z_e+=z_e_delta) {
            for (int z_i=z_i_min; z_i<z_i_max; z_i+=z_i_delta) {
                for (int z_node=z_node_min; z_node<z_node_max; z_node+=z_node_delta) {
                    for (int z_peri=z_peri_min; z_peri<z_peri_max; z_peri+=z_peri_delta) {
                        for (int z_M=z_M_min; z_M<z_M_max; z_M+=z_M_delta) {
                            for (int z_H=z_H_min; z_H<z_H_max; z_H+=z_H_delta) {

                                {
                                    // quick check if NEO
                                    // minimum perihelion: q = a_min*(1-e_max), with min and max of this specific interval
                                    const double q_min = orsa::FromUnits(grain_a_AU*z_a*(1.0-grain_e*(z_e+z_e_delta)),orsa::Unit::AU);
                                    if (q_min > OrbitID::NEO_max_q) {
                                        // ORSA_DEBUG("skipping, no NEOs in this interval");
                                        continue;
                                    }
                                }                                
                                
                                {
                                    // check if already computed this one
                                    char **result;
                                    int nrows, ncols;
                                    char sql_line[1024];
                                    sprintf(sql_line,
                                            "SELECT * FROM grid WHERE z_a_min=%i and z_a_max=%i and z_e_min=%i and z_e_max=%i and z_i_min=%i and z_i_max=%i and z_node_min=%i and z_node_max=%i and z_peri_min=%i and z_peri_max=%i and z_M_min=%i and z_M_max=%i and z_H_min=%i and z_H_max=%i",
                                            z_a,z_a+z_a_delta,
                                            z_e,z_e+z_e_delta,
                                            z_i,z_i+z_i_delta,
                                            z_node,z_node+z_node_delta,
                                            z_peri,z_peri+z_peri_delta,
                                            z_M,z_M+z_M_delta,
                                            z_H,z_H+z_H_delta);
                                    rc = sqlite3_get_table(db,sql_line,&result,&nrows,&ncols,&zErr);
                                    //
                                    if (rc != SQLITE_OK) {
                                        if (zErr != NULL) {
                                            fprintf(stderr,"SQL error: %s\n",zErr);
                                            sqlite3_free(zErr);
                                            boinc_finish(0); 
                                        }
                                    }
                                    // ORSA_DEBUG("nrows: %i  ncols: %i",nrows, ncols);
                                    //
                                    /* for (int i=0; i<nrows; ++i) {
                                       for (int j=0; j<ncols; ++j) {
                                       // i=0 is the header
                                       const int index = (i+1)*ncols+j;
                                       ORSA_DEBUG("result[%i] = %s",index, result[index]);
                                       }
                                       } 
                                    */
                                    //
                                    bool skip=false;
                                    //
                                    if (nrows==1) {
                                        // ORSA_DEBUG("skipping value already computed...");
                                        ORSA_DEBUG("skipping: (%i,%i,%i,%i,%i,%i,%i)",
                                                   z_a,
                                                   z_e,
                                                   z_i,
                                                   z_node,
                                                   z_peri,
                                                   z_M,
                                                   z_H);
                                        skip=true;
                                    } else if (nrows>1) {
                                        ORSA_ERROR("database corrupted, only one entry per grid element is admitted");
                                        boinc_finish(0); 
                                    }
                                    //
                                    sqlite3_free_table(result);
                                    //
                                    if (skip) {
                                        // ORSA_DEBUG("skipping...");
                                        continue;
                                    }
                                }
                                
                                osg::ref_ptr<OrbitFactory> orbitFactory =
                                    new OrbitFactory(grain_a_AU* z_a,
                                                     grain_a_AU*(z_a+z_a_delta),
                                                     grain_e* z_e,
                                                     grain_e*(z_e+z_e_delta),
                                                     grain_i_DEG* z_i,
                                                     grain_i_DEG*(z_i+z_i_delta),
                                                     grain_node_DEG* z_node,
                                                     grain_node_DEG*(z_node+z_node_delta),
                                                     grain_peri_DEG* z_peri,
                                                     grain_peri_DEG*(z_peri+z_peri_delta),
                                                     grain_M_DEG* z_M,
                                                     grain_M_DEG*(z_M+z_M_delta),
                                                     grain_H* z_H,
                                                     grain_H*(z_H+z_H_delta),
                                                     rnd.get(),
                                                     earthOrbit);
	  
                                unsigned int count=0;
                                unsigned int inField=0;
                                osg::ref_ptr< orsa::Statistic<double> > eta_NEO = new orsa::Statistic<double>;
                                osg::ref_ptr< orsa::Statistic<double> > eta_PHO = new orsa::Statistic<double>;
                                for (unsigned int j=0; j<numGen; ++j) {
	    
                                    orbit = orbitFactory->sample();
	    
                                    if (!orbit->isNEO()) {
                                        continue;
                                    }
	    
                                    ++count;
	    
                                    const double orbitPeriod = orbit->period();

                                    // orbit referred to J2000
                        
                                    const double original_M  = orbit->M;
                                    //
                                    orbit->M = original_M + fmod(orsa::twopi() * (skyCoverage->epoch.getRef()-orsaSolarSystem::J2000()).get_d() / orbitPeriod, orsa::twopi());
                                    orbit->relativePosition(r);
                                    const orsa::Vector orbitPosition_epoch = r + sunPosition_epoch;
                                    //
                                    orbit->M = original_M + fmod(orsa::twopi() * (skyCoverage->epoch.getRef()+apparentMotion_dt_T-orsaSolarSystem::J2000()).get_d() / orbitPeriod, orsa::twopi());
                                    orbit->relativePosition(r);
                                    const orsa::Vector orbitPosition_epoch_plus_dt = r + sunPosition_epoch_plus_dt;
                                    //
                                    orbit->M = original_M;
                        
                                    const orsa::Vector dr_epoch          = (orbitPosition_epoch         - observerPosition_epoch);
                                    const orsa::Vector dr_epoch_plus_dt  = (orbitPosition_epoch_plus_dt - observerPosition_epoch_plus_dt);
	    
                                    // orsa::print(dr_nightStart);
                                    // orsa::print(dr_nightStop);
	    
                                    // if (skyCoverage->get(dr_nightStart.normalized(),V_nightStart) && 
                                    // skyCoverage->get(dr_nightStop.normalized(), V_nightStop)) {
                                    if (skyCoverage->get(dr_epoch.normalized(),V_field)) {
	      
                                        ++inField;
	      
                                        /* 
                                           ORSA_DEBUG("   orbitPosition.length: %f [AU]",orsa::FromUnits(   orbitPosition.length(),orsa::Unit::AU,-1));
                                           orsa::print(orbitPosition);
		 
                                           ORSA_DEBUG("observerPosition.length: %f [AU]",orsa::FromUnits(observerPosition.length(),orsa::Unit::AU,-1));
                                           orsa::print(observerPosition);
                                        */
	      
                                        // const double distance = dr_epoch.length();
	      
                                        // const double appvel_arcmin_per_hour = orsa::radToArcmin()*acos(dr_nightStart.normalized()*dr_nightStop.normalized())/orsa::FromUnits((skyCoverage->nightStop.getRef()-skyCoverage->nightStart.getRef()).get_d(),orsa::Unit::HOUR,-1);
                                        // const double night_arc = acos(dr_epoch.normalized()*dr_epoch_plus_dt.normalized())/apparentMotion_dt;
	      
                                        /* 
                                           ORSA_DEBUG("OBJECT in observer FIELD!   distance: %5.2f [AU]   apparent velocity: %f [deg/day]   ORBIT: rs: %i id: %i count: %i inField: %i",
                                           orsa::FromUnits(distance,orsa::Unit::AU,-1),
                                           orsa::FromUnits(orsa::radToDeg()*night_arc,orsa::Unit::DAY),
                                           orbit->randomSeed,orbit->id,
                                           count,
                                           inField);
                                        */
	      
                                        /* 
                                           ORSA_DEBUG("a: %f [AU] e: %f i: %f [deg]",
                                           orsa::FromUnits(orbit->a,orsa::Unit::AU,-1),
                                           orbit->e,
                                           orsa::radToDeg()*orbit->i);
                                        */
	      
                                        /* 
                                           if (orbit->isNEO()) {
                                           ORSA_DEBUG("is NEO!");
                                           }
		 
                                           if (orbit->isIEO()) {
                                           ORSA_DEBUG("is IEO!");
                                           }
		 
                                           if (orbit->isAten()) {
                                           ORSA_DEBUG("is Aten!");
                                           }
		 
                                           if (orbit->isApollo()) {
                                           ORSA_DEBUG("is Apollo!");
                                           }
		 
                                           if (orbit->isAmor()) {
                                           ORSA_DEBUG("is Amor!");
                                           }
		 
                                           if (orbit->isPHO()) {
                                           ORSA_DEBUG("is PHO!");
                                           }
                                        */
	      
                                        //
                                        const orsa::Vector neo2obs    = observerPosition_epoch - orbitPosition_epoch;
                                        const orsa::Vector neo2sun    = sunPosition_epoch      - orbitPosition_epoch;
                                        const double       phaseAngle = acos((neo2obs.normalized())*(neo2sun.normalized()));
                                        // apparent magnitude
                                        const double V = apparentMagnitude(orbit->H,
                                                                           phaseAngle,
                                                                           neo2obs.length(),
                                                                           neo2sun.length());
                                        // apparent velocity
                                        const double U = acos(dr_epoch_plus_dt.normalized()*dr_epoch.normalized())/apparentMotion_dt;
                            
                                        // detection efficiency
                                        const double eta = skyCoverage->eta(V,U);
                            
                                        ORSA_DEBUG("a: %f [AU] e: %f i: %f [deg] H: %f V: %f eta: %e",
                                                   orsa::FromUnits(orbit->a,orsa::Unit::AU,-1),
                                                   orbit->e,
                                                   orsa::radToDeg()*orbit->i,
                                                   orbit->H,
                                                   V,
                                                   eta);
                            
                                        eta_NEO->insert(eta);
                                        //
                                        if (orbit->isPHO()) {
                                            eta_PHO->insert(eta);
                                        }
                            
                                        // if (boinc_is_standalone()) {
	      
                                        // }
	      
                                    } else {
	      
                                        eta_NEO->insert(0.0);
                                        //
                                        if (orbit->isPHO()) {
                                            eta_PHO->insert(0.0);
                                        }
	      
                                    }
	    
                                }
	  
                                {
                                    // all in a transaction
                                    sql = "begin";
                                    rc = sqlite3_exec(db,sql.c_str(),NULL,NULL,&zErr);
	    
                                    // save values in db
                                    double       good_eta_NEO = (eta_NEO->entries() > 0) ? eta_NEO->average()      : 0;
                                    double good_sigma_eta_NEO = (eta_NEO->entries() > 0) ? eta_NEO->averageError() : 0;
                                    double       good_eta_PHO = (eta_PHO->entries() > 0) ? eta_PHO->average()      : 0;
                                    double good_sigma_eta_PHO = (eta_PHO->entries() > 0) ? eta_PHO->averageError() : 0;
                                    //
                                    // ORSA_DEBUG("eta_NEO: %f +/- %f",eta_NEO->average(),eta_NEO->averageError());
                                    //
#warning should use _bind_ sqlite commands for better floating point accuracy
                                    //
                                    char sql_line[1024];
                                    sprintf(sql_line,
                                            "INSERT INTO grid VALUES(%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%g,%g,%g,%g)",
                                            z_a,z_a+z_a_delta,
                                            z_e,z_e+z_e_delta,
                                            z_i,z_i+z_i_delta,
                                            z_node,z_node+z_node_delta,
                                            z_peri,z_peri+z_peri_delta,
                                            z_M,z_M+z_M_delta,
                                            z_H,z_H+z_H_delta,
                                            good_eta_NEO,
                                            good_sigma_eta_NEO,
                                            good_eta_PHO,
                                            good_sigma_eta_PHO);
                                    ORSA_DEBUG("executing: [%s]",sql_line);
                                    ORSA_DEBUG("eta_NEO->entries(): %Zi",eta_NEO->entries().get_mpz_t());
                                    ORSA_DEBUG("eta_PHO->entries(): %Zi",eta_PHO->entries().get_mpz_t());
                                    // do {
                                    rc = sqlite3_exec(db,sql_line,NULL,NULL,&zErr);
                                    // if (rc==SQLITE_BUSY) {
                                    // ORSA_DEBUG("database busy, retrying...");
                                    // }
                                    // } while (rc==SQLITE_BUSY);
                                    //
                                    if (rc != SQLITE_OK) {
                                        if (zErr != NULL) {
                                            fprintf(stderr,"SQL error: %s\n",zErr);
                                            sqlite3_free(zErr);
                                            boinc_finish(0); 
                                        }
                                    }
	    
                                    // save rng state on database...
                                    {
                                        // first delete old entry
                                        rc = sqlite3_exec(db,"delete from rng",NULL,NULL,&zErr);
                                        //
                                        if (rc != SQLITE_OK) {
                                            if (zErr != NULL) {
                                                fprintf(stderr,"SQL error: %s\n",zErr);
                                                sqlite3_free(zErr);
                                                boinc_finish(0); 
                                            }		
                                        }
	      
                                        FILE * fp_rnd = fopen("rng.bin","wb");
                                        if (fp_rnd != 0) {
                                            rnd->gsl_rng_fwrite(fp_rnd);
                                            fclose(fp_rnd);
                                        } else {
                                            ORSA_ERROR("cannot open file rng.bin");  
                                            boinc_finish(1);
                                        }
                                        //
                                        fp_rnd = fopen("rng.bin","rb");
                                        // get the size f1Size of the input file
                                        fseek(fp_rnd, 0, SEEK_END);
                                        const unsigned int fileSize=ftell(fp_rnd);
                                        fseek(fp_rnd, 0, SEEK_SET);
                                        //
                                        char * copyBuffer = (char*)malloc(fileSize+1);
                                        //
                                        if (fileSize != fread(copyBuffer, sizeof(char), fileSize, fp_rnd)) {
                                            free (copyBuffer);
                                            boinc_finish(0); 
                                        }
                                        //
                                        fclose(fp_rnd);
                                        //
                                        sqlite3_stmt * insertstmt;
                                        std::string sqlinsert = "insert into rng (binary_state) values (?);";
                                        rc = sqlite3_prepare(db, sqlinsert.c_str(), strlen(sqlinsert.c_str()), &insertstmt, NULL);
                                        sqlite3_bind_blob(insertstmt, 1, (const void*)copyBuffer, fileSize, SQLITE_STATIC);
                                        sqlite3_step(insertstmt);
                                        sqlite3_finalize(insertstmt);
                                        free (copyBuffer);
                                    }
	    
                                    sql = "commit";
                                    do {
                                        rc = sqlite3_exec(db,sql.c_str(),NULL,NULL,&zErr);
                                        if (rc==SQLITE_BUSY) {
                                            ORSA_DEBUG("database busy, retrying...");
                                        }
                                    } while (rc==SQLITE_BUSY);
	    
                                }
	  
                                // seven "for" iterations...
                            }
                        }
                    }
                }
            }
        }
    }
    
    // cleanup
    do {
        rc = sqlite3_exec(db,"delete from rng",NULL,NULL,&zErr);
    } while (rc==SQLITE_BUSY);
  
    // vacuum
    do {
        rc = sqlite3_exec(db,"vacuum",NULL,NULL,&zErr);
    } while (rc==SQLITE_BUSY);
  
    boinc_finish(0);
  
    return 0;
}

