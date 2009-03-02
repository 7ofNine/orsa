#include <orsa/util.h>
// #include <orsa/print.h>

#include <gsl/gsl_math.h>
#include <gsl/gsl_eigen.h>

using namespace orsa;

std::string & orsa::removeLeadingAndTrailingSpaces(std::string & s) {
  const int first = s.find_first_not_of(" ");
  s.erase(0,first);
  const int last  = s.find_last_not_of(" ");
  s.erase(last+1,s.size());
  return s;
}

std::string & orsa::removeAllSpaces(std::string & s) {
  s.erase(remove(s.begin(),s.end(),' '),s.end());
  return s;
}

std::string & orsa::removeLeadingPlusSign(std::string & s) {
  const int first = s.find_first_not_of("+");
  s.erase(0,first);
  return s;
}


/* This code was used in orsa/interaction.cpp at some point
   to determine on the fly a good value for dt. 
   The only unsolved problem is that of using non-tmp IBPS entries,
   as temporary entries are just wrong outside the integrator.
   
   if (ref_b->propulsion.get()) {
   const orsa::Vector propulsionThrust = ref_b->propulsion->getThrust(t);
   if (propulsionThrust.lengthSquared() > orsa::epsilon()*orsa::epsilon()) {
   const orsa::BodyGroup::BodyInterval * bi =
   bg->getBodyInterval(ref_b);
   const orsa::Time dt = 
   std::min(orsa::Time(0,0,0,5,0),
   std::min(bi->max().time.getRef()-t,
   t-bi->min().time.getRef()));
   // REMEMBER: now thrust is in FrenetSerret components
   orsa::Vector T, N, B;
   ORSA_DEBUG("bi->min().time.getRef().tmp: %i",bi->min().tmp);
   ORSA_DEBUG("bi->max().time.getRef().tmp: %i",bi->max().tmp);
   if (t > bi->min().time.getRef()) {
   FrenetSerret(ref_b, bg,
   t,
   -dt,
   T, N, B);
   } else if (t < bi->max().time.getRef()) {
   FrenetSerret(ref_b, bg,
   t,
   dt,
   T, N, B);
   } else {
   ORSA_DEBUG("interval smaller than dt");
   ORSA_DEBUG("--BODY-INTERVAL-TIME-RANGE--");
   print(bi->min().time.getRef());
   print(bi->max().time.getRef());
   ORSA_DEBUG("call time:");
   print(t);
   //
   T = orsa::Vector(1,0,0);
   N = orsa::Vector(0,1,0);
   B = orsa::Vector(0,0,1);
   }
   thrust = 
   propulsionThrust.getX()*T +	
   propulsionThrust.getY()*N +
   propulsionThrust.getZ()*B;
   }
   }
*/

bool orsa::FrenetSerret(const orsa::Body * b,
			orsa::BodyGroup  * bg,
			const orsa::Time & t,
			const orsa::Time & dt,
			orsa::Vector & T,
			orsa::Vector & N,
			orsa::Vector & B) {
  if (dt.getMuSec() == 0) return false;
  orsa::Vector r_t,   v_t;
  orsa::Vector r_tdt, v_tdt;
  if (bg->getInterpolatedPosVel(r_t,  v_t,  b,t) && 
      bg->getInterpolatedPosVel(r_tdt,v_tdt,b,t+dt)) {
    T = v_t.normalized();
    // const orsa::Vector dv = v_tdt-v_t;
    // should chech that velocity is not constant...
    N = ((v_tdt-v_t)/dt.asDouble()).normalized();
    B = externalProduct(T,N).normalized();
    //
    /* 
       ORSA_DEBUG("b: [%s]",b->getName().c_str());
       print(t);
       print(dt);
       print(T);
       print(N);
       print(B);
    */
    //
    return true;
  } else {
    // ORSA_DEBUG("problems...");
    return false;
  }
}

bool orsa::eulerAnglesToMatrix(orsa::Matrix       & m,
			       const orsa::Double & psi,
			       const orsa::Double & theta,
			       const orsa::Double & phi) {
  
  // ORSA_DEBUG("this code needs to be verified!!");
  // #warning "this code needs to be verified!!"
  
  m = orsa::Matrix::identity();
  
  m.rotZ(phi);
  
  m.rotX(theta);
  
  m.rotZ(psi);
  
  return true;
}	

