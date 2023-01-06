#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <LiquidCrystal.h>
#include <WiFiClient.h>

const String ssid = "";
const String password = "";
const String lat = "";
const String lon = "";
const String apiKey = "";
const String busStop = "";
const String trainStation = "";
const int directionFilterSize = ;
const String directionFilter[directionFilterSize] = {};

const String urlOpenWeatherMap = "http://api.openweathermap.org/data/2.5/";
String urlWeather = urlOpenWeatherMap + "weather?units=metric&lat=" +
                 lat + "&lon=" + lon + "&appid=" + apiKey;
String urlForecast = urlOpenWeatherMap + "forecast?units=metric&cnt=2&lat=" +
                 lat + "&lon=" + lon + "&appid=" + apiKey;

const String urlJourneyplanner = "http://xmlopen.rejseplanen.dk/bin/rest.exe/";

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(D0, D1, D5, D6, D7, D8);

const int ledPin = D2;
const int buttonPin = D3;

int page = 0;
const int maxPage = 3; // index for the last page
bool nextPageOnUp = false; // flag for switching to next page when button is released

DynamicJsonDocument doc(10000); // capacity for JSON responses

// Current weather
double currentTemp;
double feelsLikeTemp;
double windSpeed;

// Forecast
double forecastTemp;
double forecastRain;
String forecastDesc;

// Transit
String nextBusTime;
String nextBusDirection;
String nextBusLine;
String nextTrainTime;
String nextTrainDirection;
String nextTrainLine;


void setup() {
  Serial.begin(9600); 

  // set pin modes
  pinMode(ledPin, OUTPUT);
  pinMode(buttonPin, INPUT);

  // LED starts off
  digitalWrite(ledPin, LOW);

  // set up the LCD's number of columns and rows
  lcd.begin(16, 2);

  connectWifi();

  // Call APIs
  updateCurrentWeather();
  displayCurrentTemp(); // Display weather screen as soon as it's available
  updateForecast();
  updateBus();
  updateTrain();
}


void loop() {
  if (digitalRead(buttonPin) == LOW) { // button is pressed
    if (nextPageOnUp == false) {
      nextPageOnUp = true; // switch to next page when button is released
    }
  } else { 
    if (nextPageOnUp == true) { // button is released
      nextPageOnUp = false; // ignore button state until it is pressed again
      page ++;
      page = page % (maxPage + 1); // increment page index but roll over if exceeding the last page
      lcd.clear();
      switch(page) {
        case 0:
          displayCurrentTemp();
          break;
        case 1:
          displayForecast();
          break;
        case 2:
          displayNextBus();
          break;
        case 3:
          displayNextTrain();
          break;
      }
    }
  }
}


