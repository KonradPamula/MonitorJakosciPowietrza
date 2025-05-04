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
 * Zarządza komunikacją z API GIOŚ, przetwarza dane pomiarowe, obsługuje lokalną bazę danych
 * oraz aktualizuje interfejs użytkownika poprzez sygnały. Umożliwia pobieranie danych o stacjach,
 * czujnikach, pomiarach i indeksie jakości powietrza, a także zapisywanie i wczytywanie danych historycznych.
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
     * @brief Wyszukuje stacje pomiarowe na podstawie tekstu.
     * @param searchText Tekst wyszukiwania (nazwa miejscowości).
     */
    Q_INVOKABLE void searchStations(const QString& searchText);

    /**
     * @brief Wyświetla wszystkie dostępne stacje pomiarowe.
     */
    Q_INVOKABLE void showAllStations();

    /**
     * @brief Obsługuje wybór stacji przez użytkownika.
     * @param stationId Identyfikator wybranej stacji.
     */
    Q_INVOKABLE void stationSelected(int stationId);

    /**
     * @brief Obsługuje wybór czujnika dla wybranej stacji.
     * @param sensorId Identyfikator wybranego czujnika.
     */
    Q_INVOKABLE void sensorSelected(int sensorId);

    /**
     * @brief Zapisuje bieżące pomiary do lokalnej bazy danych.
     */
    Q_INVOKABLE void saveMeasurementsToDatabase();

    /**
     * @brief Zapisuje bieżący indeks jakości powietrza do lokalnej bazy danych.
     */
    Q_INVOKABLE void saveAirQualityToDatabase();

    /**
     * @brief Sprawdza, czy istnieją dane historyczne dla stacji lub czujnika.
     * @param stationId Identyfikator stacji.
     * @param sensorId Identyfikator czujnika (domyślnie -1 dla indeksu jakości powietrza).
     * @return True, jeśli dane historyczne istnieją, w przeciwnym razie false.
     */
    Q_INVOKABLE bool hasHistoricalData(int stationId, int sensorId = -1);

    /**
     * @brief Wczytuje historyczne pomiary dla czujnika.
     * @param sensorId Identyfikator czujnika.
     */
    Q_INVOKABLE void loadHistoricalMeasurements(int sensorId);

    /**
     * @brief Wczytuje historyczny indeks jakości powietrza dla stacji.
     * @param stationId Identyfikator stacji.
     */
    Q_INVOKABLE void loadHistoricalAirQuality(int stationId);

    /**
     * @brief Przełącza między danymi bieżącymi a historycznymi.
     * @param useHistorical True dla danych historycznych, false dla bieżących.
     */
    Q_INVOKABLE void toggleDataSource(bool useHistorical);

    /**
     * @brief Analizuje pomiary i zwraca statystyki.
     * @return QVariantMap z wynikami analizy (średnia, mediana, min, max, liczba pomiarów).
     */
    Q_INVOKABLE QVariantMap analyzeMeasurements();

signals:
    /**
     * @brief Emitowany, gdy lista stacji wymaga aktualizacji.
     * @param stations Lista stacji w formacie QVariantList.
     */
    void stationsUpdateRequested(const QVariantList& stations);

    /**
     * @brief Emitowany, gdy informacje o stacji wymagają aktualizacji.
     * @param info Informacje o stacji w formacie HTML.
     */
    void stationInfoUpdateRequested(const QString& info);

    /**
     * @brief Emitowany, gdy lista czujników wymaga aktualizacji.
     * @param sensors Lista czujników w formacie QVariantList.
     */
    void sensorsUpdateRequested(const QVariantList& sensors);

    /**
     * @brief Emitowany, gdy pomiary wymagają aktualizacji.
     * @param key Klucz parametru (np. NO2).
     * @param values Lista pomiarów w formacie QVariantList.
     */
    void measurementsUpdateRequested(const QString& key, const QVariantList& values);

    /**
     * @brief Emitowany, gdy indeks jakości powietrza wymaga aktualizacji.
     * @param text Tekst opisujący indeks (np. "Dobry").
     * @param color Kolor reprezentujący poziom jakości (np. "green").
     */
    void airQualityUpdateRequested(const QString& text, const QString& color);

    /**
     * @brief Emitowany, gdy zmienia się dostępność danych historycznych.
     * @param available True, jeśli dane historyczne są dostępne, w przeciwnym razie false.
     */
    void historicalDataAvailableChanged(bool available);

    /**
     * @brief Emitowany, gdy wyniki analizy są gotowe.
     * @param analysis Wyniki analizy w formacie QVariantMap.
     */
    void analysisUpdateRequested(const QVariantMap& analysis);

