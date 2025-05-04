#include "mainwindow.h"
#include <QDebug>
#include <QDateTime>
#include <algorithm>

/**
 * @brief Konstruktor klasy MainWindow.
 *
 * Inicjalizuje menedżera sieciowego i pobiera dane o stacjach z API.
 * @param parent Wskaźnik na obiekt nadrzędny (domyślnie nullptr).
 */
MainWindow::MainWindow(QObject *parent)
    : QObject(parent)
{
    networkManager = new QNetworkAccessManager(this);
    fetchStations();
}

/**
 * @brief Destruktor klasy MainWindow.
 *
 * Zwalnia zasoby (menedżer sieciowy zwalniany automatycznie jako dziecko QObject).
 */
MainWindow::~MainWindow()
{
}

/**
 * @brief Pobiera dane o wszystkich stacjach z API.
 */
void MainWindow::fetchStations()
{
    QNetworkRequest request((QUrl(API_BASE_URL + API_STATIONS_ENDPOINT)));
    QNetworkReply* reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, &MainWindow::onStationsReceived);
}

/**
 * @brief Pobiera dane o czujnikach dla danej stacji z API.
 * @param stationId Identyfikator stacji.
 */
void MainWindow::fetchSensors(int stationId)
{
    QNetworkRequest request((QUrl(API_BASE_URL + API_SENSORS_ENDPOINT + QString::number(stationId))));
    QNetworkReply* reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, &MainWindow::onSensorsReceived);
}

/**
 * @brief Pobiera pomiary dla danego czujnika z API.
 * @param sensorId Identyfikator czujnika.
 */
void MainWindow::fetchMeasurements(int sensorId)
{
    QNetworkRequest request((QUrl(API_BASE_URL + API_MEASUREMENTS_ENDPOINT + QString::number(sensorId))));
    QNetworkReply* reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, &MainWindow::onMeasurementsReceived);
}

/**
 * @brief Pobiera indeks jakości powietrza dla danej stacji z API.
 * @param stationId Identyfikator stacji.
 */
void MainWindow::fetchAirQualityIndex(int stationId)
{
    QNetworkRequest request((QUrl(API_BASE_URL + API_AIR_QUALITY_ENDPOINT + QString::number(stationId))));
    QNetworkReply* reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, &MainWindow::onAirQualityIndexReceived);
}

/**
 * @brief Obsługuje odpowiedź API z danymi o stacjach.
 */
void MainWindow::onStationsReceived()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray response = reply->readAll();
        try {
            QJsonDocument jsonDoc = QJsonDocument::fromJson(response);
            if (jsonDoc.isNull() || !jsonDoc.isArray()) {
                throw std::runtime_error("Invalid JSON array for stations");
            }
            allStations = jsonDoc.array();

            for (const QJsonValue& value : allStations) {
                QJsonObject station = value.toObject();
                int id = station["id"].toInt();
                stationsMap[id] = station;
            }

            displayStations(allStations);
        } catch (const std::exception& e) {
            qDebug() << "Exception while parsing stations JSON:" << e.what();
            emit stationsUpdateRequested(QVariantList());
        }
    } else {
        qDebug() << "Error fetching stations:" << reply->errorString();
    }
    reply->deleteLater();
}

/**
 * @brief Obsługuje odpowiedź API z danymi o czujnikach.
 */
void MainWindow::onSensorsReceived()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray response = reply->readAll();
        try {
            QJsonDocument jsonDoc = QJsonDocument::fromJson(response);
            if (jsonDoc.isNull() || !jsonDoc.isArray()) {
                throw std::runtime_error("Invalid JSON array for sensors");
            }
            QJsonArray sensors = jsonDoc.array();

            QVariantList sensorsList;

            for (const QJsonValue& value : sensors) {
                QJsonObject sensor = value.toObject();
                int id = sensor["id"].toInt();
                QString parameterName = sensor["param"].toObject()["paramName"].toString();
                QString parameterFormula = sensor["param"].toObject()["paramFormula"].toString();
                QString text = QString("%1 (%2)").arg(parameterName, parameterFormula);

                QVariantMap sensorMap;
                sensorMap["display"] = text;
                sensorMap["sensorId"] = id;
                sensorsList.append(sensorMap);

                sensorsMap[id] = sensor;
            }

            emit sensorsUpdateRequested(sensorsList);
        } catch (const std::exception& e) {
            qDebug() << "Exception while parsing sensors JSON:" << e.what();
            emit sensorsUpdateRequested(QVariantList());
        }
    } else {
        qDebug() << "Error fetching sensors:" << reply->errorString();
    }
    reply->deleteLater();
}

