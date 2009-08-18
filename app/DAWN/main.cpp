#include "DAWN.h"

int main(int argc, char ** argv) {
  
  // QApplication app(argc, argv);
  
  // orsaQt::Debug::instance()->initTimer();
  //
  orsa::Debug::instance()->initTimer();
  
  osg::ref_ptr<orsa::BodyGroup> bg = run();
  
  if (!bg.get()) {
    exit(0);
  }	
  
  return 0;
}