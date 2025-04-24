#include <iostream>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <string>
#include <cmath>
#include <algorithm>

using json = nlohmann::json;

const double PI = 3.14159265358979323846;

size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    size_t totalSize = size * nmemb;
    output->append((char*)contents, totalSize);
    return totalSize;
}

std::string getSensorData(CURL* curl, int stationId) {
    std::string sensorBuffer;
    std::string url = "https://api.gios.gov.pl/pjp-api/rest/station/sensors/" + std::to_string(stationId);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &sensorBuffer);
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) std::cerr << "Blad pobierania sensorow: " << curl_easy_strerror(res) << "\n";
    return sensorBuffer;
}

std::string getMeasurementData(CURL* curl, int sensorId) {
    std::string dataBuffer;
    std::string url = "https://api.gios.gov.pl/pjp-api/rest/data/getData/" + std::to_string(sensorId);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &dataBuffer);
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) std::cerr << "Blad pobierania pomiarow: " << curl_easy_strerror(res) << "\n";
    return dataBuffer;
}

double calculateDistance(double lat1, double lon1, double lat2, double lon2) {
    const double R = 6371;
    double dLat = (lat2 - lat1) * PI / 180;
    double dLon = (lon2 - lon1) * PI / 180;
    double a = std::sin(dLat/2) * std::sin(dLat/2) +
               std::cos(lat1 * PI / 180) * std::cos(lat2 * PI / 180) * 
               std::sin(dLon/2) * std::sin(dLon/2);
    double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1-a));
    return R * c;
}

bool getLocationCoordinates(CURL* curl, const std::string& location, double& lat, double& lon) {
    std::string buffer;
    std::string encodedLocation = location;
    // Zastąpienie spacji znakiem '+'
    for (char& c : encodedLocation) {
        if (c == ' ') c = '+';
    }
    std::string url = "https://nominatim.openstreetmap.org/search?q=" + encodedLocation + ",+Polska&format=json&limit=1";
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Grok/1.0");
    
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) return false;
    
    try {
        json result = json::parse(buffer);
        if (!result.empty()) {
            lat = std::stod(result[0]["lat"].get<std::string>());
            lon = std::stod(result[0]["lon"].get<std::string>());
            return true;
        }
    } catch (...) {
        return false;
    }
    return false;
}

void displayStationDetails(CURL* curl, const json& station, double distance = -1) {
    std::cout << "ID: " << station["id"] << "\n";
    std::cout << "Nazwa stacji: " << station["stationName"] << "\n";
    std::cout << "Wspolrzedne geograficzne:\n";
    std::cout << "  Szerokosc: " << station["gegrLat"] << "\n";
    std::cout << "  Dlugosc: " << station["gegrLon"] << "\n";
    if (distance >= 0) {
        std::cout << "Odleglosc od podanej lokalizacji: " << distance << " km\n";
    }
    
    std::cout << "Adres:\n";
    if (station.contains("city") && station["city"].is_object()) {
        std::cout << "  Miasto: " << station["city"]["name"] << " (ID: " << station["city"]["id"] << ")\n";
        if (station["city"].contains("commune")) {
            auto commune = station["city"]["commune"];
            std::cout << "  Gmina: " << commune["communeName"] << "\n";
            std::cout << "  Powiat: " << commune["districtName"] << "\n";
            std::cout << "  Wojewodztwo: " << commune["provinceName"] << "\n";
        }
    }
    if (station.contains("addressStreet") && !station["addressStreet"].is_null()) {
        std::cout << "  Ulica: " << station["addressStreet"] << "\n";
    }

    std::string sensorData = getSensorData(curl, station["id"]);
    try {
        json sensors = json::parse(sensorData);
        std::cout << "\nStanowiska pomiarowe:\n";
        for (const auto& sensor : sensors) {
            std::cout << "  ID stanowiska: " << sensor["id"] << "\n";
            std::cout << "  ID stacji: " << sensor["stationId"] << "\n";
            if (sensor.contains("param")) {
                auto param = sensor["param"];
                std::cout << "  Parametr:\n";
                std::cout << "    Nazwa: " << param["paramName"] << "\n";
                std::cout << "    Symbol: " << param["paramFormula"] << "\n";
                std::cout << "    Kod: " << param["paramCode"] << "\n";
                std::cout << "    ID parametru: " << param["idParam"] << "\n";
                
                std::string measurementData = getMeasurementData(curl, sensor["id"]);
                json measurements = json::parse(measurementData);
                if (measurements.contains("values") && !measurements["values"].empty()) {
                    std::cout << "    Ostatni pomiar:\n";
                    auto lastValue = measurements["values"][0];
                    std::cout << "      Data: " << lastValue["date"] << "\n";
                    std::cout << "      Wartosc: " << lastValue["value"] << "\n";
                }
            }
            std::cout << "  ----------------\n";
        }
    } catch (json::exception& e) {
        std::cerr << "Blad parsowania sensorow: " << e.what() << "\n";
    }
    std::cout << "====================================\n";
}

int main() {
    #ifdef _WIN32
        system("chcp 65001 > nul");
    #endif
    
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();

    if (!curl) {
        std::cerr << "Blad inicjalizacji CURL\n";
        return 1;
    }

    std::string location;
    std::cout << "Podaj lokalizacje (np. Walcz ul. Poludniowa 10): ";
    std::getline(std::cin, location);

    std::string url = "https://api.gios.gov.pl/pjp-api/rest/station/findAll";
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
    curl_easy_setopt(curl, CURLOPT_CAINFO, "C:/curl/bin/curl-ca-bundle.crt");

    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        std::cerr << "Blad: " << curl_easy_strerror(res) << "\n";
        curl_easy_cleanup(curl);
        curl_global_cleanup();
        return 1;
    }

    try {
        json stations = json::parse(readBuffer);

        std::cout << "\nWyniki dla lokalizacji: " << location << "\n";
        std::cout << "====================================\n";

        double locationLat, locationLon;
        if (getLocationCoordinates(curl, location, locationLat, locationLon)) {
            double minDistance = std::numeric_limits<double>::max();
            json closestStation;

            for (const auto& station : stations) {
                double stationLat = std::stod(station["gegrLat"].get<std::string>());
                double stationLon = std::stod(station["gegrLon"].get<std::string>());
                double distance = calculateDistance(locationLat, locationLon, stationLat, stationLon);
                
                if (distance < minDistance) {
                    minDistance = distance;
                    closestStation = station;
                }
            }

            std::cout << "Najblizsza stacja (odleglosc: " << minDistance << " km):\n";
            displayStationDetails(curl, closestStation, minDistance);
        } else {
            std::cout << "Nie znaleziono adresu dla " << location << " w Polsce\n";
        }
    } catch (json::exception& e) {
        std::cerr << "Blad parsowania JSON: " << e.what() << "\n";
    }

    curl_easy_cleanup(curl);
    curl_global_cleanup();
    return 0;
}
