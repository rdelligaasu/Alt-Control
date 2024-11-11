//Shannon's new send IMU data from microcontroller to Unity



#include <WiFi.h>
#include <WiFiUdp.h>
#include <elapsedMillis.h>
#include <Adafruit_BNO08x.h> //for IMU

//IMU
#define BNO08X_CS 10
#define BNO08X_INT 9
#define BNO08X_RESET -1
Adafruit_BNO08x  bno08x(BNO08X_RESET);
sh2_SensorValue_t sensorValue;

//Union Tempe Wi-Fi
/*
char ssid[] = "WhiteSky-TheUnion";    // Set your Wi-Fi SSID
char password[] = "dsvk6gty";    // Set your Wi-Fi password
int status = WL_IDLE_STATUS;        // Indicator of Wi-Fi status
*/

//MIX Center
char ssid[] = "JIGGLING MULE";    // Set your Wi-Fi SSID
char password[] = "4807866177";    // Set your Wi-Fi password
int status = WL_IDLE_STATUS;        // Indicator of Wi-Fi status


WiFiUDP udp;
// const char* udpAddress = "192.168.0.8";  // IP address of the computer running Unity
const char* udpAddress = "192.168.1.73";  // IP address of the computer running Unity
const int udpPort = 4211;

// Adjust these pin numbers according to the Feather ESP32-S3 pinout
//const int buttonPin = 13;  // Example: Change this based on Feather ESP32-S3 pinout
//const int potPin = A0;     // Adjust if A0 is mapped differently on the ESP32-S3
//const int ledPin = 5;

// sensor/data variables
//int ledBrightness;
//int buttonState = 12;
// int potValue = 25;
float posX, posY, posZ;  // position data from tracked Unity object

// timers
elapsedMillis sensorReadTimer;
elapsedMillis sendToUnityTimer;
elapsedMillis readFromUnityTimer;
elapsedMillis setActuatorsTimer;
elapsedMillis printTimer;

long sensorReadInterval = 40;
long sendToUnityInterval = 40;
long readFromUnityInterval = 40;
long setActuatorsInterval = 40;
long printInterval = 250;

//Shannon's IMU variables
float iValIMU = 0.0f; //stores the float value read from the IMU
float jValIMU = 0.0f;
float kValIMU = 0.0f;
int iTemp = 0; //stores the IMU value converted to an int to be sent to Unity
int jTemp = 0;
int kTemp = 0;


void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);
  /*
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(potPin, INPUT);
  pinMode(ledPin, OUTPUT);
  */

  // Attempt to connect to Wi-Fi network
  Serial.print("Connecting to Wi-Fi: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  // Wait for the connection to establish
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  Serial.println("\nWi-Fi connected.");
  printWifiData();

  Serial.println("Now testing IMU");
  // Try to initialize!
  if (!bno08x.begin_I2C()) { //using I2C
    Serial.println("Failed to find BNO08x chip");
    while (1) { delay(10); }
  }
  Serial.println("BNO08x Found!");

  for (int n = 0; n < bno08x.prodIds.numEntries; n++) {
    Serial.print("Part ");
    Serial.print(bno08x.prodIds.entry[n].swPartNumber);
    Serial.print(": Version :");
    Serial.print(bno08x.prodIds.entry[n].swVersionMajor);
    Serial.print(".");
    Serial.print(bno08x.prodIds.entry[n].swVersionMinor);
    Serial.print(".");
    Serial.print(bno08x.prodIds.entry[n].swVersionPatch);
    Serial.print(" Build ");
    Serial.println(bno08x.prodIds.entry[n].swBuildNumber);
  }

  setReports();

  Serial.println("Reading events");
  delay(100);

}

// Here is where you define the sensor outputs you want to receive
void setReports(void) {
  Serial.println("Setting desired reports");
  if (! bno08x.enableReport(SH2_GAME_ROTATION_VECTOR)) {
    Serial.println("Could not enable game vector");
  }
}

