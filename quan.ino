#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>



ESP8266WebServer server(80);

// Replace with your network credentials
const char* ssid = "SSID";
const char* password = "PASSWORD";

// Replace with your server address and port
String serverAddress = "http://your-server-address/api/";


// defines pins numbers
const int trigPin = 2;  //D4
const int echoPin = 0;  //D3

// defines variables
long duration;
int distance;

int prevDistance = -1;
int prevOccupancyPercentage = -1;

String savedSensorKey;
int savedTotalCapacity = -1;
String jsonString;



void setup() {

  WiFi.mode(WIFI_AP);
  WiFi.softAP("Portal", "222222222");

  pinMode(trigPin, OUTPUT);  // Sets the trigPin as an Output
  pinMode(echoPin, INPUT);   // Sets the echoPin as an Input

  Serial.begin(9600);  // Starts the serial communication




  server.on("/", handleRoot);
  server.on("/sensor", handleSensor);
  server.on("/submit", handleSubmit);
  server.on("/home", handleHome);
  server.begin();
}

void loop() {

  server.handleClient();

  // Clears the trigPin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);

  // Calculating the distance
  distance = duration * 0.034 / 2;
  // Prints the distance on the Serial Monitor
  Serial.print("Distance: ");
  Serial.println(distance);


  if (savedTotalCapacity != -1) {

    int occupancyPercentage = 100 - (int)((float)distance / savedTotalCapacity * 100);



    // Ensure occupancyPercentage is not negative
    occupancyPercentage = occupancyPercentage < 0 ? 0 : occupancyPercentage;

    if (occupancyPercentage != prevOccupancyPercentage) {
      String status;
      if (occupancyPercentage == 0) {
        status = "completely empty";
      } else if (occupancyPercentage <= 10) {
        status = "almost empty";
      } else if (occupancyPercentage <= 30) {
        status = "low";
      } else if (occupancyPercentage <= 50) {
        status = "half-full";
      } else if (occupancyPercentage <= 80) {
        status = "almost full";
      } else {
        status = "full";
      }
      updateStatusRequest(status);
      createSensorRecordRequest(distance);
      updateStorageTankRequest(occupancyPercentage);


      prevOccupancyPercentage = occupancyPercentage;
    } else {
      prevOccupancyPercentage = occupancyPercentage;
    }

    Serial.print("occupancyPercentage: ");
    Serial.println(occupancyPercentage);
    Serial.print("savedTotalCapacity: ");
    Serial.println(savedTotalCapacity);
  }

  delay(2000);
}


// requests
void updateStatusRequest(String status) {
  Serial.println("Sending request...");
  WiFiClient client;
  HTTPClient http;
  String sensorId = getValueFromJsonString(jsonString, "id");
  http.begin(client, serverAddress + "sensors/" + sensorId);
  http.addHeader("Content-Type", "application/json");
  String jsonData = "{\"status\": \"" + status + "\"}";
  int httpResponseCode = http.PATCH(jsonData);
  if (httpResponseCode > 0) {
    Serial.print("HTTP response code: ");
    Serial.println(httpResponseCode);
  } else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  http.end();
}

void createSensorRecordRequest(int distance) {
  Serial.println("Sending request...");
  WiFiClient client;
  HTTPClient http;
  http.begin(client, serverAddress + "sensor-records");
  http.addHeader("Content-Type", "application/json");

  String jsonData = "{\"sensorKey\":\"" + savedSensorKey + "\",\"distance\":" + distance + "}";

  int httpResponseCode = http.POST(jsonData);
  if (httpResponseCode > 0) {
    Serial.print("HTTP response code: ");
    Serial.println(httpResponseCode);
  } else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  http.end();
}

void updateStorageTankRequest(int occupancyPercentage) {
  Serial.println("Sending request...");
  WiFiClient client;
  HTTPClient http;
  String storageTankString = getValueFromJsonString(jsonString, "storageTank");
  int startIndex = storageTankString.indexOf(":") + 1;
  String storageTankId = storageTankString.substring(startIndex);
  http.begin(client, serverAddress + "sensors/update-percentage/" + storageTankId);
  http.addHeader("Content-Type", "application/json");
  String jsonData = "{\"occupancyPercentage\":" + String(occupancyPercentage) + "}";
  int httpResponseCode = http.PATCH(jsonData);
  if (httpResponseCode > 0) {
    Serial.print("HTTP response code: ");
    Serial.println(httpResponseCode);
  } else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  http.end();
}


