include(../../variables.pri)

TEMPLATE = lib
LANGUAGE = C++
TARGET   = freedmx

QT      += network
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG      += plugin
INCLUDEPATH += ../interfaces

win32:QMAKE_LFLAGS += -shared

target.path = $$INSTALLROOT/$$PLUGINDIR
INSTALLS   += target

TRANSLATIONS += freedmx_de_DE.ts
TRANSLATIONS += freedmx_es_ES.ts
TRANSLATIONS += freedmx_fi_FI.ts
TRANSLATIONS += freedmx_fr_FR.ts
TRANSLATIONS += freedmx_it_IT.ts
TRANSLATIONS += freedmx_nl_NL.ts
TRANSLATIONS += freedmx_cz_CZ.ts
TRANSLATIONS += freedmx_pt_BR.ts
TRANSLATIONS += freedmx_ca_ES.ts
TRANSLATIONS += freedmx_ja_JP.ts

HEADERS += ../interfaces/qlcioplugin.h
HEADERS += freedmxplugin.h \
           freedmxcontroller.h

SOURCES += ../interfaces/qlcioplugin.cpp
SOURCES += freedmxplugin.cpp \
           freedmxcontroller.cpp

# First version does not require conguration UI
#FORMS += freedmxconfigure.ui
