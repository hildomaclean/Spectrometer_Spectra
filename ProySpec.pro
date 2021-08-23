#-------------------------------------------------
#
# Project created by QtCreator 2014-05-28T08:25:35
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

TARGET = ProySpec
TEMPLATE = app


SOURCES += src/main.cpp\
        src/dialog.cpp \
    src/qcustomplot.cpp \
    src/thread.cpp \
    src/aboutdlg.cpp

INCLUDEPATH += headers
HEADERS  += headers/dialog.h \
    headers/qcustomplot.h \
    headers/thread.h \
    headers/aboutdlg.h

FORMS    += ui/dialog.ui \
    ui/aboutdlg.ui


INCLUDEPATH += "C:\Program Files (x86)\Ocean Optics\OmniDriverSPAM\include"
INCLUDEPATH += $$quote(C:\Program Files\Java\jdk1.8.0_91\include)
INCLUDEPATH += $$quote(C:\Program Files\Java\jdk1.8.0_91\include\win32)

#INCLUDEPATH += $$quote(C:\Program Files (x86)\Java\jdk1.7.0_55\include)
#INCLUDEPATH += $$quote(C:\Program Files (x86)\Java\jdk1.7.0_55\include\win32)


LIBS+= "C:\Program Files (x86)\Ocean Optics\OmniDriverSPAM\OOI_HOME\SPAM32.dll"
LIBS+= "C:\Program Files (x86)\Ocean Optics\OmniDriverSPAM\OOI_HOME\common32.dll"
LIBS+= "C:\Program Files (x86)\Ocean Optics\OmniDriverSPAM\OOI_HOME\OmniDriver32.dll"

RESOURCES += \
    Resources.qrc

RC_FILE = myapp.rc #for presentation icon