//handles
void handleRoot() {
  String html = "<!DOCTYPE html><html lang='en'> <head> <meta charset='utf-8'/> <meta name='viewport' content='width=device-width, initial-scale=1'/> <title>Quanterra Setup</title> <style>*, ::after, ::before{box-sizing: border-box;}body{margin: 0; font-family: 'Segoe UI', Roboto, 'Helvetica Neue', Arial, 'Noto Sans', 'Liberation Sans'; font-size: 1rem; font-weight: 400; line-height: 1.5; color: #424143; background-color: #f5f5f5;}.form-control{display: block; width: 100%; height: calc(1.5em + 0.75rem + 2px); border: 1px solid #ced4da;}.button{position: relative; cursor: pointer; border: 1px solid transparent; color: #fff; background-color: #167eb3; border-color: #167eb3; padding: 0.5rem 1rem; font-size: 1.25rem; line-height: 1.5; border-radius: 0.3rem; width: 100%;}.button--disable{pointer-events: none; background: #167eb3;}.button:active{background: #167eb3;}.button__text{color: #ffffff; transition: all 0.2s; letter-spacing: 1px;}.button--loading .button__text{visibility: hidden; opacity: 0;}.button--loading::after{content: ''; position: absolute; width: 16px; height: 16px; top: 0; left: 0; right: 0; bottom: 0; margin: auto; border: 4px solid transparent; border-top-color: #ffffff; border-radius: 50%; animation: button-loading-spinner 1s ease infinite;}@keyframes button-loading-spinner{from{transform: rotate(0turn);}to{transform: rotate(1turn);}}.form-signin{width: 100%; max-width: 400px; padding: 15px; margin: auto;}h1{text-align: center;}.company-name{background: -webkit-linear-gradient( 90deg, rgba(22, 126, 179, 1) 5%, rgba(136, 178, 80, 1) 100% ); -webkit-background-clip: text; -webkit-text-fill-color: transparent;}</style> </head> <body> <main class='form-signin'> <form action='/submit' method='post'> <h1> <span class='company-name'>Quanterra</span> Setup </h1> <br/> <div class='form-floating'> <label>SSID</label> <input type='text' class='form-control' name='ssid'/> </div><div class='form-floating'> <br/> <label>Password</label> <input type='password' class='form-control' name='password'/> </div><br/> <br/> <button type='submit' class='button' onclick='this.classList.add(`button--loading`, `button--disable`);' > <span class='button__text'>Save</span> </button> <p style='text-align: right'> <a href='https://google.com'>quanterra.com</a> </p></form> </main> <script defer> window.addEventListener('blur', function (){var button=document.querySelector('.button'); button.classList.remove('button--loading', 'button--disable');}); </script> </body></html>";

  if (savedSensorKey == "") {
    server.send(200, "text/html", html);
  } else {
    server.sendHeader("Location", "/sensor");
    server.send(302, "text/plain", "");
  }
}

void handleSensor() {
  String html = "<!DOCTYPE html><html lang='en'> <head> <meta charset='utf-8'/> <meta name='viewport' content='width=device-width, initial-scale=1'/> <title>Quanterra Setup</title> <style>*, ::after, ::before{box-sizing: border-box;}body{margin: 0; font-family: 'Segoe UI', Roboto, 'Helvetica Neue', Arial, 'Noto Sans', 'Liberation Sans'; font-size: 1rem; font-weight: 400; line-height: 1.5; color: #424143; background-color: #f5f5f5;}.form-control{display: block; width: 100%; height: calc(1.5em + 0.75rem + 2px); border: 1px solid #ced4da;}.button{position: relative; cursor: pointer; border: 1px solid transparent; color: #fff; background-color: #167eb3; border-color: #167eb3; padding: 0.5rem 1rem; font-size: 1.25rem; line-height: 1.5; border-radius: 0.3rem; width: 100%;}.button--disable{pointer-events: none; background: #167eb3;}.button:active{background: #167eb3;}.button__text{color: #ffffff; transition: all 0.2s; letter-spacing: 1px;}.button--loading .button__text{visibility: hidden; opacity: 0;}.button--loading::after{content: ''; position: absolute; width: 16px; height: 16px; top: 0; left: 0; right: 0; bottom: 0; margin: auto; border: 4px solid transparent; border-top-color: #ffffff; border-radius: 50%; animation: button-loading-spinner 1s ease infinite;}@keyframes button-loading-spinner{from{transform: rotate(0turn);}to{transform: rotate(1turn);}}.form-signin{width: 100%; max-width: 400px; padding: 15px; margin: auto;}h1{text-align: center;}h3{text-align: center; font-weight: 400;}.company-name{background: -webkit-linear-gradient( 90deg, rgba(22, 126, 179, 1) 5%, rgba(136, 178, 80, 1) 100% ); -webkit-background-clip: text; -webkit-text-fill-color: transparent;}</style> </head> <body> <main class='form-signin'> <form action='/submit' method='post'> <h1> <span class='company-name'>Quanterra</span> Setup </h1> <h3>Please enter your sensor key</h3> <br/> <div class='form-floating'> <label>Sensor Key</label> <input type='text' class='form-control' name='sensorKey'/> </div><br/> <br/> <button type='submit' class='button' onclick='this.classList.add(`button--loading`, `button--disable`)' > <span class='button__text'>Connect</span> </button> <p style='text-align: right'> <a href='/home'>go to home</a> </p></form> </main> <script defer> window.addEventListener('blur', function (){var button=document.querySelector('.button'); button.classList.remove('button--loading', 'button--disable');}); </script> </body></html>";

  if (WiFi.status() == WL_CONNECTED) {
    server.send(200, "text/html", html);
  } else {
    server.sendHeader("Location", "/");
    server.send(302, "text/plain", "");
  }
}

