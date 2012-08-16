TEMPLATE = app

CONFIG += qt
QT     -= gui opengl

CONFIG += gmp_include gmp_lib gsl_include gsl_lib osg_include osg_lib spice_include spice_lib

include(../../orsa.pri)

TARGET   = MovingLayers

INCLUDEPATH += ../../src/
DEPENDPATH  += ../../src/

UI_DIR      =  .ui/$${PLATFORM_NAME}
MOC_DIR     = .moc/$${PLATFORM_NAME}
OBJECTS_DIR = .obj/$${PLATFORM_NAME}
DESTDIR     = .

unix:!macx {
	LIBS += -L../../lib/$${PLATFORM_NAME} -lorsa -lorsaSolarSystem -lorsaEssentialOSG -lorsaSPICE -lorsaPDS -lorsaUtil -lOpenThreads -lqd -L/home/tricaric/sqlite -lsqlite3 -lbsd
}

HEADERS += MovingLayers.h   MovingLayers_multifit.h   CCMD2SH.h   vesta.h  CubicChebyshevMassDistribution.h gaskell.h penalty.h
SOURCES += MovingLayers.cpp CubicChebyshevMassDistribution.cpp 


