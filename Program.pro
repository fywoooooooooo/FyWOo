QT += core gui charts sql network widgets datavisualization graphs graphswidgets

CONFIG += c++17 visualization3d

# ???????
TARGET = Program
TEMPLATE = app

# ??????
INCLUDEPATH += $$PWD/../include

# Qt ??????????????? Qt ?????????????
LIBS += -L$$[QT_INSTALL_LIBS]

# ????????
SOURCES += \
    main.cpp \
    src/code/mainwindow.cpp \
    src/code/ui/cpupage.cpp \
    src/code/ui/memorypage.cpp \
    src/code/ui/diskpage.cpp \
    src/code/ui/networkpage.cpp \
    src/code/ui/processpage.cpp \
    src/code/ui/settingswidget.cpp \
    src/code/ui/gpupage.cpp \
    src/code/ui/infopanel.cpp \
    src/code/ui/analysispage.cpp \
    src/code/ui/processselectiondialog.cpp \
    src/code/chart/chartwidget.cpp \
    src/code/monitor/cpumonitor.cpp \
    src/code/monitor/memorymonitor.cpp \
    src/code/monitor/diskmonitor.cpp \
    src/code/monitor/networkmonitor.cpp \
    src/code/monitor/processmonitor.cpp \
    src/code/monitor/sampler.cpp \
    src/code/storage/datastorage.cpp \
    src/code/storage/exporter.cpp \
    src/code/analysis/anomalydetector.cpp \
    src/code/analysis/performanceanalyzer.cpp \
    src/code/model/modelinterface.cpp

# ????????
HEADERS += \
    src/include/mainwindow.h \
    src/include/ui/cpupage.h \
    src/include/ui/memorypage.h \
    src/include/ui/diskpage.h \
    src/include/ui/networkpage.h \
    src/include/ui/processpage.h \
    src/include/ui/settingswidget.h \
    src/include/ui/gpupage.h \
    src/include/ui/infopanel.h \
    src/include/ui/analysispage.h \
    src/include/ui/processselectiondialog.h \
    src/include/chart/chartwidget.h \
    src/include/monitor/cpumonitor.h \
    src/include/monitor/memorymonitor.h \
    src/include/monitor/diskmonitor.h \
    src/include/monitor/networkmonitor.h \
    src/include/monitor/processmonitor.h \
    src/include/monitor/sampler.h \
    src/include/storage/datastorage.h \
    src/include/storage/exporter.h \
    src/include/analysis/anomalydetector.h \
    src/include/analysis/performanceanalyzer.h \
    src/include/model/modelinterface.h

# ??????????
# ????3D????????: CONFIG+=visualization3d
CONFIG(visualization3d) {
    message("Enabling 3D Visualization Module")
    # Qt Graphs模块已在主配置中添加
    DEFINES += USE_VISUALIZATION3D
    
    # ?????legacy????????
    SOURCES += src/legacy/visualization3d/Vis3DPage.cpp
    HEADERS += src/legacy/visualization3d/Vis3DPage.h
    
    # ???????????????Z??????????????????
    # SOURCES += src/extensions/visualization3d/Vis3DPage.cpp
    # HEADERS += src/extensions/visualization3d/Vis3DPage.h
}

# ??????????????: CONFIG+=distributed
CONFIG(distributed) {
    message("Enabling Distributed Monitoring Module")
    DEFINES += USE_DISTRIBUTED
    
    # ?????legacy????????
    SOURCES += src/legacy/distributed/code/distributedmonitor.cpp \
               src/legacy/distributed/code/monitoringintegration.cpp
    HEADERS += src/legacy/distributed/include/distributedmonitor.h \
               src/legacy/distributed/include/monitoringintegration.h
               
    # ???????????????Z??????????????????
    # SOURCES += src/extensions/distributed/*.cpp
    # HEADERS += src/extensions/distributed/*.h
}

# ????????????????: CONFIG+=blockchain
CONFIG(blockchain) {
    message("Enabling Blockchain Storage Module")
    DEFINES += USE_BLOCKCHAIN
    
    # ?????legacy????????
    SOURCES += src/legacy/blockchain/blockchain.cpp
    HEADERS += src/legacy/blockchain/blockchain.h
    
    # ???????????????Z??????????????????
    # SOURCES += src/extensions/blockchain/*.cpp
    # HEADERS += src/extensions/blockchain/*.h
}

# Windows ???????
win32 {
    LIBS += -lpdh -lpsapi -liphlpapi -lws2_32
    DEFINES += WIN32_LEAN_AND_MEAN NOMINMAX _WIN32_WINNT=0x0601
    
    # MSVC ???????
    msvc {
        # ??????????????????
        QMAKE_CXXFLAGS += /wd4996
        
        # ??????????????
        QMAKE_CXXFLAGS_RELEASE += /MT
        QMAKE_CXXFLAGS_DEBUG += /MTd
    }
    
    # MinGW ???????
    mingw {
        QMAKE_CXXFLAGS += -Wno-deprecated-declarations
        QMAKE_CXXFLAGS += -Wno-attributes
    }
}

# ??????????????????????????
OBJECTS_DIR = build/obj
MOC_DIR = build/moc
RCC_DIR = build/rcc
UI_DIR = build/ui

# ?????????????????????
CONFIG(debug, debug|release) {
    DESTDIR = build/debug
} else {
    DESTDIR = build/release
}

# ??????
RESOURCES += icons.qrc

# ??????for deployment
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target