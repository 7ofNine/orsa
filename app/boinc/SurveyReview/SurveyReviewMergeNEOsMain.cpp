#include "grain.h"

#include <orsa/debug.h>
#include <orsa/print.h>
#include <orsa/statistic.h>

#include <orsaSolarSystem/data.h>
#include <orsaSolarSystem/datetime.h>
#include <orsaSolarSystem/print.h>

#include <orsa/util.h>
#include <orsaInputOutput/MPC_asteroid.h>

#include "grain.h"
#include "fit.h"
#include "skycoverage.h"
#include "eta.h"
#include "SurveyReview.h"

// SQLite3
#include "sqlite3.h"

int main(int argc, char ** argv) {
    
    if (argc < 3) {
        ORSA_DEBUG("Usage: %s <sqlite-merged-db> <allEta-file(s)>",argv[0]);
        exit(0);
    }
    
    orsa::Debug::instance()->initTimer();
    
    ORSA_DEBUG("process ID: %i",getpid());
    
    orsaSPICE::SPICE::instance()->loadKernel("de405.bsp");
    
    // needed to work with SQLite database
    sqlite3     * db;
    char        * zErr;
    int           rc;
    std::string   sql;
    
    {
        // open database
        rc = sqlite3_open(argv[1],&db);
        // rc = sqlite3_open_v2(argv[1],&db,SQLITE_OPEN_READONLY,NULL);
        //
        if (rc) {
            fprintf(stderr,"Can't open db: %s\n",sqlite3_errmsg(db));
            sqlite3_close(db);
            exit(0);
        }
    }
    
    {
        // integrity_check
        ORSA_DEBUG("checking integrity of database");
        
        int nrows, ncols;
        char * * sql_result;
        do {
            rc = sqlite3_get_table(db,"pragma integrity_check",&sql_result,&nrows,&ncols,&zErr);
            if (rc==SQLITE_BUSY) {
                ORSA_DEBUG("database busy, retrying...");
                usleep(100000);
            }
        } while (rc==SQLITE_BUSY);
        if (rc != SQLITE_OK) {
            if (zErr != NULL) {
                ORSA_DEBUG("SQL error: %s\n",zErr); 
                sqlite3_free(zErr);
                sqlite3_close(db);
                exit(0);
            }
        }
        //
        bool integrity_check_passed=false;
        if (nrows==1) {
            if (ncols==1) {
                if (sql_result[1]==std::string("ok")) {
                    integrity_check_passed=true;
                }
            }
        }
        if (!integrity_check_passed) {
            ORSA_DEBUG("SQLite problem: integrity_check failed\n"); 
            sqlite3_close(db);
            exit(0); 
        }
        sqlite3_free_table(sql_result);
    }

    // read allEta files and extract ID (number or designation) of all observed objects (not only NEOs)
    std::list<unsigned int>      number_observed;
    std::list<std::string>  designation_observed;
    {
        const unsigned int numFiles=argc-2;
        std::vector <EfficiencyData> etaData;
        for (unsigned int fileID=0; fileID<numFiles; ++fileID) {
            etaData.clear();
            readEfficiencyDataFile(etaData,argv[fileID+2]);
            ORSA_DEBUG("file: [%s]   etaData.size(): %i",argv[fileID+2],etaData.size());
            for (unsigned int k=0; k<etaData.size(); ++k) {
                if (etaData[k].number.isSet()) {
                    number_observed.push_back(etaData[k].number.getRef());
                } else if (etaData[k].designation.isSet()) {
                    designation_observed.push_back(etaData[k].designation.getRef());
                } else {
                    ORSA_DEBUG("problem: neither number or designation is set");
                }
            }
        }
        number_observed.sort();
        number_observed.unique();
        designation_observed.sort();
        designation_observed.unique();
    }
    
    if (0) {
        // debug output
        {
            std::list<unsigned int>::const_iterator it = number_observed.begin();
            while (it != number_observed.end()) {
                ORSA_DEBUG("observed: %6i",(*it));
                ++it;
            }
        }
        {
            std::list<std::string>::const_iterator it = designation_observed.begin();
            while (it != designation_observed.end()) {
                ORSA_DEBUG("observed: %10s",(*it).c_str());
                ++it;
            }
        }
    }
    
    osg::ref_ptr<orsaInputOutput::MPCAsteroidFile> orbitFile =
        new orsaInputOutput::MPCAsteroidFile;
    orbitFile->select_NEO = true;
    orbitFile->setFileName("MPCORB.DAT.gz");
    orbitFile->read();
    ORSA_DEBUG("selected orbits: %i",orbitFile->_data.size());

    {
        // reset all object counts to zero
        ORSA_DEBUG("resetting NEO counts to 0");
        std::string sql;        
        sql = "begin";
        do {
            rc = sqlite3_exec(db,sql.c_str(),NULL,NULL,&zErr);
            if (rc==SQLITE_BUSY) {
                ORSA_DEBUG("database busy, retrying...");
                usleep(100000);
            }
        } while (rc==SQLITE_BUSY);
        if (rc != SQLITE_OK) {
            if (zErr != NULL) {
                ORSA_DEBUG("SQL error: %s\n",zErr);
                sqlite3_free(zErr);
                sqlite3_close(db);
                exit(0);
            }
        }
        
        sql = "UPDATE grid SET N_NEO=0,N_PHO=0,NEO_in_field=0,PHO_in_field=0";
        do {
            rc = sqlite3_exec(db,sql.c_str(),NULL,NULL,&zErr);
            if (rc==SQLITE_BUSY) {
                ORSA_DEBUG("database busy, retrying...");
                usleep(100000);
            }
        } while (rc==SQLITE_BUSY);
        if (rc != SQLITE_OK) {
            if (zErr != NULL) {
                ORSA_DEBUG("SQL error: %s\n",zErr);
                sqlite3_free(zErr);
                sqlite3_close(db);
                exit(0);
            }
        }
        
        sql = "commit";
        do {
            rc = sqlite3_exec(db,sql.c_str(),NULL,NULL,&zErr);
            if (rc==SQLITE_BUSY) {
                ORSA_DEBUG("database busy, retrying...");
                usleep(100000);
            }
        } while (rc==SQLITE_BUSY);
        if (rc != SQLITE_OK) {
            if (zErr != NULL) {
                fprintf(stderr,"SQL error: %s\n",zErr);
                sqlite3_free(zErr);
                sqlite3_close(db);
                exit(0); 
            }
        }
    }

    // a bodygroup with Earth and Sun, to compute the Earth MOID to check if a NEO is a PHO  
    osg::ref_ptr<orsa::BodyGroup> bg = new orsa::BodyGroup;
    // SUN
    osg::ref_ptr<orsa::Body> sun   = SPICEBody("SUN",orsaSolarSystem::Data::MSun());
    bg->addBody(sun.get());
    // EARTH
    osg::ref_ptr<orsa::Body> earth = SPICEBody("EARTH",orsaSolarSystem::Data::MEarth());
    bg->addBody(earth.get());
    // MOON
    /* osg::ref_ptr<orsa::Body> moon  = SPICEBody("MOON",orsaSolarSystem::Data::MMoon());
       bg->addBody(moon.get());
    */
    
    {
        // now the real work
        
#warning keep reference epoch updated with code in SurveyReviewMultipleJobsSubmission
        const double JD = 2455650; // epoch of orbits
        const orsa::Time orbitEpoch = orsaSolarSystem::julianToTime(JD);
        
        orsa::Orbit tmpOrbit;
        tmpOrbit.compute(earth.get(),sun.get(),bg.get(),orbitEpoch);
        const orsa::Orbit earthOrbit = tmpOrbit;
        
        std::string sql;        
        
        sql = "begin";
        do {
            rc = sqlite3_exec(db,sql.c_str(),NULL,NULL,&zErr);
            if (rc==SQLITE_BUSY) {
                ORSA_DEBUG("database busy, retrying...");
                usleep(100000);
            }
        } while (rc==SQLITE_BUSY);
        if (rc != SQLITE_OK) {
            if (zErr != NULL) {
                ORSA_DEBUG("SQL error: %s\n",zErr);
                sqlite3_free(zErr);
                sqlite3_close(db);
                exit(0);
            }
        }
        
        orsaInputOutput::MPCAsteroidData::const_iterator it_orb =
            orbitFile->_data.begin();
        while (it_orb != orbitFile->_data.end()) {
            
            bool found=false;
            
            if ((*it_orb).number.isSet()) {
                std::list<unsigned int>::const_iterator it_num = number_observed.begin();
                while (it_num != number_observed.end()) {
                    if ((*it_num) == (*it_orb).number.getRef()) {
                        found=true;
                        break;
                    }
                    ++it_num;
                }
            } else if ((*it_orb).designation.isSet()) {
                std::list<std::string>::const_iterator it_des = designation_observed.begin();
                while (it_des != designation_observed.end()) {
                    if ((*it_des) == (*it_orb).designation.getRef()) {
                        found=true;
                        break;
                    }
                    ++it_des;
                }
            } else {
                ORSA_DEBUG("neither one is set?");
            }
            
            {
                
#warning should have these delta values somewhere else, once and for all
                const int z_a_delta    = lrint( 0.05/grain_a_AU);
                const int z_e_delta    = lrint( 0.05/grain_e);
                const int z_i_delta    = lrint( 5.00/grain_i_DEG);
                const int z_node_delta = lrint(30.00/grain_node_DEG);
                const int z_peri_delta = lrint(30.00/grain_peri_DEG);
                const int z_M_delta    = lrint(30.00/grain_M_DEG);
                const int z_H_delta    = lrint( 1.00/grain_H);
                
                const double orbit_M_at_epoch = fmod(orsa::twopi()+fmod((*it_orb).orbit.getRef().M+orsa::twopi()*(orbitEpoch-(*it_orb).orbit.getRef().epoch.getRef()).get_d()/(*it_orb).orbit.getRef().period(),orsa::twopi()),orsa::twopi());
                
                const int z_a_min    = (  lrint(orsa::FromUnits((*it_orb).orbit.getRef().a,orsa::Unit::AU,-1)/grain_a_AU)/z_a_delta)*z_a_delta;
                const int z_a_max    = (1+lrint(orsa::FromUnits((*it_orb).orbit.getRef().a,orsa::Unit::AU,-1)/grain_a_AU)/z_a_delta)*z_a_delta;
                const int z_e_min    = (  lrint((*it_orb).orbit.getRef().e/grain_e)/z_e_delta)*z_e_delta;
                const int z_e_max    = (1+lrint((*it_orb).orbit.getRef().e/grain_e)/z_e_delta)*z_e_delta;
                const int z_i_min    = (  lrint((*it_orb).orbit.getRef().i*orsa::radToDeg()/grain_i_DEG)/z_i_delta)*z_i_delta;
                const int z_i_max    = (1+lrint((*it_orb).orbit.getRef().i*orsa::radToDeg()/grain_i_DEG)/z_i_delta)*z_i_delta;
                const int z_node_min = (  lrint((*it_orb).orbit.getRef().omega_node*orsa::radToDeg()/grain_node_DEG)/z_node_delta)*z_node_delta;
                const int z_node_max = (1+lrint((*it_orb).orbit.getRef().omega_node*orsa::radToDeg()/grain_node_DEG)/z_node_delta)*z_node_delta;
                const int z_peri_min = (  lrint((*it_orb).orbit.getRef().omega_pericenter*orsa::radToDeg()/grain_peri_DEG)/z_peri_delta)*z_peri_delta;
                const int z_peri_max = (1+lrint((*it_orb).orbit.getRef().omega_pericenter*orsa::radToDeg()/grain_peri_DEG)/z_peri_delta)*z_peri_delta;
                const int z_M_min    = (  lrint(orbit_M_at_epoch*orsa::radToDeg()/grain_M_DEG)/z_M_delta)*z_M_delta;
                const int z_M_max    = (1+lrint(orbit_M_at_epoch*orsa::radToDeg()/grain_M_DEG)/z_M_delta)*z_M_delta;
                
#warning MAKE SURE you are using the correct definition for z_H
                // this is the same as z_H_max: all objects with H up to H_max
                // const int z_H     = (1+lrint((*it_orb).H.getRef()/grain_H)/z_H_delta)*z_H_delta;
                //
                // this is the same as z_H_min: all objects with H up to H_min
                const int z_H     = (lrint((*it_orb).H.getRef()/grain_H)/z_H_delta)*z_H_delta;
                
                ORSA_DEBUG("H: %g   z_H: %i",(*it_orb).H.getRef(),z_H);
                
                bool isPHO=false;
                {
                    static unsigned int oID=0;
                    osg::ref_ptr<OrbitID> orbitID = new OrbitID(oID++,earthOrbit);
                    orbitID->a                = (*it_orb).orbit.getRef().a;
                    orbitID->e                = (*it_orb).orbit.getRef().e;
                    orbitID->i                = (*it_orb).orbit.getRef().i;
                    orbitID->omega_node       = (*it_orb).orbit.getRef().omega_node;
                    orbitID->omega_pericenter = (*it_orb).orbit.getRef().omega_pericenter;
                    orbitID->M                = orbit_M_at_epoch;
                    
                    orbitID->mu =orsaSolarSystem::Data::GMSun();
                    
                    isPHO=orbitID->isPHO();
                }
                
                if (1) {
                    // debug output
                    if ((*it_orb).number.isSet()) {
                        ORSA_DEBUG("observed: [%i] obj: [%i] PHO: [%i] z_a: [%i,%i] z_e: [%i,%i] z_i: [%i,%i] z_node: [%i,%i] z_peri: [%i,%i] z_M: [%i,%i] z_H: %i",
                                   found,
                                   (*it_orb).number.getRef(),
                                   isPHO,
                                   z_a_min,
                                   z_a_max,
                                   z_e_min,
                                   z_e_max,
                                   z_i_min,
                                   z_i_max,
                                   z_node_min,
                                   z_node_max,
                                   z_peri_min,
                                   z_peri_max,
                                   z_M_min,
                                   z_M_max,
                                   z_H);
                    } else if ((*it_orb).designation.isSet()) {
                        ORSA_DEBUG("observed: [%i] obj: [%s] PHO: [%i] z_a: [%i,%i] z_e: [%i,%i] z_i: [%i,%i] z_node: [%i,%i] z_peri: [%i,%i] z_M: [%i,%i] z_H: %i",
                                   found,
                                   (*it_orb).designation.getRef().c_str(),
                                   isPHO,
                                   z_a_min,
                                   z_a_max,
                                   z_e_min,
                                   z_e_max,
                                   z_i_min,
                                   z_i_max,
                                   z_node_min,
                                   z_node_max,
                                   z_peri_min,
                                   z_peri_max,
                                   z_M_min,
                                   z_M_max,
                                   z_H);
                    } else {
                        ORSA_DEBUG("neither one is set?");
                    }
                }
                
                char sql_line[1024];
                if (isPHO) {
                    if (found) {
                        sprintf(sql_line,"UPDATE grid SET N_NEO=N_NEO+1, N_PHO=N_PHO+1, NEO_in_field=NEO_in_field+1, PHO_in_field=PHO_in_field+1");
                    } else {
                        sprintf(sql_line,"UPDATE grid SET N_NEO=N_NEO+1, N_PHO=N_PHO+1");
                    }
                } else {
                    if (found) {
                        sprintf(sql_line,"UPDATE grid SET N_NEO=N_NEO+1, NEO_in_field=NEO_in_field+1");
                    } else {
                        sprintf(sql_line,"UPDATE grid SET N_NEO=N_NEO+1");
                    }
                }
                // then the WHERE ... (extra white space as first character!)
                char sql_where[1024];
                // NOTE the "<=" in z_H 
                sprintf(sql_where," WHERE z_a_min=%i and z_a_max=%i and z_e_min=%i and z_e_max=%i and z_i_min=%i and z_i_max=%i and z_node_min=%i and z_node_max=%i and z_peri_min=%i and z_peri_max=%i and z_M_min=%i and z_M_max=%i and z_H<=%i",
                        z_a_min,z_a_max,
                        z_e_min,z_e_max,
                        z_i_min,z_i_max,
                        z_node_min,z_node_max,
                        z_peri_min,z_peri_max,
                        z_M_min,z_M_max,
                        z_H);
                strcat(sql_line,sql_where);
                ORSA_DEBUG("executing [%s]",sql_line);
                //
                do {
                    rc = sqlite3_exec(db,sql_line,NULL,NULL,&zErr);
                    if (rc==SQLITE_BUSY) {
                        ORSA_DEBUG("database busy, retrying...");
                        usleep(100000);
                    }
                } while (rc==SQLITE_BUSY);
                if (rc != SQLITE_OK) {
                    if (zErr != NULL) {
                        ORSA_DEBUG("SQL error: %s\n",zErr);
                        sqlite3_free(zErr);
                        sqlite3_close(db);
                        exit(0);
                    }
                }
                
            }
            
            ++it_orb;
        }
        
        sql = "commit";
        do {
            rc = sqlite3_exec(db,sql.c_str(),NULL,NULL,&zErr);
            if (rc==SQLITE_BUSY) {
                ORSA_DEBUG("database busy, retrying...");
                usleep(100000);
            }
        } while (rc==SQLITE_BUSY);
        if (rc != SQLITE_OK) {
            if (zErr != NULL) {
                fprintf(stderr,"SQL error: %s\n",zErr);
                sqlite3_free(zErr);
                sqlite3_close(db);
                exit(0); 
            }
        }
        
    }
    
    sqlite3_close(db);
    
    return 0;
}

