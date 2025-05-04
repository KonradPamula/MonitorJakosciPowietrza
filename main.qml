import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtCharts 2.15

/**
 * @brief Główny komponent interfejsu użytkownika aplikacji.
 *
 * ApplicationWindow definiuje główne okno aplikacji do monitorowania jakości powietrza.
 * Zawiera nagłówek, panele z informacjami o stacjach, wykresy i listę stacji pomiarowych.
 */
ApplicationWindow {
    id: root
    visible: true
    width: 1200
    height: 800
    title: "Monitor Jakości Powietrza - GIOŚ"
    color: "#f5f5f5"

    /// @brief Aktualnie wybrana stacja pomiarowa.
    property var currentStation: null
    /// @brief Aktualnie wybrany czujnik.
    property var currentSensor: null
    /// @brief Czy dostępne są dane historyczne.
    property bool hasHistoricalData: false
    /// @brief Czy używane są dane historyczne.
    property bool usingHistoricalData: false
    /// @brief Czy pokazywać panel analizy danych.
    property bool showAnalysis: false

    /// @brief Główny kolor interfejsu (niebieski).
    property color primaryColor: "#1976D2"
    /// @brief Kolor akcentu (zielony).
    property color accentColor: "#4CAF50"
    /// @brief Kolor tekstu.
    property color textColor: "#424242"
    /// @brief Jasny kolor tła.
    property color lightBgColor: "#FFFFFF"
    /// @brief Kolor obramowania.
    property color borderColor: "#E0E0E0"

    /**
     * @brief Główny układ pionowy interfejsu.
     *
     * Zawiera nagłówek i główny obszar treści podzielony na dwie kolumny.
     */
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 16

        /// @brief Nagłówek aplikacji z tytułem i paskiem wyszukiwania.
        Rectangle {
            Layout.fillWidth: true
            height: 60
            color: primaryColor
            radius: 8

            RowLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 12

                /// @brief Ikona aplikacji (obecnie niewidoczna).
                Image {
                    source: "qrc:/icons/air_quality.png"
                    sourceSize.width: 40
                    sourceSize.height: 40
                    fillMode: Image.PreserveAspectFit
                    visible: false
                }

                /// @brief Tytuł aplikacji.
                Label {
                    text: "Monitor Jakości Powietrza"
                    font.pixelSize: 22
                    font.bold: true
                    color: "white"
                }

                Item { Layout.fillWidth: true }

                /// @brief Pasek wyszukiwania stacji.
                Rectangle {
                    Layout.preferredWidth: 350
                    height: 40
                    radius: 20
                    color: Qt.lighter(primaryColor, 1.7)
                    border.color: Qt.lighter(primaryColor, 1.3)

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 8

                        /// @brief Pole tekstowe do wyszukiwania miejscowości.
                        TextField {
                            id: searchField
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            placeholderText: "Wpisz nazwę miejscowości..."
                            placeholderTextColor: "white"
                            color: "white"
                            font.pixelSize: 14
                            background: Item {}
                            onAccepted: mainWindow.searchStations(text)
                        }

                        /// @brief Przycisk wyszukiwania.
                        Rectangle {
                            width: 24
                            height: 24
                            radius: 12
                            color: accentColor

                            Text {
                                anchors.centerIn: parent
                                text: "🔍"
                                color: "white"
                                font.pixelSize: 14
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: mainWindow.searchStations(searchField.text)
                            }
                        }
                    }
                }

                /// @brief Przycisk do wyświetlania wszystkich stacji.
                Button {
                    text: "Wszystkie stacje"
                    flat: true
                    contentItem: Text {
                        text: parent.text
                        color: "white"
                        font.pixelSize: 14
                    }
                    background: Rectangle {
                        color: "transparent"
                        border.color: "white"
                        border.width: 1
                        radius: 4
                    }
                    onClicked: mainWindow.showAllStations()
                }
            }
        }

        /// @brief Główny obszar treści podzielony na dwie kolumny.
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 16

            /// @brief Lewa kolumna z informacjami o stacji, indeksem jakości i wykresami.
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 2
                spacing: 12

                /// @brief Panel z informacjami o wybranej stacji.
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 120
                    color: lightBgColor
                    radius: 8
                    border.color: borderColor
                    border.width: 1

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 8

                        Label {
                            text: "Informacje o stacji pomiarowej"
                            font.pixelSize: 16
                            font.bold: true
                            color: textColor
                        }

                        /// @brief Obszar tekstowy z informacjami o stacji (HTML).
                        TextArea {
                            id: stationInfo
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            readOnly: true
                            textFormat: Text.RichText
                            wrapMode: TextEdit.Wrap
                            font.pixelSize: 12
                            background: Item {}
                        }
                    }
                }

                /// @brief Panel indeksu jakości powietrza.
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 90
                    color: lightBgColor
                    radius: 8
                    border.color: borderColor
                    border.width: 1

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 12

                        /// @brief Wskaźnik jakości powietrza (okrąg z ikoną).
                        Rectangle {
                            id: qualityIndicator
                            width: 60
                            height: 60
                            radius: 30
                            color: "#AAAAAA"

                            Label {
                                anchors.centerIn: parent
                                text: "?"
                                font.pixelSize: 28
                                font.bold: true
                                color: "white"
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4

                            Label {
                                text: "Indeks jakości powietrza"
                                font.pixelSize: 14
                                font.bold: true
                                color: textColor
                            }

                            /// @brief Etykieta z opisem indeksu jakości powietrza.
                            Label {
                                id: airQualityLabel
                                text: "Brak danych"
                                font.pixelSize: 18
                                font.bold: true
                                color: textColor
                                wrapMode: Text.WordWrap
                            }
                        }

                        /// @brief Lista rozwijana do wyboru czujnika.
                        ComboBox {
                            id: sensorsComboBox
                            Layout.preferredWidth: 200
                            Layout.preferredHeight: 36
                            font.pixelSize: 12
                            model: ListModel {
                                id: sensorsModel
                                ListElement { display: "Wybierz czujnik..."; sensorId: 0 }
                            }
                            textRole: "display"
                            onCurrentIndexChanged: {
                                if (currentIndex > 0 && model.count > currentIndex) {
                                    var sensorId = model.get(currentIndex).sensorId;
                                    currentSensor = sensorId;
                                    mainWindow.sensorSelected(sensorId);
                                }
                            }
                        }
                    }
                }

                /// @brief Panel zarządzania danymi (zapis i dane historyczne).
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 60
                    color: lightBgColor
                    radius: 8
                    border.color: borderColor
                    border.width: 1
                    visible: currentStation !== null

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 12

                        Label {
                            text: "Zarządzanie danymi"
                            font.pixelSize: 14
                            font.bold: true
                            color: textColor
                        }

                        /// @brief Przycisk zapisu pomiarów do bazy danych.
                        Button {
                            id: saveMeasurementsButton
                            text: "Zapisz pomiary"
                            font.pixelSize: 12
                            enabled: sensorsComboBox.currentIndex > 0 && !usingHistoricalData
                            onClicked: {
                                mainWindow.saveMeasurementsToDatabase();
                                saveDataToast.visible = true;
                                saveDataTimer.restart();
                            }
                        }

                        /// @brief Przycisk zapisu indeksu jakości powietrza.
                        Button {
                            id: saveAirQualityButton
                            text: "Zapisz jakość powietrza"
                            font.pixelSize: 12
                            enabled: currentStation !== null && !usingHistoricalData
                            onClicked: {
                                mainWindow.saveAirQualityToDatabase();
                                saveDataToast.visible = true;
                                saveDataTimer.restart();
                            }
                        }

                        Item { Layout.fillWidth: true }

                        /// @brief Przełącznik danych historycznych.
                        Switch {
                            id: historicalDataSwitch
                            text: "Dane historyczne"
                            font.pixelSize: 12
                            enabled: hasHistoricalData
                            checked: usingHistoricalData
                            onToggled: {
                                usingHistoricalData = checked;
                                mainWindow.toggleDataSource(checked);
                            }
                        }
                    }
                }

                /// @brief Panel z wykresem pomiarów.
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: lightBgColor
                    radius: 8
                    border.color: borderColor
                    border.width: 1

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 12

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 12

                            Label {
                                text: "Pomiary w czasie"
                                font.pixelSize: 16
                                font.bold: true
                                color: textColor
                            }

                            Item { Layout.fillWidth: true }

                            /// @brief Przycisk pokazywania/ukrywania analizy.
                            Button {
                                text: showAnalysis ? "Ukryj analizę" : "Pokaż analizę"
                                font.pixelSize: 12
                                enabled: sensorsComboBox.currentIndex > 0
                                onClicked: {
                                    if (!showAnalysis) {
                                        mainWindow.analyzeMeasurements();
                                    }
                                    showAnalysis = !showAnalysis;
                                }
                            }
                        }

                        /// @brief Wykres wyświetlający pomiary w czasie.
                        ChartView {
                            id: chartView
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            antialiasing: true
                            title: "Pomiary w czasie"
                            titleFont.pixelSize: 16
                            legend.visible: true
                            legend.font.pixelSize: 12
                            backgroundColor: "transparent"
                            titleColor: textColor
                            margins.left: 20
                            margins.right: 20
                            margins.top: 20
                            margins.bottom: 20

                            LineSeries {
                                id: measurementSeries
                                name: "Pomiary"
                                color: accentColor
                                width: 2
                                axisX: DateTimeAxis {
                                    id: timeAxis
                                    format: "dd.MM HH:mm"
                                    titleText: "Data i czas"
                                    labelsFont.pixelSize: 12
                                    labelsColor: textColor
                                    gridLineColor: "#E0E0E0"
                                    tickCount: 5
                                }
                                axisY: ValueAxis {
                                    id: valueAxis
                                    titleText: "Wartość"
                                    labelsFont.pixelSize: 12
                                    labelsColor: textColor
                                    gridLineColor: "#E0E0E0"
                                    tickCount: 5
                                }
                            }
                        }
                    }
                }

                /// @brief Panel wyników analizy pomiarów.
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: showAnalysis ? 80 : 0
                    color: "#F9F9F9"
                    radius: 4
                    border.color: borderColor
                    border.width: 1
                    visible: showAnalysis
                    clip: true

                    Behavior on Layout.preferredHeight {
                        NumberAnimation { duration: 200 }
                    }

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 16

                        ColumnLayout {
                            spacing: 4
                            Label {
                                text: "Wyniki analizy"
                                font.pixelSize: 14
                                font.bold: true
                                color: textColor
                            }

                            /// @brief Etykieta z wynikami analizy.
                            Label {
                                id: analysisLabel
                                text: ""
                                font.pixelSize: 12
                                color: textColor
                                wrapMode: Text.WordWrap
                            }
                        }

                        Item { Layout.fillWidth: true }
                    }
                }
            }

            /// @brief Prawa kolumna z listą stacji pomiarowych.
            Rectangle {
                Layout.preferredWidth: 280
                Layout.fillHeight: true
                color: lightBgColor
                radius: 8
                border.color: borderColor
                border.width: 1

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 12

                    Label {
                        text: "Stacje pomiarowe"
                        font.pixelSize: 16
                        font.bold: true
                        color: textColor
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: borderColor
                    }

                    /// @brief Lista stacji pomiarowych.
                    ListView {
                        id: stationsList
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        spacing: 4

                        model: ListModel {
                            id: stationsModel
                        }

                        delegate: Rectangle {
                            width: ListView.view.width
                            height: 50
                            color: ListView.isCurrentItem ? Qt.lighter(primaryColor) : "transparent"
                            radius: 6

                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 8
                                spacing: 8

                                Rectangle {
                                    width: 6
                                    height: 36
                                    radius: 3
                                    color: primaryColor
                                    visible: ListView.isCurrentItem
                                }

                                Label {
                                    Layout.fillWidth: true
                                    text: model.display
                                    elide: Text.ElideRight
                                    wrapMode: Text.WordWrap
                                    maximumLineCount: 2
                                    font.pixelSize: 12
                                    color: textColor
                                }
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    stationsList.currentIndex = index
                                    currentStation = model.station
                                    currentSensor = null
                                    mainWindow.stationSelected(model.stationId)
                                    usingHistoricalData = false
                                    historicalDataSwitch.checked = false
                                    showAnalysis = false
                                    analysisLabel.text = ""
                                }
                            }
                        }

                        ScrollBar.vertical: ScrollBar {
                            policy: ScrollBar.AsNeeded
                        }
                    }
                }
            }
        }
    }

    /// @brief Powiadomienie o zapisaniu danych (toast).
    Rectangle {
        id: saveDataToast
        width: saveDataToastText.width + 40
        height: 40
        radius: 20
        color: "#323232"
        visible: false
        opacity: visible ? 1.0 : 0.0

        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 30

        Behavior on opacity {
            NumberAnimation { duration: 300 }
        }

        Text {
            id: saveDataToastText
            text: "Dane zostały zapisane"
            color: "white"
            font.pixelSize: 14
            anchors.centerIn: parent
        }

        Timer {
            id: saveDataTimer
            interval: 2000
            onTriggered: saveDataToast.visible = false
        }
    }

    /**
     * @brief Połączenia sygnałów z backendu MainWindow.
     *
     * Obsługuje aktualizacje danych w interfejsie użytkownika na podstawie sygnałów z backendu.
     */
    Connections {
        target: mainWindow

        /// @brief Aktualizuje listę stacji w interfejsie.
        function onStationsUpdateRequested(stations) {
            stationsModel.clear()
            for (var i = 0; i < stations.length; i++) {
                stationsModel.append(stations[i])
            }
        }

        /// @brief Aktualizuje informacje o stacji.
        function onStationInfoUpdateRequested(info) {
            stationInfo.text = info
        }

        /// @brief Aktualizuje listę czujników.
        function onSensorsUpdateRequested(sensors) {
            sensorsModel.clear()
            sensorsModel.append({display: "Wybierz czujnik...", sensorId: 0})
            for (var i = 0; i < sensors.length; i++) {
                sensorsModel.append(sensors[i])
            }
            sensorsComboBox.currentIndex = 0
            currentSensor = null
            showAnalysis = false
            analysisLabel.text = ""
        }

        /// @brief Aktualizuje wykres pomiarów.
        function onMeasurementsUpdateRequested(key, values) {
            measurementSeries.clear()
            chartView.title = "Pomiary parametru: " + key
            showAnalysis = false
            analysisLabel.text = ""

            if (values.length === 0) {
                return
            }

            var minTime = Number.MAX_VALUE
            var maxTime = 0
            var minValue = Number.MAX_VALUE
            var maxValue = Number.MIN_VALUE

            for (var i = 0; i < values.length; i++) {
                var point = values[i]
                if (point.value !== null && !isNaN(point.value)) {
                    var timestamp = Date.parse(point.date)
                    measurementSeries.append(timestamp, point.value)

                    minTime = Math.min(minTime, timestamp)
                    maxTime = Math.max(maxTime, timestamp)
                    minValue = Math.min(minValue, point.value)
                    maxValue = Math.max(maxValue, point.value)
                }
            }

            if (minTime !== Number.MAX_VALUE && maxTime !== 0) {
                timeAxis.min = new Date(minTime)
                timeAxis.max = new Date(maxTime)

                var valueRange = maxValue - minValue
                valueAxis.min = Math.max(0, minValue - valueRange * 0.1)
                valueAxis.max = maxValue + valueRange * 0.1
            }
        }

        /// @brief Aktualizuje indeks jakości powietrza.
        function onAirQualityUpdateRequested(qualityText, color) {
            airQualityLabel.text = qualityText
            airQualityLabel.color = color
            qualityIndicator.color = color

            var icon = "?"
            if (qualityText.indexOf("Bardzo dobry") !== -1) {
                icon = "😃";
            } else if (qualityText.indexOf("Dobry") !== -1) {
                icon = "🙂";
            } else if (qualityText.indexOf("Umiarkowany") !== -1) {
                icon = "😐";
            } else if (qualityText.indexOf("Dostateczny") !== -1) {
                icon = "😕";
            } else if (qualityText.indexOf("Zły") !== -1) {
                icon = "😷";
            } else if (qualityText.indexOf("Bardzo zły") !== -1) {
                icon = "☠️";
            }

            qualityIndicator.children[0].text = icon
        }

        /// @brief Aktualizuje dostępność danych historycznych.
        function onHistoricalDataAvailableChanged(available) {
            hasHistoricalData = available;
            if (!available && usingHistoricalData) {
                usingHistoricalData = false;
                historicalDataSwitch.checked = false;
            }
        }

        /// @brief Aktualizuje wyniki analizy pomiarów.
        function onAnalysisUpdateRequested(analysis) {
            if (analysis.error) {
                analysisLabel.text = analysis.error
            } else {
                analysisLabel.text = `Średnia: ${analysis.average}\n` +
                                     `Mediana: ${analysis.median}\n` +
                                     `Min: ${analysis.min}\n` +
                                     `Max: ${analysis.max}\n` +
                                     `Liczba pomiarów: ${analysis.count}`
            }
        }
    }
}