/**
 * @brief Obsługuje odpowiedź API z danymi pomiarowymi.
 */
void MainWindow::onMeasurementsReceived()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray response = reply->readAll();
        try {
            QJsonDocument jsonDoc = QJsonDocument::fromJson(response);
            if (jsonDoc.isNull() || !jsonDoc.isObject()) {
                throw std::runtime_error("Invalid JSON object for measurements");
            }
            QJsonObject measurements = jsonDoc.object();

            QString key = measurements["key"].toString();
            QJsonArray values = measurements["values"].toArray();

            QVariantList valuesList;

            for (const QJsonValue& value : values) {
                QJsonObject measurement = value.toObject();
                QString dateStr = measurement["date"].toString();
                QVariant valueVariant = measurement["value"].toVariant();

                QVariantMap point;
                point["date"] = dateStr;
                point["value"] = valueVariant.isNull() ? QVariant() : valueVariant.toDouble();
                valuesList.append(point);
            }

            currentMeasurementKey = key;
            currentMeasurements = valuesList;

            emit historicalDataAvailableChanged(hasHistoricalData(currentStationId, currentSensorId));
            emit measurementsUpdateRequested(key, valuesList);
        } catch (const std::exception& e) {
            qDebug() << "Exception while parsing measurements JSON:" << e.what();
            emit measurementsUpdateRequested("Error", QVariantList());
        }
    } else {
        qDebug() << "Error fetching measurements:" << reply->errorString();
    }
    reply->deleteLater();
}

/**
 * @brief Obsługuje odpowiedź API z indeksem jakości powietrza.
 */
void MainWindow::onAirQualityIndexReceived()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray response = reply->readAll();
        try {
            QJsonDocument jsonDoc = QJsonDocument::fromJson(response);
            if (jsonDoc.isNull() || !jsonDoc.isObject()) {
                throw std::runtime_error("Invalid JSON object for air quality index");
            }
            QJsonObject airQuality = jsonDoc.object();

            currentAirQuality = airQuality;

            QString indexLevelName = airQuality["stIndexLevel"].toObject()["indexLevelName"].toString();
            QString calcDate = airQuality["stCalcDate"].toString();

            QString text = QString("Indeks jakości powietrza: %1 (dane z: %2)")
                               .arg(indexLevelName)
                               .arg(QDateTime::fromString(calcDate, Qt::ISODate).toString("dd.MM.yyyy HH:mm"));

            QString color;
            if (indexLevelName == "Bardzo dobry" || indexLevelName == "Dobry") {
                color = "green";
            } else if (indexLevelName == "Umiarkowany") {
                color = "orange";
            } else {
                color = "red";
            }

            emit historicalDataAvailableChanged(hasHistoricalData(currentStationId));
            emit airQualityUpdateRequested(text, color);
        } catch (const std::exception& e) {
            qDebug() << "Exception while parsing air quality JSON:" << e.what();
            emit airQualityUpdateRequested("Błąd ładowania danych", "red");
        }
    } else {
        qDebug() << "Error fetching air quality index:" << reply->errorString();
    }
    reply->deleteLater();
}

/**
 * @brief Obsługuje wybór stacji przez użytkownika.
 * @param stationId Identyfikator wybranej stacji.
 */
void MainWindow::stationSelected(int stationId)
{
    if (!stationsMap.contains(stationId)) return;

    currentStationId = stationId;

    QJsonObject station = stationsMap[stationId];
    QString info = generateStationInfo(station);
    emit stationInfoUpdateRequested(info);
    fetchSensors(stationId);
    fetchAirQualityIndex(stationId);

    emit historicalDataAvailableChanged(hasHistoricalData(stationId));
}

/**
 * @brief Obsługuje wybór czujnika przez użytkownika.
 * @param sensorId Identyfikator wybranego czujnika.
 */
void MainWindow::sensorSelected(int sensorId)
{
    if (sensorId > 0) {
        currentSensorId = sensorId;
        fetchMeasurements(sensorId);

        emit historicalDataAvailableChanged(hasHistoricalData(currentStationId, sensorId));
    }
}

