#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ESP32Servo.h> // Library Servo
#include <Wire.h>
#include <EEPROM.h>
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
#define WIFI_SSID "YLC"
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
void firebaseSetString(String, String);
int firebaseGetInt(String);

// Pin sambungan RFID
#define RST_PIN 17
#define SS_PIN 5

// Pin servo
#define SERVO_PIN 13

// Pin untuk sensor IR
#define IR1_PIN 34
#define IR2_PIN 33

void bukaServo();
void tutupServo();
int findCardIndex(String uid);

MFRC522 rfid(SS_PIN, RST_PIN);
Servo servo; // Objek servo

// Struktur untuk menyimpan data kartu

// Array untuk menyimpan daftar kartu
String registeredCards[5] = {
    "E3B06029",
    "3379D826",
    "A31EDAF",
    "959E9CB",
};
int cardCount = 5; // Jumlah kartu yang terdaftar

// Variabel untuk sisa slot parkir
int slot_terisi = 0;
int temp1 = 0;
int temp2 = 0;

void setup()
{
  Serial.begin(115200);
  SPI.begin();             // Inisialisasi SPI bus
  rfid.PCD_Init();         // Inisialisasi modul RFID
  servo.attach(SERVO_PIN); // Sambungkan servo ke pin
  servo.write(180);         // Pastikan servo berada dalam posisi tertutup (0 derajat)

  pinMode(IR1_PIN, INPUT); // IR1 sebagai input
  pinMode(IR2_PIN, INPUT); // IR2 sebagai input

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

  Serial.println("RFID Reader Siap!");
  Serial.println("Scan kartu untuk mendaftarkan...");
}

void loop()
{
  slot_terisi = firebaseGetInt("Sisa slot parkir");
  if (digitalRead(IR1_PIN) == 0 && temp1 == 0)
  {
    if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial())
    {
      delay(50);
      return;
    }
    // Baca UID kartu
    String uid = "";
    for (byte i = 0; i < rfid.uid.size; i++)
    {
      uid += String(rfid.uid.uidByte[i], HEX);
    }
    uid.toUpperCase(); // Ubah ke huruf besar

    Serial.println("UID Kartu Terbaca: " + uid);
    // Periksa apakah kartu sudah terdaftar

    int index = findCardIndex(uid);
    if (index != -1)
    {
      Serial.println("Kartu sudah terdaftar dengan UID : " + registeredCards[index]);
      rfid.PICC_HaltA();
      rfid.PCD_StopCrypto1();

      delay(100);
      if (slot_terisi >= 0 && slot_terisi < 5)
      {
        temp1 = 1;
        if (temp2 == 0)
        {
          Serial.println("IR 1 DETEKSI");
          bukaServo();
          slot_terisi++;
          firebaseSetInt("Sisa slot parkir", slot_terisi);
        }
        firebaseSetString("Palang", "Buka");
      }
    }
  }
  else if (cardCount < 5)
  {
    Serial.println("Kartu belum terdaftar");
  }
  else
  {
    Serial.println("Kapasitas penuh, tidak dapat mendaftarkan kartu baru.");
  }
  // Periksa sensor IR2 untuk mendeteksi objek
  if (digitalRead(IR2_PIN) == 0 && temp2 == 0 && temp1 == 1)
  {
    Serial.println("IR 2 DETEKSI");
    temp2 = 1;
  }

  // Kondisi untuk menutup servo ketika IR2 tidak mendeteksi objek
  if (digitalRead(IR2_PIN) == 1 && temp2 == 1 && temp1 == 1)
  {
    Serial.println("IR 2 TIDAK DETEKSI");
    tutupServo(); // Menutup servo jika IR2 tidak mendeteksi objek
    temp1 = 0;
    temp2 = 0;
    firebaseSetString("Palang", "Tutup");
    delay(100);
  }

  Serial.print("TEMP1 :");
  Serial.println(temp1);
  Serial.print("TEMP2 :");
  Serial.println(temp2);
  delay(1000);
}

// Fungsi untuk membuka kunci dengan servo
void bukaServo()
{
  servo.write(90); // Posisi 0 derajat untuk membuka
  Serial.println("Kunci terbuka.");
}

// Fungsi untuk menutup kunci dengan servo
void tutupServo()
{
  servo.write(180); // Posisi 90 derajat untuk menutup
  Serial.println("Kunci tertutup.");
}

// Fungsi untuk mencari kartu berdasarkan UID
int findCardIndex(String uid)
{
  for (int i = 0; i < cardCount; i++)
  {
    if (registeredCards[i] == uid)
    {
      return i;
    }
  }
  return -1;
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

void firebaseSetString(String databaseDirectory, String value)
{
  // Write a string on the database path
  if (Firebase.RTDB.setString(&fbdo, databaseDirectory, value))
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

int firebaseGetInt(String databaseDirectory)
{
  if (Firebase.RTDB.getInt(&fbdo, databaseDirectory))
  {
    if (fbdo.dataType() == "int")
    {
      int intValue = fbdo.intData();
      return intValue;
    }
  }
  else
  {
    Serial.println(fbdo.errorReason());
  }
  return 0;
}