void handleSubmit() {
  if (server.hasArg("ssid") && server.hasArg("password")) {

    const char* ssid = server.arg("ssid").c_str();
    const char* password = server.arg("password").c_str();

    String message;

    if (connectToWiFi(ssid, password)) {
      // message = "Wifi submitted successfully";
      server.sendHeader("Location", "/sensor");
      server.send(302, "text/plain", "");

    } else {
      message = "Incorrect ssid or password";
    }

    server.send(200, "text/html", responseHtml(message));

  } else if (server.hasArg("sensorKey")) {
    String sensorKey = server.arg("sensorKey");

    String message;

    HTTPClient http;
    WiFiClient client;
    http.begin(client, serverAddress + "sensors/by-key/" + sensorKey);

    int httpResponseCode = http.GET();

    if (httpResponseCode == 200) {
      message = "Sensor key " + sensorKey + " submitted successfully";
      savedSensorKey = sensorKey;

      jsonString = http.getString();

      savedTotalCapacity = getValueFromJsonString(jsonString, "capacity").toInt();

      server.send(200, "text/html", "<script> alert('" + message + "'); window.location.href = '/home'; </script>");

    } else if (httpResponseCode == 404) {
      message = "Sensor not found";
      server.send(200, "text/html", responseHtml(message));
    } else {
      message = "Something went wrong :(";
      server.send(200, "text/html", responseHtml(message));
    }

    http.end();

  } else {
    server.send(400, "text/html", responseHtml("Missing parameters"));
  }
}

