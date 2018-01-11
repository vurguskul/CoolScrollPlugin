##TARGET = CoolScroll
##TEMPLATE = lib

DEFINES += COOLSCROLL_LIBRARY

# CoolScroll files

SOURCES += coolscrollplugin.cpp \
    coolscrollbar.cpp \
    coolscrollbarsettings.cpp \
    settingspage.cpp \
    settingsdialog.cpp

HEADERS += coolscrollplugin.h\
        coolscroll_global.h\
        coolscrollconstants.h \
    coolscrollbar.h \
    coolscrollbarsettings.h \
    settingspage.h \
    settingsdialog.h

# Qt Creator linking

## Either set the IDE_SOURCE_TREE when running qmake,
## or set the QTC_SOURCE environment variable, to override the default setting
isEmpty(IDE_SOURCE_TREE): IDE_SOURCE_TREE = $$(QTC_SOURCE)
isEmpty(IDE_SOURCE_TREE): IDE_SOURCE_TREE = "../qt-creator"

## Either set the IDE_BUILD_TREE when running qmake,
## or set the QTC_BUILD environment variable, to override the default setting
isEmpty(IDE_BUILD_TREE): IDE_BUILD_TREE = $$(QTC_BUILD)
isEmpty(IDE_BUILD_TREE): IDE_BUILD_TREE = "../qt-creator"

##include($$QTCREATOR_SOURCES/src/qtcreatorplugin.pri)
#include($$QTCREATOR_SOURCES/src/plugins/coreplugin/coreplugin.pri)
#include($$QTCREATOR_SOURCES/src/plugins/texteditor/texteditor.pri)

FORMS += \
    settingsdialog.ui

QTC_PLUGIN_NAME = CoolScrollPlugin
QTC_LIB_DEPENDS += \
# nothing here at this time

QTC_PLUGIN_DEPENDS += \
	coreplugin

QTC_PLUGIN_RECOMMENDS += \
	# optional plugin dependencies. nothing here at this time

###### End _dependencies.pri contents ######

QMAKE_CXXFLAGS += -std=c++14

include($$IDE_SOURCE_TREE/src/qtcreatorplugin.pri)

