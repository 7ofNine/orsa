#include <orsa/integrator.h>

#include <orsa/bodygroup.h>
#include <orsa/statistic.h>

#include <iostream>

using namespace orsa;

Integrator::Integrator() : Referenced(true) {
  progressiveCleaningSteps = 0;
}

bool Integrator::integrate(orsa::BodyGroup  * bg,
			   const orsa::Time & start,
			   const orsa::Time & stop,
			   const orsa::Time & sampling_period) {
  
  // #warning "remember: if any of the bodies has propulsion, care should be taken for timestep management, to hit all the intervals where propulsion is used"
  
  doAbort = false;
 
  /* 
     ORSA_DEBUG("CALL start: %Ff [day]",
     FromUnits(FromUnits(start.getMuSec(),Unit::MICROSECOND),Unit::DAY,-1).get_mpf_t());
     ORSA_DEBUG("CALL stop:  %Ff [day]",
     FromUnits(FromUnits(stop.getMuSec(),Unit::MICROSECOND),Unit::DAY,-1).get_mpf_t());
  */
  
  // always start <= stop
  // but integration direction may not be from start to stop.... 
  // maybe I should change the name of the variables!!
  if (start > stop) {
    // ORSA_DEBUG("inverting...");
    return integrate(bg,stop,start,sampling_period);
  }
  
  Time t1, t2;
  if (!(bg->getCommonInterval(t1, t2, false))) {
    ORSA_ERROR("cannot find common interval");
    return false;
  }
  //
  // the lowest time in the common interval
  // const Time t0 = t1;
  //
  if (0) {
    ORSA_DEBUG("t1: %Ff [day]",
	       FromUnits(FromUnits(t1.getMuSec(),Unit::MICROSECOND),Unit::DAY,-1).get_mpf_t());
    ORSA_DEBUG("t2: %Ff [day]",
	       FromUnits(FromUnits(t2.getMuSec(),Unit::MICROSECOND),Unit::DAY,-1).get_mpf_t());
  }
  //
  /* 
     if (t0 != start) {
     ORSA_DEBUG("splitting integration...");
     // two separate integrations: the first forward, the second backward in time
     // inverted order...
     return (integrate(bg,t0,start,sampling_period) && 
     integrate(bg,t0,stop, sampling_period));
     }
  */
  //
  bool backwardSplit = false;
  if ((start < t1) && (t1 != stop)) {
    // ORSA_DEBUG("splitting integration: start < t1");
    // integrate(bg,start,t1,sampling_period);
    backwardSplit = true;
  }
  //
  bool forwardSplit = false;
  if ((stop > t2) && (t2 != start)) {
    // ORSA_DEBUG("splitting integration: stop > t2");
    // integrate(bg,t2,stop, sampling_period);
    forwardSplit = true;
  }
  //
  if (backwardSplit) {
    if (forwardSplit) {
      // ORSA_DEBUG("B+F");
      return (integrate(bg,start,t1,sampling_period) &&
	      integrate(bg,t2,stop, sampling_period));
    } else {
      // ORSA_DEBUG("B");
      return integrate(bg,start,t1,sampling_period);
    }
  } else if (forwardSplit) {
    // ORSA_DEBUG("F");
    return integrate(bg,t2,stop, sampling_period);
  }
  
  /* 
     Cache<mpz_class> sign;
     Time t;
     Time tEnd;
     //
     if (t1 == stop) {
     t = t1;
     tEnd = start;
     sign.set(-1);
     } else if (t2 == start) {
     t = t2;
     tEnd = stop;
     sign.set(+1);
     } else {
     // this is the case of a call for refinement, to create one more accurate snapshot of the system
     if (getClosestCommonTime(t, start, false)) {
     
     } else {
     ORSA_DEBUG("problems...");
     }
     }
  */
  //
  Cache<mpz_class> sign;
  Time t;
  Time tEnd;
  //
  if (t1 == stop) {
    t    = t1;
    tEnd = start;
    sign.set(-1);
    // ORSA_DEBUG("--SIGN--MARK--");
  } else if (t2 == start) {
    t    = t2;
    tEnd = stop;
    sign.set(+1);
    // ORSA_DEBUG("--SIGN--MARK--");
  } else {
    
    // this is the case of a call for refinement, to create one more accurate snapshot of the system
    orsa::Time ctstart, ctstop;
    if (!bg->getClosestCommonTime(ctstart, start, false)) {
      ORSA_DEBUG("problems...");
    }
    if (!bg->getClosestCommonTime(ctstop, stop, false)) {
      ORSA_DEBUG("problems...");
    }
    
    ORSA_DEBUG("ctstart: %Ff [day]",
	       FromUnits(FromUnits(ctstart.getMuSec(),Unit::MICROSECOND),Unit::DAY,-1).get_mpf_t());
    ORSA_DEBUG(" ctstop: %Ff [day]",
	       FromUnits(FromUnits(ctstop.getMuSec(),Unit::MICROSECOND),Unit::DAY,-1).get_mpf_t());
    
    if ((ctstart==start) && (ctstop==stop)) {
      // nothing to be done...
    } else if (ctstart==start) {
      ORSA_DEBUG("new code ctstart...");
      t    = ctstop;
      tEnd = stop;
      sign.set((t-tEnd).getMuSec()/abs((t-tEnd).getMuSec()));
    } else if (ctstop==stop) {
      ORSA_DEBUG("new code ctstop...");
      t    = ctstart;
      tEnd = start;
      sign.set((t-tEnd).getMuSec()/abs((t-tEnd).getMuSec()));
    } else {
      ORSA_DEBUG("---- CASE NOT HANDLED YET ----");
    }
  }	
  
  /* 
     mpz_class sign;
     //
     if (start < stop) {
     sign = 1;
     } else {
     sign = -1;
     }
  */
  
  const Time zeroTime = Time(0);
  
  // always start <= stop
  /* 
  // CANNOT DO THIS!!  
  if (start > stop) {
  ORSA_DEBUG("inverting...");
  return integrate(bg,stop,start,sampling_period);
  }
  */
  
  //
  /* 
     ORSA_DEBUG("t1: %Ff [day]",
     FromUnits(FromUnits(_t1.getMuSec(),Unit::MICROSECOND),Unit::DAY,-1).get_mpf_t());
     ORSA_DEBUG("t2: %Ff [day]",
     FromUnits(FromUnits(_t2.getMuSec(),Unit::MICROSECOND),Unit::DAY,-1).get_mpf_t());
  */
  
  /* 
     if ((start != _t1) || (_t1 < _t2)){
     ORSA_DEBUG("simple assumptions not working...");
     return false;
     }
  */
  
  // reset variables, needed when the same integrator is used multiple times
  // ORSA_DEBUG("calling Integrator::reset()...");
  // 
  reset();
  
  // integration statistics
  osg::ref_ptr< Statistic<Double> > stat = new Statistic<Double>;
  //
  osg::ref_ptr< RunningStatistic<Double> > rs = new RunningStatistic<Double>;
  rs->setLength(10);  
  
  // orsa::Time dt = sampling_period;
  //
  orsa::Time call_dt = sampling_period*sign.getRef();
  orsa::Time next_dt = sampling_period*sign.getRef();
  
  /* 
     if (start > stop) {
     call_dt = -call_dt;
     next_dt = -next_dt;
     }
  */
  
  bool ret_val = true;
  
  // const Time sampling_period(0,0,0,15,0);  // d,H,M,S,mu_S
 
  // while (t <= stop) {
  
  // a better first step
  if ((t+next_dt-tEnd)*sign.getRef() > zeroTime) {
    next_dt = tEnd - t;
  }
  
  while((tEnd-t)*sign.getRef() > zeroTime) {
    
    /* 
       ORSA_DEBUG("next_dt: %Ff   (mu: %Zi)",
       FromUnits(FromUnits(next_dt.getMuSec(),Unit::MICROSECOND),Unit::SECOND,-1).get_mpf_t(),
       next_dt.getMuSec().get_mpz_t());
    */
    
    if (doAbort) {
      // ORSA_DEBUG("OUT//1");
      ret_val = false;
      break;
    }
    
    if (next_dt == zeroTime) {
      // ORSA_DEBUG("OUT//2");
      ret_val = true;
      break;
    }
    
    call_dt = next_dt;
    
    /* ORSA_DEBUG("call timestep: %Ff [day]     = %Ff",
       FromUnits(FromUnits(call_dt.getMuSec(),Unit::MICROSECOND),Unit::DAY,-1).get_mpf_t(),
       call_dt.asDouble().get_mpf_t());
    */
    
    // while (_t <= (stop+sampling_period)) { // one more step...
    // std::cerr << "Integrator::integrate(...) --> t: " << FromUnits(FromUnits(_t.getMuSec(),Unit::MICROSECOND),Unit::DAY,-1) << " [day]" << std::endl;
    /* 
       ORSA_DEBUG("Integrator::integrate(...) --> t: %Ff [day]     t: %Ff",
       FromUnits(FromUnits(t.getMuSec(),Unit::MICROSECOND),Unit::DAY,-1).get_mpf_t(),
       t.asDouble().get_mpf_t());
    */
    // if (!(step(bg,_t,dt))) {
    if (!(step(bg,t,call_dt,next_dt))) {
      ORSA_ERROR("problem encountered in step(...) call");
      // ORSA_DEBUG("OUT//3");
      ret_val = false;
      break;
    }
    
    if (0) {
      // debug
      BodyGroup::BodyList::const_iterator _b_it = bg->getBodyList().begin();
      while (_b_it != bg->getBodyList().end()) { 
	orsa::BodyGroup::BodyInterval * _b_interval = bg->getBodyInterval((*_b_it).get());
	orsa::BodyGroup::BodyInterval::DataType & _b_interval_data = _b_interval->getData();
	orsa::BodyGroup::BodyInterval::DataType::iterator _b_interval_data_it = _b_interval_data.begin();
	unsigned int counter=0;
	while (_b_interval_data_it != _b_interval_data.end()) {
	  ++counter;
	  ORSA_DEBUG("checking interval [%05i/%05i] [%s] time: %.6Ff",
		     counter,
		     (*_b_interval).size(),
		     (*_b_it)->getName().c_str(),
		     (*_b_interval_data_it).time.getRef().asDouble().get_mpf_t());
	  ++_b_interval_data_it;
	}
	++_b_it;	
      }
    }
    
    /* 
       ORSA_DEBUG("next timestep: %Ff [day]",
       FromUnits(FromUnits(next_dt.getMuSec(),Unit::MICROSECOND),Unit::DAY,-1).get_mpf_t());
    */
    
    // next_dt = call_dt;
    
    if (!lastCallRejected()) {
      
      singleStepDone(bg,t,call_dt,next_dt);
      
      t += call_dt;
      
      stat->insert(call_dt.asDouble());
      
      // debug
      /* ORSA_DEBUG("t: %Ff [day]   E: %24.16Fe",
	 orsa::FromUnits(t.asDouble(),orsa::Unit::DAY,-1).get_mpf_t(),
	 bg->totalEnergy(t).get_mpf_t());
      */
      
      /* 
	 ORSA_DEBUG("successful dt: %Ff +/- %Ff [day]",
	 FromUnits(stat->average(),Unit::DAY,-1).get_mpf_t(),
	 FromUnits(stat->averageError(),Unit::DAY,-1).get_mpf_t());
      */
      //
      
#warning "restore this functionality, and also introduce a new one to limit local storage..."
      if (0) if (progressiveCleaningSteps > 0) {
	if ((stat->entries() % progressiveCleaningSteps) == 0) {
	  
	  // ORSA_DEBUG("progressive cleaning...");
	  
	  // ORSA_DEBUG("remove all tmp TRVs...");
	  BodyGroup::BodyList::const_iterator _b_it = bg->getBodyList().begin();
	  while (_b_it != bg->getBodyList().end()) { 
	    /* 
	       if ((*_b_it)->getBodyPosVelCallback() != 0) {
	       // ORSA_DEBUG("skipping body [%s]",(*_b_it)->getName().c_str());
	       ++_b_it;
	       continue;
	       }
	    */
	    //
	    if (!((*_b_it)->getInitialConditions().translational->dynamic())) { 
	      ++_b_it;
	      continue;
	    }
	    
	    // orsa::Interval<BodyGroup::TRV> * _b_interval = bg->getBodyInterval((*_b_it).get());
	    orsa::BodyGroup::BodyInterval * _b_interval = bg->getBodyInterval((*_b_it).get());
	    // orsa::Interval<BodyGroup::TRV>::DataType & _b_interval_data = _b_interval->getData();
	    orsa::BodyGroup::BodyInterval::DataType & _b_interval_data = _b_interval->getData();
	    //
	    // orsa::Interval<BodyGroup::TRV>::DataType::iterator _b_interval_data_it = _b_interval_data.begin();
	    orsa::BodyGroup::BodyInterval::DataType::iterator _b_interval_data_it = _b_interval_data.begin();
	    while (_b_interval_data_it != _b_interval_data.end()) {
	      
	      /* 
		 ORSA_DEBUG("testing: body [%s]   t: %Ff   [tmp: %i]   this: %x   end: %x",
		 (*_b_it)->getName().c_str(),
		 (*_b_interval_data_it).time.getRef().asDouble().get_mpf_t(),
		 (*_b_interval_data_it).tmp,
		 (&(*_b_interval_data_it)),
		 (&(*(_b_interval_data.end()))));
	      */
	      
	      if ((*_b_interval_data_it).tmp==true) {
		_b_interval_data_it = _b_interval_data.erase(_b_interval_data_it);
		// ORSA_DEBUG("removed...");
	      } else {
		++_b_interval_data_it;
	      }
	    }     
	    // IMPORTANT!
	    _b_interval->update();
	    
	    ++_b_it;
	  }
	  
	}
      }    
      //
      rs->insert(call_dt.asDouble());
      if (rs->isFull()) {
	/* 
	   ORSA_DEBUG("running average dt: %Ff +/- %Ff [day]",
	   FromUnits(rs->average(),Unit::DAY,-1).get_mpf_t(),
	   FromUnits(rs->averageError(),Unit::DAY,-1).get_mpf_t());
	*/
	//
	/* ORSA_DEBUG("radt: %Ff %Ff %Ff",
	   FromUnits(FromUnits(t.getMuSec(),Unit::MICROSECOND),Unit::DAY,-1).get_mpf_t(),
	   FromUnits(rs->average(),Unit::SECOND,-1).get_mpf_t(),
	   FromUnits(rs->averageError(),Unit::SECOND,-1).get_mpf_t());
	*/
	/* 
	   {
	   // debug
	   BodyGroup::BodyList::iterator _b_it = bg->getBodyList().begin();
	   while (_b_it != bg->getBodyList().end()) {
	   if ((*_b_it)->getBodyPosVelCallback() != 0) {
	   ++_b_it;
	   continue;
	   }
	   ORSA_DEBUG("b: [%s]   interval size: %i",
	   (*_b_it)->getName().c_str(),
	   bg->getBodyInterval((*_b_it).get())->size());
	   ++_b_it;
	   }
	   }	
	*/
	
      }
    } else {
      // last call was rejected...
      // ORSA_DEBUG("rej...");
    }
    
    if ((t+next_dt-tEnd)*sign.getRef() > zeroTime) {
      next_dt = tEnd - t;
      if (next_dt == zeroTime) {
	// out of here!
	// ORSA_DEBUG("OUT//4");
	break;
      }
    } else {
      orsa::Time eventTime;
      BodyGroup::BodyList::const_iterator bl_it = bg->getBodyList().begin();
      while (bl_it != bg->getBodyList().end()) { 
	if ((*bl_it)->propulsion.get()) {
	  eventTime = t;
	  if ((*bl_it)->propulsion->nextEventTime(eventTime,sign.getRef())) {
	    if ((t+next_dt-eventTime)*sign.getRef() > zeroTime) {
	      next_dt = eventTime - t;
	      //
	      // must call reset to allow step rejection
	      reset();
	    }
	  }
	}
	++bl_it;
      }
    }
    
  }
  
  // remove all tmp TRVs
  if (1) {
    // ORSA_DEBUG("remove all tmp TRVs...");
    BodyGroup::BodyList::const_iterator _b_it = bg->getBodyList().begin();
    while (_b_it != bg->getBodyList().end()) { 
      /* 
	 if ((*_b_it)->getBodyPosVelCallback() != 0) {
	 // ORSA_DEBUG("skipping body [%s]",(*_b_it)->getName().c_str());
	 ++_b_it;
	 continue;
	 }
      */
      //
      if (!((*_b_it)->getInitialConditions().translational->dynamic())) { 
	++_b_it;
	continue;
      }
      
      /* 
	 orsa::Interval<BodyGroup::TRV> * _b_interval = bg->getBodyInterval((*_b_it).get());
	 orsa::Interval<BodyGroup::TRV>::DataType & _b_interval_data = _b_interval->getData();
	 //
	 orsa::Interval<BodyGroup::TRV>::DataType::iterator _b_interval_data_it = _b_interval_data.begin();
      */
      //
      orsa::BodyGroup::BodyInterval * _b_interval = bg->getBodyInterval((*_b_it).get());
      orsa::BodyGroup::BodyInterval::DataType & _b_interval_data = _b_interval->getData();
      orsa::BodyGroup::BodyInterval::DataType::iterator _b_interval_data_it = _b_interval_data.begin();
      //
      while (_b_interval_data_it != _b_interval_data.end()) {
	/* 
	   ORSA_DEBUG("testing: body [%s]   t: %Ff   [tmp: %i]   this: %x   end: %x",
	   (*_b_it)->getName().c_str(),
	   (*_b_interval_data_it).t.asDouble().get_mpf_t(),
	   (*_b_interval_data_it).tmp,
	   (&(*_b_interval_data_it)),
	   (&(*(_b_interval_data.end()))));
	*/
	if ((*_b_interval_data_it).tmp==true) {
	  _b_interval_data_it = _b_interval_data.erase(_b_interval_data_it);
	  // ORSA_DEBUG("removed...");
	} else {
	  ++_b_interval_data_it;
	}
      }     
      // IMPORTANT!
      // ORSA_DEBUG("calling update...");
      _b_interval->update();
      // ORSA_DEBUG("done...");
      
      ++_b_it;
    }
  }
  
  /* 
     if (1) {
     // debug
     BodyGroup::BodyList::iterator _b_it = bg->getBodyList().begin();
     while (_b_it != bg->getBodyList().end()) { 
     orsa::BodyGroup::BodyInterval * _b_interval = bg->getBodyInterval((*_b_it).get());
     orsa::BodyGroup::BodyInterval::DataType & _b_interval_data = _b_interval->getData();
     orsa::BodyGroup::BodyInterval::DataType::iterator _b_interval_data_it = _b_interval_data.begin();
     unsigned int counter=0;
     while (_b_interval_data_it != _b_interval_data.end()) {
     
     ++counter;
     
     if ((*_b_interval_data_it).rotational->dynamic()) {
     ORSA_DEBUG("[%05i/%05i] [%s] dyn: %i time: %.6Ff phi: %Ff theta: %Ff psi: %Ff",
     counter,
     (*_b_interval).size(),
     (*_b_it)->getName().c_str(),
     (*_b_interval_data_it).rotational->dynamic(),
     (*_b_interval_data_it).time.getRef().asDouble().get_mpf_t(),
     (*_b_interval_data_it).rotational->getPhi().get_mpf_t(),
     (*_b_interval_data_it).rotational->getTheta().get_mpf_t(),
     (*_b_interval_data_it).rotational->getPsi().get_mpf_t());
     } 
     
     ++_b_interval_data_it;
     }
     ++_b_it;	
     }
     }
  */
  
  return ret_val;
}
