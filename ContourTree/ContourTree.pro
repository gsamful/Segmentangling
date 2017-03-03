QT += core
QT -= gui

CONFIG += c++11

TARGET = ContourTree
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    MergeTree.cpp \
    Grid3D.cpp

HEADERS += \
    DisjointSets.hpp \
    MergeTree.hpp \
    ScalarFunction.hpp \
    Grid3D.hpp

# Unix configuration
unix:!macx{
    INCLUDEPATH += -I /usr/local/include/
    QMAKE_CXXFLAGS += -fopenmp
    QMAKE_LFLAGS   += -fopenmp
    LIBS += -lboost_thread -lboost_filesystem -lboost_program_options -lboost_system -lboost_iostreams
}

win32{
    CONFIG += console

    INCLUDEPATH += "$$(BOOST_PATH)"
    LIBS += "-ladvapi32"
    LIBS += "-L$$(BOOST_LIB_PATH)"
}