bool orsa::matrixToEulerAngles(orsa::Double       & psi,
			       orsa::Double       & theta,
			       orsa::Double       & phi,
			       const orsa::Matrix & m) {
  
  // const orsa::Matrix l2g = localToGlobal(t);
  
  // const orsa::Double sinTheta = sqrt(one()-int_pow(m.getM33(),2));
  const orsa::Double cosTheta = m.getM33();
  
  // if (fabs(sinTheta) > epsilon()) {
  if ((one()-cosTheta*cosTheta) > (epsilon()*epsilon())) {
    
    psi   = atan2(-m.getM13(),
		   m.getM23());
    
    phi   = atan2(-m.getM31(), 
		  -m.getM32());
    
    // should check this better...
    if (fabs(m.getM23()) > epsilon()) {
      theta = atan2(-m.getM23()/cos(psi),
		     m.getM33());
    } else if (fabs(m.getM31()) > epsilon()) {
      theta = atan2(m.getM31()/sin(phi),
		    m.getM33());
    } else {
      // we should not be here...
      ORSA_DEBUG("problems...");
      theta = zero();
    }
    
  } else {
    
    // ORSA_DEBUG("using singular code...");
    
    psi = theta = zero();
    
    phi = atan2(m.getM21(), 
		m.getM11());
    
  }
  
  if (0) {
    
    ORSA_DEBUG("psi..: %Ff",  psi.get_mpf_t());
    ORSA_DEBUG("theta: %Ff",theta.get_mpf_t());
    ORSA_DEBUG("phi..: %Ff",  phi.get_mpf_t());
    
    orsa::Matrix m = orsa::Matrix::identity();
    //
    m.rotZ(phi);
    m.rotX(theta);
    m.rotZ(psi);
    
    const orsa::Matrix dm = m-m;
    
    ORSA_DEBUG("m: %Ff %Ff %Ff %Ff %Ff %Ff %Ff %Ff %Ff",
	       m.getM11().get_mpf_t(),
	       m.getM12().get_mpf_t(),
	       m.getM13().get_mpf_t(),
	       m.getM21().get_mpf_t(),
	       m.getM22().get_mpf_t(),
	       m.getM23().get_mpf_t(),
	       m.getM31().get_mpf_t(),
	       m.getM32().get_mpf_t(),
	       m.getM33().get_mpf_t());
    
    ORSA_DEBUG("m: %Ff %Ff %Ff %Ff %Ff %Ff %Ff %Ff %Ff",
	       m.getM11().get_mpf_t(),
	       m.getM12().get_mpf_t(),
	       m.getM13().get_mpf_t(),
	       m.getM21().get_mpf_t(),
	       m.getM22().get_mpf_t(),
	       m.getM23().get_mpf_t(),
	       m.getM31().get_mpf_t(),
	       m.getM32().get_mpf_t(),
	       m.getM33().get_mpf_t());
    
    ORSA_DEBUG("dm: %Ff %Ff %Ff %Ff %Ff %Ff %Ff %Ff %Ff",
	       dm.getM11().get_mpf_t(),
	       dm.getM12().get_mpf_t(),
	       dm.getM13().get_mpf_t(),
	       dm.getM21().get_mpf_t(),
	       dm.getM22().get_mpf_t(),
	       dm.getM23().get_mpf_t(),
	       dm.getM31().get_mpf_t(),
	       dm.getM32().get_mpf_t(),
	       dm.getM33().get_mpf_t());
  }
  
  return true;
}	


orsa::Matrix orsa::QuaternionToMatrix (const orsa::Quaternion & q) {
  
  const orsa::Double s = q.getScalar();
  const orsa::Vector v = q.getVector();
  
  const orsa::Double q0 = s;
  const orsa::Double q1 = v.getX();
  const orsa::Double q2 = v.getY();
  const orsa::Double q3 = v.getZ();
  
  // Eq. (7.7) book by J.B. Kuipers on Quaternions
  return orsa::Matrix(2*q0*q0-1+2*q1*q1,   2*q1*q2-2*q0*q3, 2*q1*q3+2*q0*q2,
		      2*q1*q2+2*q0*q3,   2*q0*q0-1+2*q2*q2, 2*q2*q3-2*q0*q1,
		      2*q1*q3-2*q0*q2,     2*q2*q3+2*q0*q1, 2*q0*q0-1+2*q3*q3);
}

orsa::Quaternion orsa::MatrixToQuaternion (const orsa::Matrix & m) {
  //! assumes that m is a rotation matrix, with det(m)=1;
  
  // From Sec. 7.9 of Quaternions book bu J.B. Kuipers
  
  const orsa::Double q0 = sqrt(m.getM11()+m.getM22()+m.getM33()+1)/2;
  
  const orsa::Double q1 = (m.getM23()-m.getM32())/(4*q0);
  const orsa::Double q2 = (m.getM31()-m.getM13())/(4*q0);
  const orsa::Double q3 = (m.getM12()-m.getM21())/(4*q0);
  
  return orsa::Quaternion(q0, orsa::Vector(q1,q2,q3));
}

