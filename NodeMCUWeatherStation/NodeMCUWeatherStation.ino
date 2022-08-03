#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <LiquidCrystal.h>
#include <WiFiClient.h>
#include <Arduino_JSON.h>
#include <Ultrasonic.h>

const char* ssid = "";
const char* password = "";

//Your Domain name with URL path or IP address with path
String urlPath = "";

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(D6, D5, D3, D2, D1, D0);

void setup() {
  Serial.begin(9600); 

  // set up the LCD's number of columns and rows
  lcd.begin(16, 2);

  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  lcd.print("Connecting");
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
  lcd.setCursor(0, 0);
  lcd.print("Connected   ");
}

void loop() {
  //Check WiFi connection status
  if(WiFi.status()== WL_CONNECTED){
    WiFiClient client;
    HTTPClient http;
    
    // Your Domain name with URL path or IP address with path
    http.begin(client, urlPath);
    
    // Send HTTP GET request
    int httpResponseCode = http.GET();
    
    if (httpResponseCode>0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      String response = http.getString();
      //Serial.println(response);

      // Decode JSON response
      JSONVar responseJSON = JSON.parse(response);
      if (JSON.typeof(responseJSON) == "undefined") {
        Serial.println("Parsing input failed!");
      } else {
        double temp = responseJSON["features"][0]["properties"]["value"];
        Serial.print("Temperature: ");
        Serial.print(temp);
        Serial.print("°C");
        Serial.println();

        lcd.setCursor(0, 1);
        lcd.print("Temp: ");
        lcd.print(temp);
        lcd.print(" C ");
      }
    }
    else {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }
    // Free resources
    http.end();
  }
  else {
    Serial.println("WiFi Disconnected");
  }
  
  // Delay until next check
  delay(300000);
}