void connectWifi() {
  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  lcd.print("Connecting");
  while(WiFi.status() != WL_CONNECTED) {
    // blink
    blink(250);
    delay(250);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
  digitalWrite(ledPin, HIGH); // turn LED on
}


void blink(int interval) {
    digitalWrite(ledPin, LOW);
    delay(interval);
    digitalWrite(ledPin, HIGH);
}


String getAPIResponse(String url) {
  WiFiClient client;
  HTTPClient http;
  
  // Your Domain name with URL path or IP address with path
  http.begin(client, url);
  Serial.println("Making request to: " + url);
  
  // Send HTTP GET request
  int httpResponseCode = http.GET();
  
  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    String response = http.getString();
    //Serial.println(response);
    
    // Free resources
    http.end();

    return response;
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
}


void deserializeJSON(String response) {
    // Parse JSON string and store result in doc
    DeserializationError err = deserializeJson(doc, response);
    if (err) {
      Serial.print(F("deserializeJson() failed with code "));
      Serial.println(err.f_str());
    } else {
      // blink after update
      blink(250);
    }
}


void displayCurrentTemp() {
  if (!currentTemp || !feelsLikeTemp) {
    updateCurrentWeather();
  }
  // Display temperature
  lcd.setCursor(0, 0);
  lcd.print("Currently ");
  lcd.print(currentTemp);
  lcd.setCursor(0, 1);
  lcd.print("Feels like ");
  lcd.print(feelsLikeTemp);
}


void updateCurrentWeather() {
    String response = getAPIResponse(urlWeather);
    deserializeJSON(response);
    currentTemp = doc["main"]["temp"];
    feelsLikeTemp = doc["main"]["feels_like"];
    windSpeed = doc["wind"]["speed"];
    Serial.print("Temperature: ");
    Serial.print(currentTemp);
    Serial.print("°C");
    Serial.println();
}


void displayForecast() {
  if (!forecastTemp || !forecastDesc) {
    updateForecast();
  }
  lcd.setCursor(0, 0);
  lcd.print("Forecast: ");
  lcd.print(forecastTemp);
  lcd.setCursor(0, 1);
  lcd.print(forecastDesc);
}


void updateForecast() {
  String response = getAPIResponse(urlForecast);
  deserializeJSON(response);
  JsonObject forecast = doc["list"][1]; // Select second forecast (3-6h from now);
  forecastTemp = forecast["main"]["temp"];
  forecastRain = forecast["rain"]["3h"];
  forecastDesc = forecast["weather"][0]["description"].as<String>();
  forecastDesc[0] = toupper(forecastDesc[0]);
}


String constructLocationURL(String query) {
  const String base = urlJourneyplanner + "location?format=json&input=";
  return base + query;
}


String constructDepartureURL(String id, int offset) {
  const String base = urlJourneyplanner + "departureBoard?format=json&id=";
  return base + id + "&offsetTime=" + String(offset);
}


void displayNextBus() {
  if (!nextBusTime) {
    updateBus();
  }

  lcd.setCursor(0, 0);
  lcd.print("Next bus ");
  lcd.print(nextBusTime);
  lcd.setCursor(0, 1);
  lcd.print(nextBusLine);
  lcd.print(" ");
  lcd.print(nextBusDirection);
}


void displayNextTrain() {
  if (!nextTrainTime) {
    updateTrain();
  }

  lcd.setCursor(0, 0);
  lcd.print("Next train ");
  lcd.print(nextTrainTime);
  lcd.setCursor(0, 1);
  lcd.print(nextTrainLine);
  lcd.print(" ");
  lcd.print(nextTrainDirection);
}


JsonObject getFirstDeparture(JsonArray departures) {
  // Iterate over departures to find first departure that matches filters
  for (int i = 0; i < departures.size(); i++) {
    JsonObject departure = departures[i];
    String dir = departure["direction"].as<String>();
    for (int j = 0; j < directionFilterSize; j++) {
      if (directionFilter[j] == dir) {
        return departure;
      }
    }
  }
}


void updateBus() {
  String locationURL = constructLocationURL(busStop);
  String locationResponse = getAPIResponse(locationURL);
  deserializeJSON(locationResponse);
  String stopID = doc["LocationList"]["StopLocation"][0]["id"]; // Select best matching stop
  
  String departureURL = constructDepartureURL(stopID, 1);
  String departuresResponse = getAPIResponse(departureURL);
  deserializeJSON(departuresResponse);
  JsonArray departures = doc["DepartureBoard"]["Departure"];
  JsonObject departure = getFirstDeparture(departures);
  nextBusTime = departure["time"].as<String>();
  nextBusLine = departure["line"].as<String>();
  nextBusDirection = departure["direction"].as<String>();
  nextBusDirection.replace("ø", "o");
}


void updateTrain() {
  String locationURL = constructLocationURL(trainStation);
  String locationResponse = getAPIResponse(locationURL);
  deserializeJSON(locationResponse);
  String stopID = doc["LocationList"]["StopLocation"][0]["id"]; // Select best matching stop
  
  String departureURL = constructDepartureURL(stopID, 1);
  String departuresResponse = getAPIResponse(departureURL);
  deserializeJSON(departuresResponse);
  JsonArray departures = doc["DepartureBoard"]["Departure"];
  JsonObject departure = getFirstDeparture(departures);
  nextTrainTime = departure["time"].as<String>();
  nextTrainLine = departure["line"].as<String>();
  nextTrainDirection = departure["direction"].as<String>();
  nextTrainDirection.replace("ø", "o");
}
