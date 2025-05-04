#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "mainwindow.h"

/**
 * @brief Główna funkcja aplikacji.
 *
 * Inicjalizuje aplikację Qt, ładuje interfejs QML i łączy go z logiką backendu (MainWindow).
 * @param argc Liczba argumentów wiersza poleceń.
 * @param argv Tablica argumentów wiersza poleceń.
 * @return Kod wyjścia aplikacji (0 oznacza sukces, -1 w przypadku błędu).
 */
int main(int argc, char *argv[])
{
    /// Inicjalizacja aplikacji Qt z obsługą argumentów wiersza poleceń.
    QApplication app(argc, argv);

    /// Silnik QML do ładowania i renderowania interfejsu użytkownika.
    QQmlApplicationEngine engine;

    /// Obiekt backendu zarządzający logiką aplikacji.
    MainWindow mainWindow;

    /// Rejestracja obiektu MainWindow w kontekście QML, umożliwia dostęp z QML.
    engine.rootContext()->setContextProperty("mainWindow", &mainWindow);

    /// URL do głównego pliku QML w zasobach.
    const QUrl url(QStringLiteral("qrc:/main.qml"));

    /// Połączenie sygnału objectCreated z lambdą sprawdzającą poprawność utworzenia obiektu QML.
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
                         if (!obj && url == objUrl)
                             QCoreApplication::exit(-1);
                     }, Qt::QueuedConnection);

    /// Ładowanie głównego pliku QML do silnika.
    engine.load(url);

    /// Sprawdzenie, czy załadowano obiekty QML; w razie błędu zwraca -1.
    if (engine.rootObjects().isEmpty())
        return -1;

    /// Uruchomienie pętli zdarzeń aplikacji Qt.
    return app.exec();
}