private slots:
    /**
     * @brief Obsługuje odpowiedź API z danymi o stacjach.
     */
    void onStationsReceived();

    /**
     * @brief Obsługuje odpowiedź API z danymi o czujnikach.
     */
    void onSensorsReceived();

    /**
     * @brief Obsługuje odpowiedź API z danymi pomiarowymi.
     */
    void onMeasurementsReceived();

    /**
     * @brief Obsługuje odpowiedź API z indeksem jakości powietrza.
     */
    void onAirQualityIndexReceived();

private:
    /// @brief Menedżer sieciowy do żądań HTTP.
    QNetworkAccessManager* networkManager;

    /// @brief Bazowy URL API GIOŚ.
    const QString API_BASE_URL = "https://api.gios.gov.pl/pjp-api/rest/";
    /// @brief Endpoint do pobierania wszystkich stacji.
    const QString API_STATIONS_ENDPOINT = "station/findAll";
    /// @brief Endpoint do pobierania czujników dla stacji.
    const QString API_SENSORS_ENDPOINT = "station/sensors/";
    /// @brief Endpoint do pobierania danych pomiarowych.
    const QString API_MEASUREMENTS_ENDPOINT = "data/getData/";
    /// @brief Endpoint do pobierania indeksu jakości powietrza.
    const QString API_AIR_QUALITY_ENDPOINT = "aqindex/getIndex/";

    /// @brief Tablica JSON ze wszystkimi stacjami.
    QJsonArray allStations;
    /// @brief Mapa stacji według ich ID.
    QMap<int, QJsonObject> stationsMap;
    /// @brief Mapa czujników według ich ID.
    QMap<int, QJsonObject> sensorsMap;

    /// @brief ID aktualnie wybranej stacji.
    int currentStationId = -1;
    /// @brief ID aktualnie wybranego czujnika.
    int currentSensorId = -1;
    /// @brief Klucz bieżącego parametru pomiarowego (np. NO2).
    QString currentMeasurementKey;
    /// @brief Lista bieżących pomiarów.
    QVariantList currentMeasurements;
    /// @brief Obiekt JSON z bieżącym indeksem jakości powietrza.
    QJsonObject currentAirQuality;

    /**
     * @brief Zwraca ścieżkę do lokalnej bazy danych.
     * @return Ścieżka do katalogu bazy danych.
     */
    QString getDatabasePath();

    /**
     * @brief Generuje ścieżkę do pliku z pomiarami.
     * @param stationId Identyfikator stacji.
     * @param sensorId Identyfikator czujnika.
     * @return Ścieżka do pliku JSON z pomiarami.
     */
    QString getMeasurementsFilePath(int stationId, int sensorId);

    /**
     * @brief Generuje ścieżkę do pliku z indeksem jakości powietrza.
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
     * @brief Pobiera dane o czujnikach dla stacji z API.
     * @param stationId Identyfikator stacji.
     */
    void fetchSensors(int stationId);

    /**
     * @brief Pobiera pomiary dla czujnika z API.
     * @param sensorId Identyfikator czujnika.
     */
    void fetchMeasurements(int sensorId);

    /**
     * @brief Pobiera indeks jakości powietrza dla stacji z API.
     * @param stationId Identyfikator stacji.
     */
    void fetchAirQualityIndex(int stationId);

    /**
     * @brief Wyświetla listę stacji w interfejsie.
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
