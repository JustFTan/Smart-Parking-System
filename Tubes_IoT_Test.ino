#include <Arduino.h>

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#if defined(ESP32) || defined(ARDUINO_RASPBERRY_PI_PICO_W)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif
/* FIREBASE START */
// FIREBASE LIBRARY
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h" // Provide the token generation process info.
#include "addons/RTDBHelper.h"  // Provide the RTDB payload printing info and other helper functions.
// FIREBASE SETUP
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
bool signupOK = false;
/* 1. Define the WiFi credentials */
#define WIFI_SSID "JFT"
#define WIFI_PASSWORD "26012005"
/* 2. Define the API Key */
#define API_KEY "AIzaSyCm3BK8On4QymCxtK0yCSYxHN7WDS5MCs0"
/* 3. Define the RTDB URL */
#define DATABASE_URL "https://tubes-40c38-default-rtdb.firebaseio.com/"
/* 4. ACTIVATE IT for authenticated account: Define the user Email and
password that alreadey registerd or added in your project */
// #define USER_EMAIL "<YOUR EMAIL HERE>"
// #define USER_PASSWORD "<YOUR PASSWORD HERE>"
// Function List
void firebaseSetInt(String, int);
void updateLCD();
void enterParking();
void exitParking();

// Pin Definitions
const int IR_Entrance = 16;     // Pin untuk sensor masuk
const int IR_Exit = 17;         // Pin untuk sensor keluar
const int Button_Entrance = 23; // Pin untuk tombol masuk
const int Button_Exit = 19;     // Pin untuk tombol keluar
const int LED_PIN = 13;         // Pin untuk LED

// LCD setup
LiquidCrystal_I2C lcd(0x27, 16, 2); // I2C Address 0x27 (sesuaikan jika perlu)

// Variabel
const int totalSlots = 10;
int availableSlots = totalSlots;

void setup()
{
  // Inisialisasi LCD
  Serial.begin(115200);
  // Inisialisasi GPIO
  pinMode(IR_Entrance, INPUT);
  pinMode(IR_Exit, INPUT);
  pinMode(LED_PIN, OUTPUT); // LED sebagai output
  pinMode(Button_Entrance, INPUT_PULLUP);
  pinMode(Button_Exit, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  /* WIFI START */
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
  /* WIFI END */
  /* FIREBASE START */
  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);
  config.api_key = API_KEY; // Assign RTDB API Key
  /*For anonymous account: Sign up */
  if (Firebase.signUp(&config, &auth, "", ""))
  {
    Serial.println("Firebase success");
    digitalWrite(LED_BUILTIN, LOW);
    signupOK = true;
  }
  else
  {
    String firebaseErrorMessage = config.signer.signupError.message.c_str();
    Serial.printf("%s\n", firebaseErrorMessage);
  }
  /* ACTIVATE IT For authenticated account: Assign the user sign in
  credentials */
  // auth.user.email = USER_EMAIL;
  // auth.user.password = USER_PASSWORD;
  config.database_url = DATABASE_URL;                 // Assign rtdb url
  config.token_status_callback = tokenStatusCallback; // Set callback
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  /* FIREBASE END */
  lcd.begin(16, 2);
  lcd.backlight();
  updateLCD();
}

void loop()
{
  // Tombol masuk ditekan
  if (digitalRead(Button_Entrance) == LOW)
  {
    delay(200); // Debounce
    if (availableSlots > 0 && digitalRead(IR_Entrance) == LOW)
    {
      enterParking();
    }
    else
    {
      Serial.println("Parking Full or no car detected!");
    }
  }

  // Tombol keluar ditekan
  if (digitalRead(Button_Exit) == LOW)
  {
    delay(200); // Debounce
    if (availableSlots < totalSlots && digitalRead(IR_Exit) == LOW)
    {
      exitParking();
    }
    else
    {
      Serial.println("No car to exit!");
    }
  }
  firebaseSetInt("Sisa slot parkir", availableSlots);
}

void enterParking()
{
  // Nyalakan LED untuk menunjukkan parkir tersedia
  digitalWrite(LED_PIN, HIGH);
  availableSlots--;
  updateLCD();
  delay(2000);                // Simulasikan kendaraan masuk
  digitalWrite(LED_PIN, LOW); // Matikan LED
}

void exitParking()
{
  // Nyalakan LED untuk menunjukkan parkir tersedia
  digitalWrite(LED_PIN, HIGH);
  availableSlots++;
  updateLCD();
  delay(2000);                // Simulasikan kendaraan keluar
  digitalWrite(LED_PIN, LOW); // Matikan LED
}

void updateLCD()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Slots Avail:");
  lcd.print(availableSlots);
  lcd.setCursor(0, 1);
  lcd.print("Total Slots:");
  lcd.print(totalSlots);

  // Tampilkan informasi di Serial Monitor
  Serial.print("Available Slots: ");
  Serial.println(availableSlots);
  Serial.print("Total Slots: ");
  Serial.println(totalSlots);
}

void firebaseSetInt(String databaseDirectory, int value)
{
  // Write an Int number on the database path test/int
  if (Firebase.RTDB.setInt(&fbdo, databaseDirectory, value))
  {
    Serial.print("PASSED: ");
    Serial.println(value);
  }
  else
  {
    Serial.println("FAILED");
    Serial.println("REASON: " + fbdo.errorReason());
  }
}