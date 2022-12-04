#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Firebase_ESP_Client.h>
#include <Wire.h>
#include "WiFi.h"
#include "time.h"

// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

int mass = 1.48; //weight of ball for physics equations in kg change depending on ball

// deleted all wifi credentials for security since its going on github 

//WIFI Credentials @ UP
//const char* SSID = "";

// Wifi @ Home
const char* SSID = "";
const char* password = "";

//API KEY For Firebase project
#define API_KEY "AIzaSyC_QxZiIZeQvNxDG7v0B9MAeYlkFCVfPg0"

// Authentication for firebase, to not have random people getting into our database
// deleted as well for security
#define USER_EMAIL ""
#define USER_PASSWORD ""

//Link to our database
#define DATABASE_URL "https://iot-ball-ab2f2-default-rtdb.firebaseio.com/"

//Defines our accelerometer
Adafruit_MPU6050 mpu;

// Define Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Variable to save USER UID
String uid;

// Database main path (to be updated in setup with the user UID)
String databasePath;

// Database child nodes
String accelPathX = "/accelerationX";
String accelPathY = "/accelerationY";
String accelPathZ = "/accelerationZ";
String totForce = "/totalForce";

String rotationPath = "/rotation";
String timePath = "/timestamp";

// Parent Node (to be updated in every loop)
String parentPath;

int timestamp;
FirebaseJson json;

const char* ntpServer = "pool.ntp.org"; // online server for time 

// Timer variables (send new readings every three minutes)
unsigned long sendDataPrevMillis = 0;
unsigned long timerDelay = 180000;

//Function to get time in epoc (time in seconds since 1970)
unsigned long getTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return (0);
  }
  time(&now);
  return now;
}

void setup(void) {
  Serial.begin(115200);

  // try to connect to wifi
  
  //WiFi.begin(SSID); // starts the connection @UP
  WiFi.begin(SSID,password); // starts the connection @home
  while (WiFi.status() != WL_CONNECTED) { // while not connected
    delay(500);
    Serial.println("Connecting to WiFi.."); // print connecting
  }
  Serial.println("Connected to the WiFi network"); // when it connects print connected

  // Try to initialize the mpu chip
  if (!mpu.begin()) { // if not initialized
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }
  Serial.println("MPU6050 Found!"); // successfully started the chip

  // Set up The Ranges on MPU
  // the lower the ranges the more sensitive it is
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G); // this sets the max range for accel

  mpu.setGyroRange(MPU6050_RANGE_500_DEG); // this sets the max range for gyroscore

  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ); // this is a filter for smoothing out high frequency noise

  // configure the time
  configTime(0, 0, ntpServer);

  //firebase setup
  config.api_key = API_KEY;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  config.database_url = DATABASE_URL;

  // Assign the callback function for the long running token generation task
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h

  // Assign the maximum retry of token generation
  config.max_token_generation_retry = 5;

  // Initialize the library with the Firebase authen and config
  Firebase.begin(&config, &auth);

  // Getting the user UID might take a few seconds
  Serial.println("Getting User UID");
  while ((auth.token.uid) == "") {
    Serial.print('.');
    delay(1000);
  }

  uid = auth.token.uid.c_str();
  Serial.print("User UID: ");
  Serial.print(uid);

  // Update database path
  databasePath = "/UsersData/" + uid + "/readings";

  Serial.println("");
  Serial.println("SET UP COMPLETED");
  Serial.println("");
}

void loop() {

  /* Get new sensor events with the readings */
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  //Get current time
  timestamp = getTime();
  Serial.print ("time: ");
  Serial.println (timestamp);

  //Set the path for database with time each time
  parentPath = databasePath + "/" + String(timestamp);
  
  // this is the calculation of the full magnitude of acceleration, adding all vectors and applying the math
  int fullAcc = sqrt((sq(a.acceleration.x) + sq(a.acceleration.y) + sq(a.acceleration.z))) ; 
  int Force = mass * fullAcc;// f = ma from physics 

  //set the info for firebase
  json.set(accelPathX.c_str(), String(a.acceleration.x));
  json.set(accelPathY.c_str(), String(a.acceleration.y));
  json.set(accelPathZ.c_str(), String(a.acceleration.z));
  json.set(totForce.c_str(), int(Force));

  // log to firebase if enough force applied
  if(Force >= 10){
    Serial.printf("Set json... %s\n", Firebase.RTDB.setJSON(&fbdo, parentPath.c_str(), &json) ? "ok" : fbdo.errorReason().c_str());
  }
  delay(500);// speed you want to log at 
}
