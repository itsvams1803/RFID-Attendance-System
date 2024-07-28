#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <WiFiClientSecureBearSSL.h>

//-----------------------------------------
#define RST_PIN  D3
#define SS_PIN   D4
#define BUZZER   D8

//-----------------------------------------
MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;  
MFRC522::StatusCode status;      

//-----------------------------------------
// Be aware of Sector Trailer Blocks
int blockNum = 2;  
// Create another array to read data from Block
// Length of buffer should be 2 Bytes more than the size of Block (16 Bytes)
byte bufferLen = 18;
byte readBlockData[18];

//-----------------------------------------
String card_holder_name;
const String sheet_url = "https://script.google.com/macros/s/AKfycby8I1FgbthhqJYFpo3Bhsw4Kbkf2V9ps4dQHeygyT7H32s5YL5AHjUdDao0A21drpva/exec?name=";

//-----------------------------------------
// Fingerprint for demo URL, Issued On  Monday, October 16, 2023 at 1:32:35 PM
// Expires On  Monday, January 8, 2024 at 1:32:34 PM

const uint8_t fingerprint[50] = {0xb2, 0x9b, 0x1e, 0xc7, 0xe3, 0xb8, 0xf2, 0x26, 0xaa, 0x3a, 0x87, 0x8a, 0xce, 0x2f, 0x6f, 0x9d, 0x5a, 0xd4, 0xe1, 0xf3, 0x27, 0xd8, 0x17, 0xd9, 0xff, 0x08, 0x2a, 0x11, 0x1a, 0x01, 0x02, 0xd3};

//-----------------------------------------
#define WIFI_SSID "WIFI Vamsi"
#define WIFI_PASSWORD "musiclove"

//-----------------------------------------

LiquidCrystal_I2C lcd(0x27, 16, 2);

/****************************************************************************************************
 * setup() function
 ****************************************************************************************************/
void setup() {
  //--------------------------------------------------
  // Initialize serial communications with the PC
  lcd.init();
  lcd.backlight();
  Serial.begin(9600);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("RFID  Attendance");
  lcd.setCursor(5, 1);
  lcd.print("System");
  delay(3000);
  
  // WiFi Connectivity
  Serial.println();
  Serial.print("Connecting to AP");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connecting to AP");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    lcd.setCursor(0, 1);
    lcd.print(".");
    delay(200);
  }
  Serial.println("");
  Serial.println("WiFi connected.");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi connected.");
  delay(2000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Scan your card..");
  delay(2000);
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println();
  //--------------------------------------------------
  // Set BUZZER as OUTPUT
  pinMode(BUZZER, OUTPUT);
  //--------------------------------------------------
  // Initialize SPI bus
  SPI.begin();
}

/****************************************************************************************************
 * loop() function
 ****************************************************************************************************/
void loop() {
  //--------------------------------------------------
  // Initialize MFRC522 Module
  mfrc522.PCD_Init();
  // Look for new cards
  // Reset the loop if no new card is present on RC522 Reader
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return;
  }
  // Select one of the cards
  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }
  // Read data from the same block
  //--------------------------------------------------
  Serial.println();
  Serial.println(F("Reading last data from RFID..."));
  ReadDataFromBlock(blockNum, readBlockData);
  
  // Print the data read from block
  Serial.println();
  Serial.print(F("Last data in RFID:"));
  Serial.print(blockNum);
  Serial.print(F(" --> "));
  for (int j = 0; j < 16; j++) {
    Serial.write(readBlockData[j]);
  }
  Serial.println();
  //--------------------------------------------------
  digitalWrite(BUZZER, HIGH);
  delay(200);
  digitalWrite(BUZZER, LOW);
  delay(200);
  digitalWrite(BUZZER, HIGH);
  delay(200);
  digitalWrite(BUZZER, LOW);
  //--------------------------------------------------
  
  if (WiFi.status() == WL_CONNECTED) {
    //-------------------------------------------------------------------------------
    std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
    //-------------------------------------------------------------------------------
    client->setFingerprint(fingerprint);
    // Or, if you want to ignore the SSL certificate
    // then use the following line instead:
    // client->setInsecure();
    //-----------------------------------------------------------------
    card_holder_name = sheet_url + String((char*)readBlockData);
    card_holder_name.trim();
    Serial.println(card_holder_name);
    lcd.clear();
    lcd.setCursor(4, 0);
    lcd.print("Welcome");
    lcd.setCursor(5, 1);
    lcd.print(String((char*)readBlockData));
    delay(2000);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Scan your card..");
    delay(2000);
    //-----------------------------------------------------------------
    HTTPClient https;
    Serial.print(F("[HTTPS] begin...\n"));
    //-----------------------------------------------------------------

    if (https.begin(*client, (String)card_holder_name)) {
      //-----------------------------------------------------------------
      // HTTP
      Serial.print(F("[HTTPS] GET...\n"));
      // start connection and send HTTP header
      int httpCode = https.GET();
      //-----------------------------------------------------------------
      // httpCode will be negative on error
      if (httpCode > 0) {
        // HTTP header has been sent and Server response header has been handled
        Serial.printf("[HTTPS] GET... code: %d\n", httpCode);
        // file found at server
      }
      //-----------------------------------------------------------------
      else {
        Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
      }
      //-----------------------------------------------------------------
      https.end();
      delay(1000);
    } else {
      Serial.printf("[HTTPS] Unable to connect\n");
    }
  }
}

/****************************************************************************************************
 * ReadDataFromBlock() function
 ****************************************************************************************************/
void ReadDataFromBlock(int blockNum, byte readBlockData[]) { 
  //----------------------------------------------------------------------------
  // Prepare the key for authentication
  // All keys are set to FFFFFFFFFFFFh at chip delivery from the factory
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
  //----------------------------------------------------------------------------
  // Authenticating the desired data block for Read access using Key A
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockNum, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Authentication failed for Read: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  } else {
    Serial.println("Authentication success");
  }
  //----------------------------------------------------------------------------
  // Reading data from the Block
  status = mfrc522.MIFARE_Read(blockNum, readBlockData, &bufferLen);
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Reading failed: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  } else {
    Serial.println("Block was read successfully");  
  }
}
