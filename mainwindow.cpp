#include "mainwindow.h"
#include <QDebug>
#include <QDateTime>
#include <algorithm> // Dla std::sort

MainWindow::MainWindow(QObject *parent)
    : QObject(parent)
{
    networkManager = new QNetworkAccessManager(this);
    fetchStations();
}

MainWindow::~MainWindow()
{
}

void MainWindow::fetchStations()
{
    QNetworkRequest request((QUrl(API_BASE_URL + API_STATIONS_ENDPOINT)));
    QNetworkReply* reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, &MainWindow::onStationsReceived);
}

void MainWindow::fetchSensors(int stationId)
{
    QNetworkRequest request((QUrl(API_BASE_URL + API_SENSORS_ENDPOINT + QString::number(stationId))));
    QNetworkReply* reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, &MainWindow::onSensorsReceived);
}

void MainWindow::fetchMeasurements(int sensorId)
{
    QNetworkRequest request((QUrl(API_BASE_URL + API_MEASUREMENTS_ENDPOINT + QString::number(sensorId))));
    QNetworkReply* reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, &MainWindow::onMeasurementsReceived);
}

void MainWindow::fetchAirQualityIndex(int stationId)
{
    QNetworkRequest request((QUrl(API_BASE_URL + API_AIR_QUALITY_ENDPOINT + QString::number(stationId))));
    QNetworkReply* reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, &MainWindow::onAirQualityIndexReceived);
}

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

            // Store stations in map for quick access
            for (const QJsonValue& value : allStations) {
                QJsonObject station = value.toObject();
                int id = station["id"].toInt();
                stationsMap[id] = station;
            }

            displayStations(allStations);
        } catch (const std::exception& e) {
            qDebug() << "Exception while parsing stations JSON:" << e.what();
            emit stationsUpdateRequested(QVariantList()); // Emit empty list to clear UI
        }
    } else {
        qDebug() << "Error fetching stations:" << reply->errorString();
    }
    reply->deleteLater();
}

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
            emit sensorsUpdateRequested(QVariantList()); // Emit empty list to clear UI
        }
    } else {
        qDebug() << "Error fetching sensors:" << reply->errorString();
    }
    reply->deleteLater();
}

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

            // Zapisz aktualne dane do użycia później
            currentMeasurementKey = key;
            currentMeasurements = valuesList;

            // Sprawdź, czy są dostępne dane historyczne
            emit historicalDataAvailableChanged(hasHistoricalData(currentStationId, currentSensorId));

            emit measurementsUpdateRequested(key, valuesList);
        } catch (const std::exception& e) {
            qDebug() << "Exception while parsing measurements JSON:" << e.what();
            emit measurementsUpdateRequested("Error", QVariantList()); // Emit empty list to clear UI
        }
    } else {
        qDebug() << "Error fetching measurements:" << reply->errorString();
    }
    reply->deleteLater();
}

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

            // Zapisujemy aktualne dane jakości powietrza
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

            // Sprawdź, czy są dostępne dane historyczne
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

void MainWindow::stationSelected(int stationId)
{
    if (!stationsMap.contains(stationId)) return;

    currentStationId = stationId;

    QJsonObject station = stationsMap[stationId];
    QString info = generateStationInfo(station);
    emit stationInfoUpdateRequested(info);
    fetchSensors(stationId);
    fetchAirQualityIndex(stationId);

    // Sprawdź, czy są dostępne dane historyczne dla tej stacji
    emit historicalDataAvailableChanged(hasHistoricalData(stationId));
}

void MainWindow::sensorSelected(int sensorId)
{
    if (sensorId > 0) {
        currentSensorId = sensorId;
        fetchMeasurements(sensorId);

        // Sprawdź, czy są dostępne dane historyczne dla tego czujnika
        emit historicalDataAvailableChanged(hasHistoricalData(currentStationId, sensorId));
    }
}

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

void MainWindow::showAllStations()
{
    displayStations(allStations);
}

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

// Metody do obsługi bazy danych lokalnych

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

QString MainWindow::getMeasurementsFilePath(int stationId, int sensorId)
{
    return QString("%1/measurements_station%2_sensor%3.json")
    .arg(getDatabasePath())
        .arg(stationId)
        .arg(sensorId);
}

QString MainWindow::getAirQualityFilePath(int stationId)
{
    return QString("%1/airquality_station%2.json")
    .arg(getDatabasePath())
        .arg(stationId);
}

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

bool MainWindow::hasHistoricalData(int stationId, int sensorId)
{
    if (sensorId == -1) {
        // Sprawdzamy tylko dane dla stacji
        QFile file(getAirQualityFilePath(stationId));
        return file.exists();
    } else {
        // Sprawdzamy dane dla czujnika
        QFile file(getMeasurementsFilePath(stationId, sensorId));
        return file.exists();
    }
}

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

    // Oblicz średnią
    double average = sum / count;
    analysis["average"] = QString::number(average, 'f', 2);

    // Oblicz medianę
    std::sort(values.begin(), values.end());
    double median;
    if (count % 2 == 0) {
        median = (values[count / 2 - 1] + values[count / 2]) / 2.0;
    } else {
        median = values[count / 2];
    }
    analysis["median"] = QString::number(median, 'f', 2);

    // Wartości minimalna i maksymalna
    analysis["min"] = QString::number(minValue, 'f', 2);
    analysis["max"] = QString::number(maxValue, 'f', 2);

    // Liczba pomiarów
    analysis["count"] = count;

    emit analysisUpdateRequested(analysis);
    return analysis;
}