/**
 * @brief Filtruje stacje na podstawie tekstu wyszukiwania.
 * @param searchText Tekst wyszukiwania (nazwa miejscowości).
 */
void MainWindow::searchStations(const QString& searchText)
{
    if (searchText.isEmpty()) {
        displayStations(allStations);
        return;
    }

    QJsonArray filteredStations;
    for (const QJsonValue& value : allStations) {
        QJsonObject station = value.toObject();
        QString cityName = station["city"].toObject()["name"].toString().toLower();
        if (cityName.contains(searchText.toLower())) {
            filteredStations.append(station);
        }
    }

    displayStations(filteredStations);
}

/**
 * @brief Wyświetla wszystkie stacje.
 */
void MainWindow::showAllStations()
{
    displayStations(allStations);
}

/**
 * @brief Przygotowuje listę stacji do wyświetlenia w interfejsie.
 * @param stations Tablica JSON z danymi stacji.
 */
void MainWindow::displayStations(const QJsonArray& stations)
{
    QVariantList stationsList;

    for (const QJsonValue& value : stations) {
        QJsonObject station = value.toObject();
        int id = station["id"].toInt();
        QString stationName = station["stationName"].toString();
        QString cityName = station["city"].toObject()["name"].toString();

        QString displayText = QString("%1 - %2").arg(cityName, stationName);

        QVariantMap stationMap;
        stationMap["display"] = displayText;
        stationMap["stationId"] = id;
        stationMap["station"] = station.toVariantMap();
        stationsList.append(stationMap);
    }

    emit stationsUpdateRequested(stationsList);
}

/**
 * @brief Generuje informacje o stacji w formacie HTML.
 * @param station Obiekt JSON z danymi stacji.
 * @return Tekst HTML z informacjami o stacji.
 */
QString MainWindow::generateStationInfo(const QJsonObject& station)
{
    QString stationName = station["stationName"].toString();
    QJsonObject city = station["city"].toObject();
    QString cityName = city["name"].toString();
    QString street = station["addressStreet"].toString();

    QJsonObject commune = city["commune"].toObject();
    QString communeName = commune["communeName"].toString();
    QString districtName = commune["districtName"].toString();
    QString provinceName = commune["provinceName"].toString();

    double lat = station["gegrLat"].toString().toDouble();
    double lon = station["gegrLon"].toString().toDouble();

    QString info = QString("<h3>%1</h3>").arg(stationName) +
                   QString("<p><b>Miasto:</b> %1</p>").arg(cityName) +
                   QString("<p><b>Ulica:</b> %1</p>").arg(street) +
                   QString("<p><b>Gmina:</b> %1</p>").arg(communeName) +
                   QString("<p><b>Powiat:</b> %1</p>").arg(districtName) +
                   QString("<p><b>Województwo:</b> %1</p>").arg(provinceName) +
                   QString("<p><b>Współrzędne:</b> %1, %2</p>").arg(lat).arg(lon);

    return info;
}

/**
 * @brief Zwraca ścieżkę do lokalnej bazy danych.
 * @return Ścieżka do katalogu bazy danych.
 */
QString MainWindow::getDatabasePath()
{
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(dataPath);

    try {
        if (!dir.exists()) {
            if (!dir.mkpath(".")) {
                throw std::runtime_error("Failed to create database directory");
            }
        }
    } catch (const std::exception& e) {
        qDebug() << "Exception while creating database path:" << e.what();
    }

    return dataPath;
}

/**
 * @brief Generuje ścieżkę do pliku z pomiarami.
 * @param stationId Identyfikator stacji.
 * @param sensorId Identyfikator czujnika.
 * @return Ścieżka do pliku JSON z pomiarami.
 */
QString MainWindow::getMeasurementsFilePath(int stationId, int sensorId)
{
    return QString("%1/measurements_station%2_sensor%3.json")
    .arg(getDatabasePath())
        .arg(stationId)
        .arg(sensorId);
}

/**
 * @brief Generuje ścieżkę do pliku z indeksem jakości powietrza.
 * @param stationId Identyfikator stacji.
 * @return Ścieżka do pliku JSON z danymi jakości powietrza.
 */
QString MainWindow::getAirQualityFilePath(int stationId)
{
    return QString("%1/airquality_station%2.json")
    .arg(getDatabasePath())
        .arg(stationId);
}

