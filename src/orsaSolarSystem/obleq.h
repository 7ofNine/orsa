#ifndef _ORSA_SOLAR_SYSTEM_OBLEQ_
#define _ORSA_SOLAR_SYSTEM_OBLEQ_

#include <orsa/datetime.h>
#include <orsa/double.h>
#include <orsa/matrix.h>

namespace orsaSolarSystem {
  
  orsa::Double obleq(const orsa::Time &);
  
  orsa::Double obleqJ2000();
  
  orsa::Matrix eclipticToEquatorial();
  orsa::Matrix equatorialToEcliptic();
  
}; // namespace orsaSolarSystem

#endif // _ORSA_SOLAR_SYSTEM_OBLEQ_
