#include "SurveyReview.h"

#include <orsaSPICE/spice.h>
#include <orsaSPICE/spiceBodyRotationalCallback.h>
#include <orsaSPICE/spiceBodyTranslationalCallback.h>

// BOINC API
#include <boinc_api.h>

// globaly accessible proxy
// osg::ref_ptr<PhaseComponentProxy> phaseComponentProxy = new PhaseComponentProxy(0.01);

// globaly accessible proxy
// osg::ref_ptr<Log10Proxy> log10Proxy = new Log10Proxy(0.01);

orsa::Body * SPICEBody (const std::string  & bodyName,
                        const double & bodyMass) {
  
    orsa::Body * localBody = new orsa::Body;
    localBody->setName(bodyName);
    // localBody->setMass(bodyMass);  
  
    osg::ref_ptr<orsaSPICE::SpiceBodyTranslationalCallback> sbtc = 
        new orsaSPICE::SpiceBodyTranslationalCallback(bodyName);
    orsa::IBPS ibps;
    ibps.inertial = new orsa::PointLikeConstantInertialBodyProperty(bodyMass);
    ibps.translational = sbtc.get();
    localBody->setInitialConditions(ibps);
  
    return localBody;
}

// Taking definitons from: http://neo.jpl.nasa.gov/neo/groups.html

// static members of OrbitID
const double OrbitID::NEO_max_q = FromUnits(1.3,  orsa::Unit::AU);
const double OrbitID::ONE_AU    = FromUnits(1.0,  orsa::Unit::AU);
const double OrbitID::EARTH_q   = FromUnits(0.983,orsa::Unit::AU);
const double OrbitID::EARTH_Q   = FromUnits(1.017,orsa::Unit::AU);

// it looks like only q<1.3 AU is required in the definition
bool OrbitID::isNEO() const {
    if (a*(1-e) < NEO_max_q) {
        return true;
    } else {
        return false;
    }
}

bool OrbitID::isIEO() const {
    if (a*(1+e) < EARTH_q) {
        return true;
    } else {
        return false;
    }
}

bool OrbitID::isAten() const {
    if ( (a < ONE_AU) && (a*(1+e) > EARTH_q) ) {
        return true;
    } else {
        return false;
    }
}

bool OrbitID::isApollo() const {
    if ( (a > ONE_AU) && (a*(1-e) < EARTH_Q) ) {
        return true;
    } else {
        return false;
    }
}

bool OrbitID::isAmor() const {
    if ( (a > ONE_AU) && (a*(1-e) > EARTH_Q) && (a*(1-e) < NEO_max_q) ) {
        return true;
    } else {
        return false;
    }
}

// Earth MOID < 0.05 AU
bool OrbitID::isPHO() const {
    
    double moid, M1, M2;
    if (!orsa::MOID(moid, 
                    M1,
                    M2,
                    earthOrbit,
                    (*this),
                    16,
                    1e-6)) {
        ORSA_DEBUG("problems while computing MOID...");
    }
  
    // ORSA_DEBUG("moid: %f [AU]",orsa::FromUnits(moid,orsa::Unit::AU,-1));
  
    return (moid < FromUnits(0.05,orsa::Unit::AU));
}

OrbitID * OrbitFactory::sample() const {
    
    OrbitID * orbit = new OrbitID(idCounter++,earthOrbit);
    
    orbit->a = FromUnits(a_AU_min+(a_AU_max-a_AU_min)*rnd->gsl_rng_uniform(),orsa::Unit::AU);  
    orbit->e = e_min+(e_max-e_min)*rnd->gsl_rng_uniform();
    orbit->i = orsa::degToRad()*(i_DEG_min+(i_DEG_max-i_DEG_min)*rnd->gsl_rng_uniform());
    orbit->omega_node       = orsa::degToRad()*(node_DEG_min+(node_DEG_max-node_DEG_min)*rnd->gsl_rng_uniform());
    orbit->omega_pericenter = orsa::degToRad()*(peri_DEG_min+(peri_DEG_max-peri_DEG_min)*rnd->gsl_rng_uniform());
    orbit->M                = orsa::degToRad()*(M_DEG_min+(M_DEG_max-M_DEG_min)*rnd->gsl_rng_uniform());
    //
    // orbit->H = H_min+(H_max-H_min)*rnd->gsl_rng_uniform();
    
    orbit->mu = GMSun;
    
    // debug
    /* if (boinc_is_standalone()) {
       if (idCounter%10==0) ORSA_DEBUG("idCounter: %Zi",idCounter.get_mpz_t());
       }
    */
    
    return orbit;
}
