TEMPLATE = app

CONFIG += qt
QT     += gui opengl

CONFIG += gmp_include gmp_lib gsl_include gsl_lib osg_include osg_lib qwt_include qwt_lib spice_include spice_lib

include(../../orsa.pri)

TARGET   = RendezvousWithVesta

INCLUDEPATH += ../../src/
DEPENDPATH  += ../../src/

UI_DIR      =  .ui/$${PLATFORM_NAME}
MOC_DIR     = .moc/$${PLATFORM_NAME}
OBJECTS_DIR = .obj/$${PLATFORM_NAME}
DESTDIR     = ../../bin/$${PLATFORM_NAME}

unix:!macx {
	LIBS += -L../../lib/$${PLATFORM_NAME} -lorsa -lorsaOSG -lorsaQt -lorsaSolarSystem -lorsaSPICE -losgViewer -losgDB -losgdb_freetype -losgText -losgUtil -losgGA -losg -lOpenThreads -lgmp -lgmpxx -losgText -losgdb_freetype -losgUtil -losg
}

macx {
        LIBS += -L../../lib/$${PLATFORM_NAME} -lorsa -lorsaOSG -lorsaQt -lorsaSolarSystem -lorsaSPICE -losgViewer -losgdb_freetype /opt/local/lib/libfreetype.a -losgText -losgUtil -losgGA -losg -lOpenThreads -losgDB -losg
}

win32 {
        LIBS += -L../../lib/$${PLATFORM_NAME} -lorsa -lorsaOSG -lorsaQt -lorsaSolarSystem -lorsaSPICE -losg -losgViewer -losgGA
}

HEADERS += RendezvousWithVesta.h   RendezvousWithVestaVersion.h   mainThread.h   multiminPhase.h   vestaViz.h   plot.h vesta.h gaskell.h
SOURCES += RendezvousWithVesta.cpp RendezvousWithVestaVersion.cpp mainThread.cpp multiminPhase.cpp plot.cpp     main.cpp

# Kleopatra stuff
#HEADERS += kleopatrashape.h
#SOURCES += 

unix:!macx { BUILD_COUNTER_BIN = ./BuildCounter.$${PLATFORM_NAME} }
macx:      { BUILD_COUNTER_BIN = ./BuildCounter.$${PLATFORM_NAME}.app/Contents/MacOS/BuildCounter.$${PLATFORM_NAME} }

counter.depends  = $${SOURCES} $${HEADERS}
counter.target   = BuildCounter.h
counter.commands = $${BUILD_COUNTER_BIN}

QMAKE_EXTRA_TARGETS += counter
