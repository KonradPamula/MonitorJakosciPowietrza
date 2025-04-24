#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QVariantList>
#include <QVariantMap>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QDateTime>

/**
 * @brief Klasa główna aplikacji do monitorowania jakości powietrza.
 *
 * Klasa MainWindow zarządza komunikacją z API GIOŚ, przetwarza dane pomiarowe,
 * obsługuje lokalną bazę danych oraz aktualizuje interfejs użytkownika poprzez sygnały.
 * Umożliwia pobieranie danych o stacjach, czujnikach, pomiarach i indeksie jakości powietrza,
 * a także zapisywanie i wczytywanie danych historycznych.
 */
class MainWindow : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Konstruktor klasy MainWindow.
     * @param parent Wskaźnik na obiekt nadrzędny (domyślnie nullptr).
     */
    explicit MainWindow(QObject *parent = nullptr);

    /**
     * @brief Destruktor klasy MainWindow.
     */
    ~MainWindow();

    /**
     * @brief Wyszukuje stacje pomiarowe na podstawie podanego tekstu.
     * @param searchText Tekst do wyszukiwania (nazwa miejscowości).
     */
    Q_INVOKABLE void searchStations(const QString& searchText);

    /**
     * @brief Wyświetla wszystkie dostępne stacje pomiarowe.
     */
    Q_INVOKABLE void showAllStations();

    /**
     * @brief Obsługuje wybór stacji pomiarowej przez użytkownika.
     * @param stationId Identyfikator wybranej stacji.
     */
    Q_INVOKABLE void stationSelected(int stationId);

    /**
     * @brief Obsługuje wybór czujnika dla wybranej stacji.
     * @param sensorId Identyfikator wybranego czujnika.
     */
    Q_INVOKABLE void sensorSelected(int sensorId);

    /**
     * @brief Zapisuje aktualne pomiary do lokalnej bazy danych.
     */
    Q_INVOKABLE void saveMeasurementsToDatabase();

    /**
     * @brief Zapisuje aktualny indeks jakości powietrza do lokalnej bazy danych.
     */
    Q_INVOKABLE void saveAirQualityToDatabase();

    /**
     * @brief Sprawdza, czy istnieją zapisane dane historyczne dla danej stacji i czujnika.
     * @param stationId Identyfikator stacji.
     * @param sensorId Identyfikator czujnika (domyślnie -1, jeśli sprawdzamy tylko stację).
     * @return Zwraca true, jeśli dane historyczne istnieją, w przeciwnym razie false.
     */
    Q_INVOKABLE bool hasHistoricalData(int stationId, int sensorId = -1);

    /**
     * @brief Wczytuje zapisane historyczne pomiary dla danego czujnika.
     * @param sensorId Identyfikator czujnika.
     */
    Q_INVOKABLE void loadHistoricalMeasurements(int sensorId);

    /**
     * @brief Wczytuje zapisany historyczny indeks jakości powietrza dla danej stacji.
     * @param stationId Identyfikator stacji.
     */
    Q_INVOKABLE void loadHistoricalAirQuality(int stationId);

    /**
     * @brief Przełącza źródło danych między danymi bieżącymi a historycznymi.
     * @param useHistorical True, jeśli mają być użyte dane historyczne, false dla danych bieżących.
     */
    Q_INVOKABLE void toggleDataSource(bool useHistorical);

    /**
     * @brief Analizuje aktualne pomiary i zwraca wyniki statystyczne.
     * @return Zwraca QVariantMap z wynikami analizy (średnia, mediana, min, max, liczba pomiarów).
     */
    Q_INVOKABLE QVariantMap analyzeMeasurements();

signals:
    /**
     * @brief Sygnał emitowany, gdy lista stacji wymaga aktualizacji.
     * @param stations Lista stacji w formacie QVariantList.
     */
    void stationsUpdateRequested(const QVariantList& stations);

    /**
     * @brief Sygnał emitowany, gdy informacje o stacji wymagają aktualizacji.
     * @param info Informacje o stacji w formacie HTML.
     */
    void stationInfoUpdateRequested(const QString& info);

    /**
     * @brief Sygnał emitowany, gdy lista czujników wymaga aktualizacji.
     * @param sensors Lista czujników w formacie QVariantList.
     */
    void sensorsUpdateRequested(const QVariantList& sensors);

    /**
     * @brief Sygnał emitowany, gdy pomiary wymagają aktualizacji.
     * @param key Klucz parametru (np. NO2).
     * @param values Lista pomiarów w formacie QVariantList.
     */
    void measurementsUpdateRequested(const QString& key, const QVariantList& values);

    /**
     * @brief Sygnał emitowany, gdy indeks jakości powietrza wymaga aktualizacji.
     * @param text Tekst opisujący indeks (np. "Dobry").
     * @param color Kolor reprezentujący poziom jakości (np. "green").
     */
    void airQualityUpdateRequested(const QString& text, const QString& color);

    /**
     * @brief Sygnał emitowany, gdy zmienia się dostępność danych historycznych.
     * @param available True, jeśli dane historyczne są dostępne, w przeciwnym razie false.
     */
    void historicalDataAvailableChanged(bool available);

    /**
     * @brief Sygnał emitowany, gdy wyniki analizy pomiarów są gotowe.
     * @param analysis Wyniki analizy w formacie QVariantMap.
     */
    void analysisUpdateRequested(const QVariantMap& analysis);

