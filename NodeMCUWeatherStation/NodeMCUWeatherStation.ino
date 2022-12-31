#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <LiquidCrystal.h>
#include <WiFiClient.h>
#include <Arduino_JSON.h>

const String ssid = "";
const String password = "";
const String lat = "";
const String lon = "";
const String apiKey = "";

const String urlBase = "http://api.openweathermap.org/data/2.5/";
String urlWeather = urlBase + "weather?units=metric&lat=" +
                 lat + "&lon=" + lon + "&appid=" + apiKey;
String urlForecast = urlBase + "forecast?units=metric&cnt=2&lat=" +
                 lat + "&lon=" + lon + "&appid=" + apiKey;

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(D0, D1, D5, D6, D7, D8);

const int ledPin = D2;
const int buttonPin = D3;

int page = 0;
const int maxPage = 1; // index for the last page
bool nextPageOnUp = false; // flag for switching to next page when button is released

// Current weather
double currentTemp;
double feelsLikeTemp;
double windSpeed;

// Forecast
double forecastTemp;
double forecastRain;
String forecastDesc;


void setup() {
  Serial.begin(9600); 

  // set pin modes
  pinMode(ledPin, OUTPUT);
  pinMode(buttonPin, INPUT);

  // LED starts off
  digitalWrite(ledPin, LOW);

  // set up the LCD's number of columns and rows
  lcd.begin(16, 2);

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

  // Call APIs
  updateCurrentWeather();
  updateForecast();

  // Display weather screen
  displayCurrentTemp();
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
      }
    }
  }
}


void blink(int interval) {
    digitalWrite(ledPin, LOW);
    delay(interval);
    digitalWrite(ledPin, HIGH);
}


JSONVar getAPIResponse(String url) {
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
    Serial.println(response);
    
    // Free resources
    http.end();

    // Decode JSON response
    JSONVar responseJSON = JSON.parse(response);
    if (JSON.typeof(responseJSON) == "undefined") {
      Serial.println("Parsing input failed!");
    } else {
      // blink after update
      blink(250);
      return responseJSON;
    }
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
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
    JSONVar responseJSON = getAPIResponse(urlWeather);
    currentTemp = responseJSON["main"]["temp"];
    feelsLikeTemp = responseJSON["main"]["feels_like"];
    windSpeed = responseJSON["wind"]["speed"];
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
  JSONVar responseJSON = getAPIResponse(urlForecast);
  JSONVar forecast = responseJSON["list"][1]; // Select second forecast (3-6h from now);
  forecastTemp = forecast["main"]["temp"];
  forecastRain = forecast["rain"]["3h"];
  forecastDesc = JSON.stringify(forecast["weather"][0]["description"]); // Use stringify to avoid unexplained crash during casting
  forecastDesc.replace("\"", ""); // Remove quotation marks from stringify
  forecastDesc[0] = toupper(forecastDesc[0]);
}