/**
 * @brief Zapisuje dokument JSON do pliku.
 * @param filePath Ścieżka do pliku.
 * @param jsonDoc Dokument JSON do zapisania.
 * @return True, jeśli zapis się powiódł, w przeciwnym razie false.
 */
bool MainWindow::saveJsonToFile(const QString& filePath, const QJsonDocument& jsonDoc)
{
    try {
        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly)) {
            throw std::runtime_error("Failed to open file for writing: " + filePath.toStdString());
        }

        file.write(jsonDoc.toJson());
        file.close();
        return true;
    } catch (const std::exception& e) {
        qDebug() << "Exception while saving JSON to file:" << e.what();
        return false;
    }
}

/**
 * @brief Wczytuje dokument JSON z pliku.
 * @param filePath Ścieżka do pliku.
 * @return Dokument JSON lub pusty dokument w przypadku błędu.
 */
QJsonDocument MainWindow::loadJsonFromFile(const QString& filePath)
{
    try {
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly)) {
            throw std::runtime_error("Failed to open file for reading: " + filePath.toStdString());
        }

        QByteArray data = file.readAll();
        file.close();

        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        if (jsonDoc.isNull()) {
            throw std::runtime_error("Failed to parse JSON from file: " + filePath.toStdString());
        }
        return jsonDoc;
    } catch (const std::exception& e) {
        qDebug() << "Exception while loading JSON from file:" << e.what();
        return QJsonDocument();
    }
}

/**
 * @brief Zapisuje bieżące pomiary do lokalnej bazy danych.
 */
