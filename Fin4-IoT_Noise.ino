#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

// Network Parameters
char ssid[] = ""; //SSID of your Wi-Fi router
char pass[] = ""; //Password of your Wi-Fi router
char accessToken1[] = "OamkVMibqXjxZCMZCXfJUVYANwsAIBsc";
char accessToken2[] = "easoZEPZgvYRMWbHmUvsXPwcwhQHAChD";
char pingAddress[] = "http://www.finfour.net/wapi/oracle-ping"; // Server Ping-API address
char verificationAddress[] = "http://www.finfour.net/wapi/asset-block/sensor-verify"; // Claim address
bool verificationSent1 = false;

// Computation Variables
int hvalues[100] = {}; // Stores latest 100 Humidity-Sensor Values
int arrayCounter = 0;
int hval = 0;          // Humidity-Sensor Value
int average = 0;       // Average over last 100 Humidity-Sensor Values
int nvalues[25] = {};
int noise = 0;         // Noise received over 50 ms
int nval = 0;          // Noise-Sensor Value

// Pin Setup
int pushButton = D1;
int sensorPin1 = A0; // Humidity-Sensor
int sensorPin2 = D5; // Noise-Sensor

// Control Variables
int isPinging = 1; 
int allowChange = 0;
int buttonPressed = 0;
int pressedBefore = 0; // Old Button value
int loopCounter = 0;   // Counter for specific offsets
int timeOffset = 500;  // Modulo for resetting loopCounter
int delayTime = 2;    // Delay every loop cycle

void setup()
{
  // Start Serial Monitor (integrated)
  Serial.begin(9600);
  delay(10);

  // Set up Led and Push Button  
  pinMode(pushButton, INPUT_PULLUP); // Pullup-Resistor needed for avoiding floating Pin
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(sensorPin1, INPUT);
  pinMode(sensorPin2, INPUT);


  // Set initial value for Led (1 == Led off, 0 == Led on)
  digitalWrite(LED_BUILTIN, 1);

  
  // Connect to Wi-Fi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to...");
  Serial.println(ssid);

  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("");
  Serial.println("Wi-Fi connected successfully");
  

  // Set Array entries for Average Computation to initial Value received from Humidity-Sensor
  hval = analogRead(sensorPin1);
  for (int i = 0; i < 100; i++) {
    hvalues[i] = hval;
  }
}

void loop () {

  // Read data (analog) from Humidity-Sensor and digitaly from Noise-Sensor
  hval = analogRead(sensorPin1);
  nval = digitalRead(sensorPin2);

  // Check if PushButton is pressed (1 == not pressed, 0 == pressed)
  buttonPressed = digitalRead(pushButton);

  // Allow Change if Button has been released since the last push
  if (buttonPressed != pressedBefore) {
    allowChange = 1;
  } else {
    allowChange = 0;
  }

  // Change state if PushButton is pressed and we allow a change of state
  if (!buttonPressed && allowChange) {
    isPinging = !isPinging;
  }

  // Calculate Average over last 100 values (cyclic Array)
  if (loopCounter % 50 == 0) {
    hvalues[arrayCounter] = hval;
    arrayCounter = ++arrayCounter % 100;
    average = sum(hvalues, 100) / 100;
  }

  nvalues[loopCounter % 25] = nval;

  if (loopCounter % 25 == 24) {
    noise = sum(nvalues, 25);
    Serial.print("Noise = ");
    Serial.println(noise);

    if (noise >= 10) {
      verifyClaim(accessToken2);
    }
  }

  // Executing once per timeOffset * delayTime
  if (!loopCounter) {

    // Print if Device is not connected to Network
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("!!!!  NOT CONNECTED  !!!!");
    }

    // Ping the Server if pinging is enabled
    if (isPinging) {
      ping(accessToken1);
      ping(accessToken2);
    }
    
    // Logging
    Serial.print("Sensor giving back = ");
    Serial.println(hval);
    Serial.print("Average = ");
    Serial.println(average);
  }

  //Serial.print("Noise = ");
  //Serial.println(noise);

  // Update last Button state
  pressedBefore = buttonPressed;

  // Send verification if measured Value from Humidity-Sensor is too far away from Average and if not yet verified (Difference relative to value of Average)
  if (hval < (average - 100*(float(average) / 450)) && not verificationSent1) {

    // Send Verification
    verifyClaim(accessToken1);

    // Logging
    Serial.println("************* CLAB ************* Plant has been watered *** token has been issued ******");
    Serial.println(hval);

    // Turn on Led
    digitalWrite(LED_BUILTIN, 0);
    
    verificationSent1 = 1;

    // Pause program execution after Verification has been sent
    delay(20000);
  }

  // Loop counter to measure and display values at specific times
  loopCounter = ++loopCounter % timeOffset;
  delay(delayTime);
  
}

// Ping Server
void ping(char token[]) {
  
  // Declare object of class HTTPClient
  HTTPClient http;

  // Specify Ping destination
  http.begin(pingAddress);

  // Specify Content type and authenticate as Oracle
  http.addHeader("Content-Type", "application/json");

  // Send Request and receive http Code
  int httpCode = http.POST(String("{\"accessToken\": \"") + token + String("\"}"));

  // Logging
  Serial.println("\n ------ ping! ------ \n");

  // Close connection
  http.end();
}

// Send Verification of Claim to Server 
void verifyClaim(char token[]) {

  // Declare object of class HTTPClient
  HTTPClient http;

  // Specify request destination
  http.begin(verificationAddress);

  // Specify content-type header and "log in" as Oracle
  http.addHeader("Content-Type", "application/json");

  // Actually send the request
  int httpCode = http.POST(String("{\"isAccepted\": true, \"accessToken\": \"") + token + String("\"}"));

  // Get the response payload
  String payload = http.getString();

  // Logging
  Serial.print("httpCode = ");
  Serial.println(httpCode);
  Serial.print("payload is = ");
  Serial.println(payload);

  // Close connection
  http.end();
}

// Compute Sum of values in Array
int sum(int values[], int len) {
  int s = 0;
  for (int i = 0; i < len; i++) {
    s += values[i];
  }

  return s;
}