private slots:
    /**
     * @brief Slot wywoływany po otrzymaniu danych o stacjach z API.
     */
    void onStationsReceived();

    /**
     * @brief Slot wywoływany po otrzymaniu danych o czujnikach z API.
     */
    void onSensorsReceived();

    /**
     * @brief Slot wywoływany po otrzymaniu danych pomiarowych z API.
     */
    void onMeasurementsReceived();

    /**
     * @brief Slot wywoływany po otrzymaniu indeksu jakości powietrza z API.
     */
    void onAirQualityIndexReceived();

private:
    QNetworkAccessManager* networkManager; ///< Menedżer sieciowy do wykonywania żądań HTTP.

    // API endpoints
    const QString API_BASE_URL = "https://api.gios.gov.pl/pjp-api/rest/"; ///< Bazowy URL API GIOŚ.
    const QString API_STATIONS_ENDPOINT = "station/findAll"; ///< Endpoint do pobierania wszystkich stacji.
    const QString API_SENSORS_ENDPOINT = "station/sensors/"; ///< Endpoint do pobierania czujników dla stacji.
    const QString API_MEASUREMENTS_ENDPOINT = "data/getData/"; ///< Endpoint do pobierania danych pomiarowych.
    const QString API_AIR_QUALITY_ENDPOINT = "aqindex/getIndex/"; ///< Endpoint do pobierania indeksu jakości powietrza.

    // Data structures
    QJsonArray allStations; ///< Tablica JSON ze wszystkimi stacjami.
    QMap<int, QJsonObject> stationsMap; ///< Mapa przechowująca stacje według ich ID.
    QMap<int, QJsonObject> sensorsMap; ///< Mapa przechowująca czujniki według ich ID.

    // Aktualne dane
    int currentStationId = -1; ///< ID aktualnie wybranej stacji.
    int currentSensorId = -1; ///< ID aktualnie wybranego czujnika.
    QString currentMeasurementKey; ///< Klucz aktualnego parametru pomiarowego (np. NO2).
    QVariantList currentMeasurements; ///< Lista aktualnych pomiarów.
    QJsonObject currentAirQuality; ///< Obiekt JSON z aktualnym indeksem jakości powietrza.

    /**
     * @brief Pobiera ścieżkę do lokalnej bazy danych.
     * @return Ścieżka do katalogu bazy danych.
     */
    QString getDatabasePath();

    /**
     * @brief Generuje ścieżkę do pliku z pomiarami dla danej stacji i czujnika.
     * @param stationId Identyfikator stacji.
     * @param sensorId Identyfikator czujnika.
     * @return Ścieżka do pliku JSON z pomiarami.
     */
    QString getMeasurementsFilePath(int stationId, int sensorId);

    /**
     * @brief Generuje ścieżkę do pliku z indeksem jakości powietrza dla danej stacji.
     * @param stationId Identyfikator stacji.
     * @return Ścieżka do pliku JSON z danymi jakości powietrza.
     */
    QString getAirQualityFilePath(int stationId);

    /**
     * @brief Zapisuje dokument JSON do pliku.
     * @param filePath Ścieżka do pliku.
     * @param jsonDoc Dokument JSON do zapisania.
     * @return True, jeśli zapis się powiódł, w przeciwnym razie false.
     */
    bool saveJsonToFile(const QString& filePath, const QJsonDocument& jsonDoc);

    /**
     * @brief Wczytuje dokument JSON z pliku.
     * @param filePath Ścieżka do pliku.
     * @return Dokument JSON lub pusty dokument w przypadku błędu.
     */
    QJsonDocument loadJsonFromFile(const QString& filePath);

    /**
     * @brief Pobiera dane o stacjach z API.
     */
    void fetchStations();

    /**
     * @brief Pobiera dane o czujnikach dla danej stacji z API.
     * @param stationId Identyfikator stacji.
     */
    void fetchSensors(int stationId);

    /**
     * @brief Pobiera pomiary dla danego czujnika z API.
     * @param sensorId Identyfikator czujnika.
     */
    void fetchMeasurements(int sensorId);

    /**
     * @brief Pobiera indeks jakości powietrza dla danej stacji z API.
     * @param stationId Identyfikator stacji.
     */
    void fetchAirQualityIndex(int stationId);

    /**
     * @brief Wyświetla listę stacji w interfejsie użytkownika.
     * @param stations Tablica JSON z danymi stacji.
     */
    void displayStations(const QJsonArray& stations);

    /**
     * @brief Generuje informacje o stacji w formacie HTML.
     * @param station Obiekt JSON z danymi stacji.
     * @return Tekst HTML z informacjami o stacji.
     */
    QString generateStationInfo(const QJsonObject& station);
};

#endif // MAINWINDOW_H