void MainWindow::saveMeasurementsToDatabase()
{
    if (currentStationId < 0 || currentSensorId < 0 || currentMeasurements.isEmpty()) {
        qDebug() << "No data to save";
        return;
    }

    QJsonObject data;
    data["stationId"] = currentStationId;
    data["sensorId"] = currentSensorId;
    data["key"] = currentMeasurementKey;
    data["saveDate"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    QJsonArray measurementsArray;
    for (const QVariant& measurement : currentMeasurements) {
        QVariantMap map = measurement.toMap();
        QJsonObject obj;
        obj["date"] = map["date"].toString();
        obj["value"] = map["value"].toDouble();
        measurementsArray.append(obj);
    }

    data["measurements"] = measurementsArray;

    QJsonDocument jsonDoc(data);
    QString filePath = getMeasurementsFilePath(currentStationId, currentSensorId);

    if (saveJsonToFile(filePath, jsonDoc)) {
        qDebug() << "Measurements saved to:" << filePath;
        emit historicalDataAvailableChanged(true);
    }
}

/**
 * @brief Zapisuje bieżący indeks jakości powietrza do lokalnej bazy danych.
 */
void MainWindow::saveAirQualityToDatabase()
{
    if (currentStationId < 0 || currentAirQuality.isEmpty()) {
        qDebug() << "No air quality data to save";
        return;
    }

    QJsonObject data;
    data["stationId"] = currentStationId;
    data["saveDate"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    data["airQuality"] = currentAirQuality;

    QJsonDocument jsonDoc(data);
    QString filePath = getAirQualityFilePath(currentStationId);

    if (saveJsonToFile(filePath, jsonDoc)) {
        qDebug() << "Air quality saved to:" << filePath;
        emit historicalDataAvailableChanged(true);
    }
}

/**
 * @brief Sprawdza, czy istnieją dane historyczne dla stacji lub czujnika.
 * @param stationId Identyfikator stacji.
 * @param sensorId Identyfikator czujnika (domyślnie -1 dla indeksu jakości powietrza).
 * @return True, jeśli dane historyczne istnieją, w przeciwnym razie false.
 */
bool MainWindow::hasHistoricalData(int stationId, int sensorId)
{
    if (sensorId == -1) {
        QFile file(getAirQualityFilePath(stationId));
        return file.exists();
    } else {
        QFile file(getMeasurementsFilePath(stationId, sensorId));
        return file.exists();
    }
}

/**
 * @brief Wczytuje historyczne pomiary dla czujnika.
 * @param sensorId Identyfikator czujnika.
 */
void MainWindow::loadHistoricalMeasurements(int sensorId)
{
    if (currentStationId < 0) {
        qDebug() << "No station selected";
        return;
    }

    QString filePath = getMeasurementsFilePath(currentStationId, sensorId);
    QJsonDocument doc = loadJsonFromFile(filePath);

    if (doc.isObject()) {
        QJsonObject data = doc.object();
        QString key = data["key"].toString();
        QJsonArray measurements = data["measurements"].toArray();

        QVariantList valuesList;
        for (const QJsonValue& value : measurements) {
            QJsonObject measurement = value.toObject();
            QVariantMap point;
            point["date"] = measurement["date"].toString();
            point["value"] = measurement["value"].toDouble();
            valuesList.append(point);
        }

        emit measurementsUpdateRequested(key + " (dane historyczne)", valuesList);
    }
}

/**
 * @brief Wczytuje historyczny indeks jakości powietrza dla stacji.
 * @param stationId Identyfikator stacji.
 */
void MainWindow::loadHistoricalAirQuality(int stationId)
{
    QString filePath = getAirQualityFilePath(stationId);
    QJsonDocument doc = loadJsonFromFile(filePath);

    if (doc.isObject()) {
        QJsonObject data = doc.object();
        QJsonObject airQuality = data["airQuality"].toObject();

        QString indexLevelName = airQuality["stIndexLevel"].toObject()["indexLevelName"].toString();
        QString calcDate = airQuality["stCalcDate"].toString();
        QString saveDate = data["saveDate"].toString();

        QString text = QString("Indeks jakości powietrza (HISTORYCZNY): %1 (dane z: %2, zapisane: %3)")
                           .arg(indexLevelName)
                           .arg(QDateTime::fromString(calcDate, Qt::ISODate).toString("dd.MM.yyyy HH:mm"))
                           .arg(QDateTime::fromString(saveDate, Qt::ISODate).toString("dd.MM.yyyy HH:mm"));

        QString color;
        if (indexLevelName == "Bardzo dobry" || indexLevelName == "Dobry") {
            color = "green";
        } else if (indexLevelName == "Umiarkowany") {
            color = "orange";
        } else {
            color = "red";
        }

        emit airQualityUpdateRequested(text, color);
    }
}

/**
 * @brief Przełącza między danymi bieżącymi a historycznymi.
 * @param useHistorical True dla danych historycznych, false dla bieżących.
 */
void MainWindow::toggleDataSource(bool useHistorical)
{
    if (useHistorical) {
        if (currentSensorId > 0) {
            loadHistoricalMeasurements(currentSensorId);
        }
        if (currentStationId > 0) {
            loadHistoricalAirQuality(currentStationId);
        }
    } else {
        if (currentSensorId > 0) {
            fetchMeasurements(currentSensorId);
        }
        if (currentStationId > 0) {
            fetchAirQualityIndex(currentStationId);
        }
    }
}

/**
 * @brief Analizuje pomiary i zwraca statystyki.
 * @return QVariantMap z wynikami analizy (średnia, mediana, min, max, liczba pomiarów).
 */
QVariantMap MainWindow::analyzeMeasurements()
{
    QVariantMap analysis;

    if (currentMeasurements.isEmpty()) {
        analysis["error"] = "Brak danych do analizy";
        return analysis;
    }

    double sum = 0.0;
    int count = 0;
    double minValue = std::numeric_limits<double>::max();
    double maxValue = std::numeric_limits<double>::lowest();
    QList<double> values;

    for (const QVariant& measurement : currentMeasurements) {
        QVariantMap map = measurement.toMap();
        if (map["value"].isValid() && !map["value"].isNull()) {
            double value = map["value"].toDouble();
            sum += value;
            count++;
            minValue = std::min(minValue, value);
            maxValue = std::max(maxValue, value);
            values.append(value);
        }
    }

    if (count == 0) {
        analysis["error"] = "Brak ważnych danych do analizy";
        return analysis;
    }

    double average = sum / count;
    analysis["average"] = QString::number(average, 'f', 2);

    std::sort(values.begin(), values.end());
    double median;
    if (count % 2 == 0) {
        median = (values[count / 2 - 1] + values[count / 2]) / 2.0;
    } else {
        median = values[count / 2];
    }
    analysis["median"] = QString::number(median, 'f', 2);

    analysis["min"] = QString::number(minValue, 'f', 2);
    analysis["max"] = QString::number(maxValue, 'f', 2);
    analysis["count"] = count;

    emit analysisUpdateRequested(analysis);
    return analysis;
}