void handleHome() {
  String html = "<!DOCTYPE html><html lang='en'> <head> <meta charset='utf-8'/> <meta name='viewport' content='width=device-width, initial-scale=1'/> <title>Quanterra Home</title> <style>*, ::after, ::before{box-sizing: border-box;}body{margin: 0; font-family: 'Segoe UI', Roboto, 'Helvetica Neue', Arial, 'Noto Sans', 'Liberation Sans'; font-size: 1rem; font-weight: 400; line-height: 1.5; color: #424143; background-color: #f5f5f5;}.form-signin{width: 100%; max-width: 400px; padding: 15px; margin: auto;}h3{font-weight: 400;}.company-name{background: -webkit-linear-gradient( 90deg, rgba(22, 126, 179, 1) 5%, rgba(136, 178, 80, 1) 100% ); -webkit-background-clip: text; -webkit-text-fill-color: transparent;}table{border-collapse: collapse; margin: 20px auto; background-color: #fff; box-shadow: 0px 0px 5px rgba(0, 0, 0, 0.3);}th, td{padding: 10px; border: 1px solid #ccc; text-align: left;}th{background-color: #f2f2f2;}.check{display: inline-block; transform: rotate(45deg); height: 24px; width: 12px; border-bottom: 7px solid #78b13f; border-right: 7px solid #78b13f;}.top{text-align: center;}.actions{display: flex; justify-content: center;}.link{position: relative; cursor: pointer; border: 1px solid transparent; color: #fff; background-color: #167eb3; border-color: #167eb3; padding: 0.2rem 0.4rem; font-size: 1rem; line-height: 1.5; border-radius: 0.3rem; text-decoration: none;}.link__text{display: flex; align-items: center; gap: 5px; color: #ffffff; transition: all 0.2s; letter-spacing: 1px;}.edit-pencil{position: relative; display: inline-block; width: 25px; height: 36px; vertical-align: middle; cursor: pointer;}.edit-pencil:before{position: absolute; top: 50%; left: 50%; transform: translate(-2px, -3px) rotate(-45deg); width: 3px; height: 5px; background-color: #ffffff; box-shadow: 1px 0px 0px #ffffff, 2px 0px 0px #ffffff, 3px 0px 0px #ffffff, -1px 0px 0px #ffffff, -2px 0px 0px #ffffff, -3px 0px 0px #ffffff, -3.3px 0px 0px #ffffff, 7px 0px 0px #ffffff; content: '';}.edit-pencil:after{position: absolute; top: 50%; left: 50%; transform: translate(-9px, 3px) rotate(-45deg); font-size: 1px; border: solid 3em transparent; border-left-width: 0; border-right-width: 5em; border-right-color: #ffffff; content: '';}.edit-pencil:hover:before{background-color: #ffffff; box-shadow: 1px 0px 0px #ffffff, 2px 0px 0px #ffffff, 3px 0px 0px #ffffff, -1px 0px 0px #ffffff, -2px 0px 0px #ffffff, -3px 0px 0px #ffffff, -3.3px 0px 0px #ffffff, 7px 0px 0px #ffffff;}.edit-pencil:hover:after{border-right-color: #ffffff;}</style> </head> <body> <main class='form-signin'> <div class='top'> <h1> <span class='company-name'>Quanterra</span> Setup </h1> <h3>Connected Sensor Information</h3> <div class='check'></div></div><br/> <table> <thead> <tr> <th>ID</th> <th>Name</th> <th>Status</th> </tr></thead> <tbody> <tr>";
  html += "<td id='sensor-id'>" + getValueFromJsonString(jsonString, "id") + "</td>";
  html += "<td id='sensor-name'>" + getValueFromJsonString(jsonString, "name") + "</td>";
  html += "<td id='sensor-status'>" + getValueFromJsonString(jsonString, "status") + "</td>";

  html += "</tr></tbody> </table> <br/> <div class='actions'> <a class='link' href='/sensor'> <span class='link__text'> Change sensor <div class='edit-pencil'></div></span> </a> </div></main> </body></html>";

  if (savedSensorKey != "") {
    server.send(200, "text/html", html);
  } else {
    String message = "To access the home page, you must first connect to a sensor.";
    server.send(200, "text/html", "<script> alert('" + message + "'); window.location.href = '/sensor'; </script>");
  }
}


//utils
bool connectToWiFi(const char* ssid, const char* password) {
  // Connect to WiFi
  WiFi.begin(ssid, password);
  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < 5) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
    retries++;
  }
  if (WiFi.status() != WL_CONNECTED) {
    // failed to connect, return false
    Serial.println("Failed to connect to WiFi");
    return false;
  }
  Serial.println("Connected to WiFi");
  return true;
}

String responseHtml(String message) {
  String html = "<!DOCTYPE html><html lang='en'> <head> <meta charset='utf-8'/> <meta name='viewport' content='width=device-width, initial-scale=1'/> <title>Quanterra Setup</title> <style>*, ::after, ::before{box-sizing: border-box;}body{margin: 0; font-family: 'Segoe UI', Roboto, 'Helvetica Neue', Arial, 'Noto Sans', 'Liberation Sans'; font-size: 1rem; font-weight: 400; line-height: 1.5; color: #424143; background-color: #f5f5f5;}.form-signin{width: 100%; max-width: 400px; padding: 15px; margin: auto;}h1{text-align: center;}.company-name{background: -webkit-linear-gradient( 90deg, rgba(22, 126, 179, 1) 5%, rgba(136, 178, 80, 1) 100% ); -webkit-background-clip: text; -webkit-text-fill-color: transparent;}</style> </head> <body> <main class='form-signin'> <h1> <span class='company-name'>Quanterra</span> Setup </h1> <br/> <p style='text-align: center'>";
  html += message;
  html += "<br/> Please try again.";
  html += "</p></main> </body></html>";
  return html;
}

String getValueFromJsonString(String jsonString, String key) {
  String value = "N/A";
  int keyIndex = jsonString.indexOf(key);
  if (keyIndex != -1) {
    int valueIndex = keyIndex + key.length() + 2;
    int valueEndIndex = jsonString.indexOf(",", valueIndex);

    if (valueEndIndex == -1) {
      valueEndIndex = jsonString.indexOf("}", valueIndex);
    }

    if (valueEndIndex != -1) {
      value = jsonString.substring(valueIndex, valueEndIndex);
    }
  }


  int checkQuotes = value.indexOf("\"");

  if (checkQuotes != -1 && checkQuotes == 0) {
    value = value.substring(1, value.length() - 1);
  }

  return value;
}
