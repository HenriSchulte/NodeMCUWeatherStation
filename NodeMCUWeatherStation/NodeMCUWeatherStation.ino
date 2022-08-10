#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <LiquidCrystal.h>
#include <WiFiClient.h>
#include <Arduino_JSON.h>

const char* ssid = "";
const char* password = "";

//Your Domain name with URL path or IP address with path
String urlPath = "";

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(D0, D1, D5, D6, D7, D8);

const int ledPin = D2;
const int buttonPin = D3;

int page = 0;
const int maxPage = 1; // index for the last page
bool nextPageOnUp = false; // flag for switching to next page when button is released

double currentTemp;

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
    digitalWrite(ledPin, HIGH);
    delay(250);
    digitalWrite(ledPin, LOW);
    delay(250);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
  digitalWrite(ledPin, HIGH); // turn LED on

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

void displayCurrentTemp() {
  if (!currentTemp) {
    currentTemp = getCurrentTemp();
  }
  // Display temperature
  lcd.setCursor(0, 0);
  lcd.print("Current temp: ");
  lcd.setCursor(0, 1);
  lcd.print(currentTemp);
  lcd.print(" C ");
  
}

double getCurrentTemp() {
  WiFiClient client;
  HTTPClient http;
  
  // Your Domain name with URL path or IP address with path
  http.begin(client, urlPath);
  
  // Send HTTP GET request
  int httpResponseCode = http.GET();
  
  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    String response = http.getString();
    //Serial.println(response);
    double temp;

    // Decode JSON response
    JSONVar responseJSON = JSON.parse(response);
    if (JSON.typeof(responseJSON) == "undefined") {
      Serial.println("Parsing input failed!");
    } else {
      temp = responseJSON["features"][0]["properties"]["value"];
      Serial.print("Temperature: ");
      Serial.print(temp);
      Serial.print("°C");
      Serial.println();
    }

    // blink after update
    digitalWrite(ledPin, LOW);
    delay(250);
    digitalWrite(ledPin, HIGH);

    return temp;
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();
}

void displayForecast() {
  lcd.setCursor(0, 0);
  lcd.print("Forecast:");
  lcd.setCursor(0, 1);
  lcd.print("Great weather");
}
