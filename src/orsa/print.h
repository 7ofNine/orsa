#ifndef _ORSA_PRINT_
#define _ORSA_PRINT_

#include <orsa/datetime.h>
#include <orsa/matrix.h>
#include <orsa/quaternion.h>

namespace orsa {
  
  inline void print(const double & d) {
    ORSA_DEBUG("d: %+20.12g",d);
  }
  
  inline void print(const orsa::Vector & v) {
    ORSA_DEBUG("v: length: %.12e [%+.12e,%+.12e,%+.12e]",
	       v.length(),
	       v.getX(),
	       v.getY(),
	       v.getZ());
  }
  
  inline void print(const orsa::Matrix & m) {
    ORSA_DEBUG("m: det(m) = %.12e\n"
	       "[%+.12e,%+.12e,%+.12e]\n"
	       "[%+.12e,%+.12e,%+.12e]\n"
	       "[%+.12e,%+.12e,%+.12e]",
	       m.determinant(),
	       m.getM11(),  m.getM12(),  m.getM13(), 
	       m.getM21(),  m.getM22(),  m.getM23(), 
	       m.getM31(),  m.getM32(),  m.getM33());  
  }
  
  inline void print(const orsa::Quaternion & q) {
    ORSA_DEBUG("q: s: %+9.6e   v: %9.6e x [%6.3f,%6.3f,%6.3f]  l: %9.6e",
	       q.getScalar(),
	       q.getVector().length(),
	       q.getVector().normalized().getX(),
	       q.getVector().normalized().getY(),
	       q.getVector().normalized().getZ(),
	       q.length());
  }
  
  inline void print(const orsa::Time & t) {
    ORSA_DEBUG("t: %20.14f [day]",orsa::FromUnits(t.get_d(),orsa::Unit::DAY,-1));
  }
  
} // namespace orsa

#endif // _ORSA_PRINT_
