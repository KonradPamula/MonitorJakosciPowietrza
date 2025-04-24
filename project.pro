QT += quick qml network charts widgets

CONFIG += c++17

SOURCES += \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    mainwindow.h

RESOURCES += qml.qrc

# Quick compiler for better QML performance
CONFIG += qtquickcompiler

# Optionally enable debugging of QML
# CONFIG += qml_debug

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
