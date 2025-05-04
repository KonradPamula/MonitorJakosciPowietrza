#/**
# * @file project.pro
# * @brief Plik konfiguracyjny projektu Qt.
# *
# * Określa moduły Qt, pliki źródłowe, nagłówkowe, zasoby oraz ustawienia kompilacji i wdrażania.
# */

#/**
# * @brief Moduły Qt używane w projekcie.
# *
# * Włącza moduły do obsługi QML, sieci, wykresów i widżetów.
# */
QT += quick qml network charts widgets

#/**
# * @brief Włączenie standardu C++17 dla kompilatora.
# */
CONFIG += c++17

#/**
# * @brief Lista plików źródłowych projektu.
# */
SOURCES += \
    main.cpp \
    mainwindow.cpp

#/**
# * @brief Lista plików nagłówkowych projektu.
# */
HEADERS += \
    mainwindow.h

#/**
# * @brief Plik zasobów zawierający QML i inne zasoby (np. ikony).
# */
RESOURCES += qml.qrc

#/**
# * @brief Włączenie kompilatora Qt Quick dla lepszej wydajności QML.
# */
CONFIG += qtquickcompiler

#/**
# * @brief Opcjonalne włączenie debugowania QML
# */
# CONFIG += qml_debug

#/**
# * @brief Reguły wdrażania dla różnych platform.
# *
# * Określa ścieżki instalacji dla platformy QNX i innych systemów Unix (poza Androidem).
# */
qnx: target.path = /tmp/$$  {TARGET}/bin
else: unix:!android: target.path = /opt/  $${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
