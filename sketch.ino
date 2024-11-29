#include <SoftwareSerial.h>
#include <DHT.h>

#define DHT_PIN 7              // GPIO pin for the DHT sensor
#define DHT_TYPE DHT22         // DHT sensor type

#define ssid "Wokwi-GUEST"     // WiFi SSID
#define password ""            // WiFi password

#define serverUrl "iot-temperature-monitoring-mock-server.free.beeceptor.com"  // Beeceptor URL
#define serverUrlConsole "https://app.beeceptor.com/console/iot-temperature-monitoring-mock-server"  // Beeceptor URL Console

DHT dht(DHT_PIN, DHT_TYPE);    // Initialize DHT sensor
SoftwareSerial esp8266(2, 3);  // RX, TX pins for ESP8266 communication

void logMessage(const String& step, const String& message) {
  Serial.print(step + ": ");
  Serial.println(message);
}

void initESP8266() {
  logMessage("Initialization Started", "ESP8266...");

  // Reset ESP8266
  sendATCommand("AT+RST\r\n", 2000, "Resetting ESP8266");
  // Set WiFi mode to Station
  sendATCommand("AT+CWMODE=1\r\n", 1000, "Setting WiFi mode to Station");
  // Connect to WiFi
  sendATCommand("AT+CWJAP=\"" + String(ssid) + "\",\"" + String(password) + "\"\r\n", 5000, "Connecting to WiFi");
  // Get IP address
  sendATCommand("AT+CIFSR\r\n", 1000, "Retrieving IP address");
  
  logMessage("Initialization Completed", "ESP8266");

  logMessage("\nServer URL Console", serverUrlConsole);
}

void sendTemperatureData(float temperature) {
  // Convert temperature to string with 2 decimal precision
  String temperatureStr = String(temperature, 2);
  
  // HTTP POST request structure
  String httpRequest = "POST / HTTP/1.1\r\n";
  httpRequest += "Host: " + String(serverUrl) + "\r\n";
  httpRequest += "Content-Type: application/x-www-form-urlencoded\r\n";
  httpRequest += "Content-Length: " + String(temperatureStr.length()) + "\r\n";
  httpRequest += "Connection: close\r\n\r\n";
  httpRequest += temperatureStr;

  logMessage("HTTP Request", "Sending HTTP request...");

  // Establish TCP connection to server
  sendATCommand("AT+CIPSTART=\"TCP\",\"" + String(serverUrl) + "\",80\r\n", 5000, "Opening TCP connection to server");
  // Send the size of the request
  sendATCommand("AT+CIPSEND=" + String(httpRequest.length()) + "\r\n", 2000, "Sending request size");
  // Send the actual request
  sendATCommand(httpRequest, 2000, "Sending HTTP request body");
}

void sendATCommand(String command, int timeout, const String& description) {

  String response = "";
  esp8266.print(command);
  
  long int time = millis();
  while ((time + timeout) > millis()) {
    while (esp8266.available()) {
      char c = (char)esp8266.read();
      if (isPrintable(c) || c == '\n' || c == '\r') {
        response += c;
      }
    }
  }

  if (response.indexOf("OK") >= 0) {
    logMessage("Response", "SUCCESS");
  } else if (response.indexOf("ERROR") >= 0) {
    logMessage("Response", "ERROR " + response);
  } else {
    logMessage("Response", "RECEIVED");
  }
}

void setup() {
  Serial.begin(115200);
  // Initialize SoftwareSerial for ESP8266
  esp8266.begin(115200); 
  // Initialize DHT sensor
  dht.begin();            
  // Initialize ESP8266
  initESP8266();
}

void loop() {
  delay(2000);

  // Read temperature
  float temperature = dht.readTemperature();
  if (isnan(temperature)) {
    logMessage("Error", "Failed to read temperature!");
    return;
  }
  
  // Log temperature to Serial Monitor
  logMessage("\nTemperature", String(temperature, 2) + "°C");
  
  // Send temperature data to the server
  sendTemperatureData(temperature);
}