orsa::Matrix orsa::localToGlobal(const orsa::Body       * b,
				 const orsa::BodyGroup  * bg,
				 const orsa::Time       & t) {
  
  if ((bg==0) || (b==0)) {
    ORSA_ERROR("invalid pointer...");
    return orsa::Matrix::identity();
  }
  
  orsa::IBPS ibps;
  if (bg->getInterpolatedIBPS(ibps, b, t)) { 
    if (ibps.rotational.get()) {
      const orsa::Matrix m = QuaternionToMatrix(ibps.rotational.get()->getQ());
      orsa::Matrix m_tr;
      orsa::Matrix::transpose(m, m_tr);
      return m_tr;
    } else {
      // ORSA_DEBUG("problems... body: [%s]",b->getName().c_str());
      return orsa::Matrix::identity();
    }
  } else {
    ORSA_DEBUG("problems...body: [%s]",b->getName().c_str());
    return orsa::Matrix::identity();
  }
}

orsa::Matrix orsa::globalToLocal(const orsa::Body       * b,
				 const orsa::BodyGroup  * bg,
				 const orsa::Time       & t) {
  
  if ((bg==0) || (b==0)) {
    ORSA_ERROR("invalid pointer...");
    return orsa::Matrix::identity();
  }
  
  orsa::IBPS ibps;
  if (bg->getInterpolatedIBPS(ibps, b, t)) { 
    if (ibps.rotational.get()) {
      return QuaternionToMatrix(ibps.rotational.get()->getQ());
    } else {
      // ORSA_DEBUG("problems... body: [%s]",b->getName().c_str());
      return orsa::Matrix::identity();
    }
  } else {
    ORSA_DEBUG("problems... body: [%s]",b->getName().c_str());
    return orsa::Matrix::identity();
  }
}

orsa::Double orsa::asteroidDiameter(const orsa::Double & p, 
				    const orsa::Double & H) {
  return orsa::FromUnits(1329,orsa::Unit::KM)*pow(10,-0.2*H)/sqrt(p);
}

// ConstantZRotation

/* 
   bool ConstantZRotation::update(const orsa::Time & t) {
   const Double _phi = _phi0 + _omega*(t-_t0).asDouble();
   orsa::Matrix localMatrix = Matrix::identity();
   // same rotation as Attitude::localToGlobal(t)
   localMatrix.rotZ(_phi);
   _m = localMatrix;
   return true;
   }
*/


void orsa::principalAxis(orsa::Matrix & genericToPrincipal,
			 orsa::Matrix & principalInertiaMatrix,
			 const orsa::Matrix & inertiaMatrix) {
  
  const orsa::Matrix & I = inertiaMatrix;
  
  double data[] = { 
    I.getM11().get_d(), I.getM12().get_d(), I.getM13().get_d(), 
    I.getM21().get_d(), I.getM22().get_d(), I.getM23().get_d(), 
    I.getM31().get_d(), I.getM32().get_d(), I.getM33().get_d()};
  
  gsl_matrix_view m = gsl_matrix_view_array (data, 3, 3);
  
  gsl_vector * eval = gsl_vector_alloc (3);
  
  gsl_matrix * evec = gsl_matrix_alloc (3, 3);
  
  gsl_eigen_symmv_workspace * w = gsl_eigen_symmv_alloc (3);
  
  gsl_eigen_symmv (&m.matrix, eval, evec, w);
  
  gsl_eigen_symmv_free (w);
  
  gsl_eigen_symmv_sort (eval, evec, 
			GSL_EIGEN_SORT_ABS_ASC);
  
  principalInertiaMatrix.set(gsl_vector_get(eval,0),0,0,
			     0,gsl_vector_get(eval,1),0,
			     0,0,gsl_vector_get(eval,2));
  
  genericToPrincipal.set(gsl_matrix_get(evec,0,0),gsl_matrix_get(evec,0,1),gsl_matrix_get(evec,0,2),
			 gsl_matrix_get(evec,1,0),gsl_matrix_get(evec,1,1),gsl_matrix_get(evec,1,2),
			 gsl_matrix_get(evec,2,0),gsl_matrix_get(evec,2,1),gsl_matrix_get(evec,2,2));
  
  // force det=1
  // genericToPrincipal /= genericToPrincipal.determinant();
  
  gsl_vector_free (eval);
  gsl_matrix_free (evec);
}