void loop() {
  readSensorInput();
  sendDataToUnity();
  readDataFromUnity();
  //setActuatorOutputs();
  //printIncomingData();

  //Serial.println("LOOP() COMPLETED !!!!!!!!");
}

//Shannon's Senor Input function
void readSensorInput() {
  if (sensorReadTimer >= sensorReadInterval) {
    sensorReadTimer = 0;
    //Serial.println("In readSensorInputTest()");
    readFromIMU(); //read input from IMU
    updateIMUIntVals(); //store int versions of the IMU float data
  }
}

void readFromIMU(){ //reads and prints IMU data
  if (bno08x.wasReset()) {
      Serial.print("sensor was reset ");
      setReports();
  }
    
  if (! bno08x.getSensorEvent(&sensorValue)) {
      return;
  }
  //Serial.println("Now reading from IMU");
  switch (sensorValue.sensorId) {

      //read, store, and print i, j, & k float values from the IMU
      case SH2_GAME_ROTATION_VECTOR:
        iValIMU = sensorValue.un.gameRotationVector.i;
        Serial.print("i: ");
        Serial.print(iValIMU);
        
        jValIMU = sensorValue.un.gameRotationVector.j;
        Serial.print(" j: ");
        Serial.print(jValIMU);

        kValIMU = sensorValue.un.gameRotationVector.k;
        Serial.print(" k: ");
        Serial.println(kValIMU);
        break;
  }

}

void updateIMUIntVals() { //convert the IMU float values to int values and store them in their respective Temp variables
  iTemp = (int) (iValIMU * 100.0f);
  jTemp = (int) (jValIMU * 100.0f);
  kTemp = (int) (kValIMU * 100.0f);
}

//-----------------------------------------
void sendDataToUnity() {
  if (sendToUnityTimer >= sendToUnityInterval) {
    sendToUnityTimer = 0;
    char packetBuffer[50];

    //construct & send message using the int versions of the IMU values
    //sprintf(packetBuffer, "i: %d, j: %d", iTemp, jTemp); //WORKS!!!! NOW ADD K !!!!!!!!!
    sprintf(packetBuffer, "i: %d, j: %d, k: %d", iTemp, jTemp, kTemp); //WORKS!!!!!! 
    udp.beginPacket(udpAddress, udpPort);
    udp.write((uint8_t*)packetBuffer, strlen(packetBuffer));
    udp.endPacket();
  }
}

void readDataFromUnity() {
  if (readFromUnityTimer >= readFromUnityInterval) {
    readFromUnityTimer = 0; //Added this

    int packetSize = udp.parsePacket();
    if (packetSize) {
      char packetBuffer[255];
      int len = udp.read(packetBuffer, 255);
      if (len > 0) {
        packetBuffer[len] = 0;
      }

      Serial.println(packetBuffer); // Uncomment to display received position data

      // Parse position data
      sscanf(packetBuffer, "X:%f,Y:%f,Z:%f", &posX, &posY, &posZ);
    }
  }
}

/* Not currently using these functions: 
void setActuatorOutputs() {
  if (setActuatorsTimer >= setActuatorsInterval) {
    setActuatorsTimer = 0;
    ledBrightness = abs((int)posY % 255);
    analogWrite(ledPin, ledBrightness);
  }
}

void printIncomingData() {
  if (printTimer >= printInterval) {
    printTimer = 0;
    Serial.print("Position - X: ");
    Serial.print(posX);
    Serial.print(" Y: ");
    Serial.print(posY);
    Serial.print(" Z: ");
    Serial.println(posZ);
  }
}
*/

void printWifiData() {
  // Print the Wi-Fi IP address
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // Print the subnet mask
  IPAddress subnet = WiFi.subnetMask();
  Serial.print("NetMask: ");
  Serial.println(subnet);

  // Print the gateway address
  IPAddress gateway = WiFi.gatewayIP();
  Serial.print("Gateway: ");
  Serial.println(gateway);
  Serial.println();
}
