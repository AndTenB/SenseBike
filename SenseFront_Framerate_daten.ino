#include <TFT_eSPI.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME680.h>
#include <TinyGPS++.h>
#include "sps30.h"
#include <SD.h>
#include <esp_now.h>
#include <WiFi.h>
#include <esp_sleep.h>

File myFile;

float temperature_offset = -3.0; // Beispiel Offset von 2 Grad

// BME680 Einstellungen
#define BME_SDA 44
#define BME_SCL 43
Adafruit_BME680 bme;

// GPS Einstellungen
#define GPS_RX 17  // Beispiel-Pin, bitte anpassen
#define GPS_TX 18  // Beispiel-Pin, bitte anpassen

TinyGPSPlus gps;
HardwareSerial SerialGPS(1);

#include "hothead.h"
unsigned colour = 0xFFFF;

TFT_eSPI tft = TFT_eSPI();  // Create a display object
TFT_eSprite spr = TFT_eSprite(&tft);  // Create a sprite object

#define DEG2RAD 0.0174532925 // Define a conversion from degrees to radians

#define topbutton 0
#define lowerbutton 14
#define PIN_POWER_ON 15  // LCD and battery Power Enable
#define PIN_LCD_BL 38    // BackLight enable pin (see Dimming.txt)
#define BACKLIGHT_PWM_CHANNEL 0 // PWM channel for the backlight

bool secondScreen = false; // Zustandsvariable für den Bildschirm
int pointerPosition = 0;
float globalPM10Value = 0; // Globale Variable für den PM10-Wert
float globalPM1Value = 0; // Globale Variable für den PM1.0-Wert
float globalPM2_5Value = 0; // Globale Variable für den PM2.5-Wert
float globalPM4Value = 0; // Globale Variable für den PM4.0-Wert
float globalIAQValue = 0; // Globale Variable für den IAQ-Wert
bool showWarning = false;
bool espNowConnected = false; // Verbindungsstatus für ESP-NOW

unsigned int segmentColors[13] = {
  0x07E0, 0x07E0, 0xAFE5, 0xBFE0, 0xFFE0, 0xF7E0, 0xFBE0, 
  0xFDA0, 0xFB60, 0xFB20, 0xFAE0, 0xF800, 0xC800
};

int fileIndex = 0;
String fileName;

// Deep Sleep Variablen
bool buttonPressed = false;
unsigned long buttonPressStartTime = 0;
const unsigned long pressDuration = 3000; // 3 Sekunden

// Aufnahmerate in Millisekunden
unsigned long recordInterval = 10000; // Standard 5 Sekunden
unsigned long lastRecordTime = 0;

typedef struct struct_message {
    float distance1;
    float distance2;
    bool vehicleApproaching;
} struct_message;

struct_message myData;

void setup() {
  pinMode(PIN_POWER_ON, OUTPUT);
  pinMode(PIN_LCD_BL, OUTPUT);
  pinMode(lowerbutton, INPUT_PULLUP);
  pinMode(topbutton, INPUT_PULLUP);
  
//  digitalWrite(PIN_POWER_ON, HIGH);
 //   digitalWrite(PIN_LCD_BL, HIGH);
//   Initialize PWM for backlight control
  ledcAttachPin(PIN_LCD_BL, BACKLIGHT_PWM_CHANNEL);
  ledcSetup(BACKLIGHT_PWM_CHANNEL, 5000, 8); // 5 kHz PWM, 8-bit resolution

  Serial.begin(115200);
  initSerialGPS();
  
  Wire.begin(BME_SDA, BME_SCL);
  if (!bme.begin()) {
    Serial.println("Could not find a valid BME680 sensor, check wiring!");
    while (1);
  }
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150); // 320°C for 150 ms

  sensirion_i2c_init();
  while (sps30_probe() != 0) {
    Serial.println("SPS sensor probing failed");
    delay(500);
  }
  Serial.println("SPS sensor probing successful");
  sps30_set_fan_auto_cleaning_interval_days(4);
  sps30_start_measurement();

 // tft.init();
 // setBacklight(100); // Set initial backlight to 100%
  tft.setRotation(0);
  tft.setSwapBytes(true);
  tft.fillScreen(TFT_WHITE);
  tft.pushImage(10, 90, 155, 170, hothead);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.setFreeFont(&Orbitron_Light_24);
  tft.drawString(" Bike Sense  ", 0, 35);
  tft.setTextSize(1);
  tft.setTextDatum(4);

  Initial_SD();
  createNewFile();

  displayProgress();
  tft.fillScreen(TFT_BLACK);
  delay(1000);
  spr.createSprite(tft.width(), tft.height());

  // Initialize WiFi and ESP-NOW
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_register_recv_cb(OnDataRecv);

  // Überprüfe, ob der ESP32 aus dem Tiefschlaf erwacht ist
  if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT1) {
    checkButtonPress();
  }
}

void loop() {
  static bool lastButtonStateLower = HIGH;
  static bool lastButtonStateTop = HIGH;
  bool currentButtonStateLower = digitalRead(lowerbutton);
  bool currentButtonStateTop = digitalRead(topbutton);

  // Überprüfe, ob beide Buttons gleichzeitig gedrückt werden
  if (currentButtonStateLower == LOW && currentButtonStateTop == LOW) {
    if (!buttonPressed) {
      buttonPressed = true;
      buttonPressStartTime = millis();
    } else if (millis() - buttonPressStartTime >= pressDuration) {
      enterDeepSleep();
    }
  } else {
    buttonPressed = false;
  }

  if (lastButtonStateLower == HIGH && currentButtonStateLower == LOW) {
    delay(50);
    secondScreen = !secondScreen;
  }
  lastButtonStateLower = currentButtonStateLower;
  lastButtonStateTop = currentButtonStateTop;

  if (millis() - lastRecordTime >= recordInterval) {
    updateSensorData();
    lastRecordTime = millis();
  }
  
  if (!secondScreen) {
    spr.fillSprite(TFT_BLACK);
    drawDial();
    displaySpeed();
  } else {
    displayAdditionalData();
  }

  // Blinke die Warnmeldung mit 1 Hz Frequenz
  if (myData.vehicleApproaching) {
    static unsigned long lastBlinkTime = 0;
    if (millis() - lastBlinkTime >= 500) {
      lastBlinkTime = millis();
      showWarning = !showWarning;
    }
    if (showWarning) {
      displayWarning();
    }
  }

  // Zeige den Verbindungsstatus an
  drawConnectionStatus();

  spr.pushSprite(0, 0);
  delay(100);
}

void Initial_SD(){
  Serial.print("Initializing SD card...");

  if (!SD.begin(10)) {
    Serial.println("initialization failed!");
    while (1);
  }
  Serial.println("initialization done.");
  delay(500);
}

void createNewFile() {
  while (true) {
    fileName = "/datalog" + String(fileIndex) + ".txt";
    if (!SD.exists(fileName)) {
      break;
    }
    fileIndex++;
  }
  myFile = SD.open(fileName, FILE_WRITE);
  if (myFile) {
    myFile.println("Temp;Hum;Speed;Latitude;Longitude;PM1;PM25;PM4;PM10;D1;D2;VehicleApproaching;");
    myFile.close();
  } else {
    Serial.println("Error creating file: " + fileName);
  }
}

void displayProgress() {
  int progress = 0;
  uint16_t colour = TFT_BLUE;

  while (progress <= 100) {
    tft.drawRoundRect(20, 270, 125, 18, 3, TFT_BLUE);
    int blocks = progress / 5;
    for (int i = 0; i < blocks; i++) {
      int x = i * 6 + 23;
      colour = (progress <= 90) ? 0xF800 : 0x07E0;
      tft.fillRect(x, 274, 5, 10, colour);
    }
    progress++;
    delay(50);
  }
  delay(1500);
}

void displaySpeed() {
  bool gpsDataValid = false;

  while (SerialGPS.available() > 0) {
    if (gps.encode(SerialGPS.read())) {
      if (gps.speed.isValid()) {
        gpsDataValid = true;
        float speedKmH = gps.speed.kmph();

        spr.setFreeFont(&FreeMonoBold9pt7b);
        spr.setTextColor(TFT_WHITE, TFT_BLACK);
        String speedText = String(speedKmH, 1) + " km/h";

        int textWidth = spr.textWidth(speedText);
        int textX = (spr.width() - textWidth) / 2 - 10;
        int textY = spr.height() - 70;

        spr.drawString(speedText, textX, textY, 7);
      }
    }
  }

  if (!gpsDataValid) {
    String noSignalText = "Search GPS";

    spr.setFreeFont(&FreeMonoBold9pt7b);
    spr.setTextColor(TFT_WHITE, TFT_BLACK);

    int textWidth = spr.textWidth(noSignalText);
    int textX = (spr.width() - textWidth) / 2 - 20;
    int textY = spr.height() - 30;

    spr.drawString(noSignalText, textX, textY);
  }
}

void displayAdditionalData() {
  spr.fillSprite(TFT_BLACK);
  spr.setTextColor(TFT_WHITE, TFT_BLACK);
  spr.setFreeFont(&FreeMonoBold9pt7b);

  spr.drawString("Temp: " + String(bme.temperature + temperature_offset, 1) + " C", 5, 20);
  spr.drawString("Hum: " + String(bme.humidity, 1) + " %", 5, 50);
  spr.drawString("Pressure: " + String(bme.pressure / 100.0, 1) + " hPa", 5, 80);

  struct sps30_measurement m;
  if (sps30_read_measurement(&m) == 0) {
    spr.drawString("PM1.0: " + String(m.mc_1p0, 1), 5, 110);
    spr.drawString("PM2.5: " + String(m.mc_2p5, 1), 5, 140);
    spr.drawString("PM4.0: " + String(m.mc_4p0, 1), 5, 170);
    spr.drawString("PM10: " + String(m.mc_10p0, 1), 5, 200);
  }
  
  // Zeige den IAQ-Wert an
  spr.drawString("IAQ: " + String(globalIAQValue, 1), 5, 230);
  
  // Zeige GPS-Daten an, falls verfügbar
  if (gps.location.isValid()) {
    spr.drawString("Lat: " + String(gps.location.lat(), 6), 5, 260);
    spr.drawString("Lon: " + String(gps.location.lng(), 6), 5, 290);
  } else {
    spr.drawString("Lat: N/A", 5, 260);
    spr.drawString("Lon: N/A", 5, 290);
  }

  // Zeige ESP-NOW empfangene Daten an
  spr.drawString("D1: " + String(myData.distance1, 2) + " cm", 5, 320);
  spr.drawString("D2: " + String(myData.distance2, 2) + " cm", 5, 350);
  spr.drawString("Vehicle: " + String(myData.vehicleApproaching ? "Yes" : "No"), 5, 380);
}

void displayWarning() {
  spr.fillRect(0, spr.height() / 2 - 50, spr.width(), 100, TFT_RED);
  spr.setTextColor(TFT_WHITE, TFT_RED);
  spr.setFreeFont(&FreeMonoBold12pt7b);
  spr.setTextDatum(MC_DATUM);
  spr.drawString("Vehicle", spr.width() / 2, spr.height() / 2 - 20);
  spr.drawString("Approaching", spr.width() / 2, spr.height() / 2 + 20);
  spr.setTextDatum(TL_DATUM);  // Reset text datum to default
}

void drawDial() {
  int cx = spr.width() / 2;
  int cy = spr.height() / 2 - 30;
  float r1 = min(cx, cy) - 40;
  float r2 = min(cx, cy) - 10;

  for (int a = -120, segmentIndex = 0; a <= 120; a += 20, segmentIndex++) {
    float px1, py1, px2, py2;
    getCoord(cx, cy, &px1, &py1, &px2, &py2, r1, r2, a);

    uint16_t color = (segmentIndex < pointerPosition) ? segmentColors[segmentIndex] : TFT_BLACK;
    spr.drawWedgeLine(px1, py1, px2, py2, r1 / 25, r2 / 20, color, TFT_BLACK);
  }

  spr.fillCircle(cx, cy, r1 - 20, TFT_BLACK);
  displayPM10Value(cx, cy);
}

void getCoord(int16_t x, int16_t y, float *xp1, float *yp1, float *xp2, float *yp2, float r1, float r2, float a) {
  float sx = cos((a - 90) * DEG2RAD);
  float sy = sin((a - 90) * DEG2RAD);
  *xp1 = sx * r1 + x;
  *yp1 = sy * r1 + y;
  *xp2 = sx * r2 + x;
  *yp2 = sy * r2 + y;
}

void updateSensorData() {
  if (!bme.performReading()) {
    Serial.println("Failed to perform reading :(");
    return;
  }

  struct sps30_measurement m;
  if (sps30_read_measurement(&m) == 0) {
    globalPM1Value = m.mc_1p0;
    globalPM2_5Value = m.mc_2p5;
    globalPM4Value = m.mc_4p0;
    globalPM10Value = m.mc_10p0;
    Serial.printf("SPS30 -> PM1.0: %.2f, PM2.5: %.2f, PM4.0: %.2f, PM10: %.2f\n", m.mc_1p0, m.mc_2p5, m.mc_4p0, m.mc_10p0);
    pointerPosition = map(globalPM10Value, 0, 50, 0, 12);
    pointerPosition = constrain(pointerPosition, 0, 12);
  }

  globalIAQValue = calculateIAQ(bme.gas_resistance, bme.humidity);
  saveDataToSD();
}

void displayPM10Value(int cx, int cy) {
  spr.setFreeFont(&FreeMonoBold12pt7b);
  spr.setTextColor(TFT_WHITE, TFT_BLACK);

  String pm10ValueText = String(globalPM10Value, 1);
  int valueTextWidth = spr.textWidth(pm10ValueText);
  spr.drawString(pm10ValueText, cx - valueTextWidth / 2, cy - 20);

  spr.setFreeFont(&FreeMonoBold9pt7b);
  String pm10Text = "PM10";
  int pm10TextWidth = spr.textWidth(pm10Text);
  spr.drawString(pm10Text, cx - pm10TextWidth / 2, cy);
}

float calculateIAQ(float gasResistance, float humidity) {
  // Implementiere hier deinen Algorithmus zur Berechnung des IAQ-Werts
  // Dies ist nur ein Beispiel und kein genauer Algorithmus zur Berechnung des IAQ
  float iaq = gasResistance / 1000.0 * (100 - humidity) / 100.0;
  return iaq;
}

void saveDataToSD() {
  File dataFile = SD.open(fileName, FILE_APPEND);
  if (dataFile) {
    dataFile.print(bme.temperature + temperature_offset);
    dataFile.print(";");
    dataFile.print(bme.humidity);
    dataFile.print(";");

    if (gps.speed.isValid()) {
      dataFile.print(gps.speed.kmph());
    } else {
      dataFile.print("N/A");
    }
    dataFile.print(";");

    if (gps.location.isValid()) {
      dataFile.print(gps.location.lat(), 6);
    } else {
      dataFile.print("N/A");
    }
    dataFile.print(";");

    if (gps.location.isValid()) {
      dataFile.print(gps.location.lng(), 6);
    } else {
      dataFile.print("N/A");
    }
    dataFile.print(";");

    dataFile.print(globalPM1Value);
    dataFile.print(";");
    dataFile.print(globalPM2_5Value);
    dataFile.print(";");
    dataFile.print(globalPM4Value);
    dataFile.print(";");
    dataFile.print(globalPM10Value);
    dataFile.print(";");

    // ESP-NOW Daten
    dataFile.print(myData.distance1);
    dataFile.print(";");
    dataFile.print(myData.distance2);
    dataFile.print(";");
    dataFile.print(myData.vehicleApproaching ? "Yes" : "No");
    dataFile.print(";");

    dataFile.println();
    dataFile.close();
    Serial.println("SensorData Write");
  } else {
    Serial.println("Error opening " + fileName);
  }
}

void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
  memcpy(&myData, incomingData, sizeof(myData));
  Serial.print("Received D1: ");
  Serial.print(myData.distance1);
  Serial.print(" cm, D2: ");
  Serial.print(myData.distance2);
  Serial.print(" cm, Vehicle Approaching: ");
  Serial.println(myData.vehicleApproaching ? "Yes" : "No");
  
  espNowConnected = true; // Setze den Verbindungsstatus auf verbunden
}

void drawConnectionStatus() {
  uint16_t color = espNowConnected ? TFT_GREEN : TFT_RED;
  spr.fillCircle(spr.width() - 20, 20, 10, color);
  espNowConnected = false; // Setze den Verbindungsstatus zurück
}

void checkButtonPress() {
  unsigned long pressStartTime = millis();
  while (millis() - pressStartTime < 3000) {
    if (digitalRead(lowerbutton) == HIGH || digitalRead(topbutton) == HIGH) {
      return; // Wenn eine der Tasten losgelassen wird, breche ab
    }
  }
  // Wenn beide Tasten für 3 Sekunden gedrückt gehalten werden, aktiviere den ESP32
}

void enterDeepSleep() {
  Serial.println("Entering deep sleep...");
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  tft.setFreeFont(&FreeMonoBold9pt7b); // Schriftart auf kleinere Schriftgröße ändern
  tft.drawString("Going to", tft.width() / 2, tft.height() / 2 - 10); // Erste Zeile
  tft.drawString("Sleep...", tft.width() / 2, tft.height() / 2 + 10); // Zweite Zeile
  delay(1000);

  // Deaktiviere die Sensoren
  digitalWrite(PIN_POWER_ON, LOW); // Ausschalten der Stromversorgung der Sensoren
  
  // Setze den Air530 GPS in den Standby-Modus
  SerialGPS.print("$PMTK161,0*28<CR><LF>");
  delay(100);
  endSerialGPS(); // Serielle Kommunikation beenden

  // Setze den SPS30 in den Sleep-Modus

  sps30_stop_measurement();
  
  delay(100);

  sps30_sleep();


  // Konfiguriere den Wakeup-Pin und gehe in den Deep Sleep Modus
  esp_sleep_enable_ext1_wakeup((1ULL << lowerbutton) | (1ULL << topbutton), ESP_EXT1_WAKEUP_ALL_LOW); // Beide Tasten müssen gedrückt werden
  esp_deep_sleep_start();
}

void setBacklight(int brightness) {
  if (brightness == 0) {
    ledcWrite(BACKLIGHT_PWM_CHANNEL, 0); // Schaltet das Backlight aus
  } else {
    int dutyCycle = map(brightness, 0, 100, 0, 255);
    ledcWrite(BACKLIGHT_PWM_CHANNEL, dutyCycle);
  }
}

void initSerialGPS() {
  SerialGPS.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);
  Serial.println("GPS initialized.");
}

void endSerialGPS() {
  SerialGPS.end();
  Serial.println("GPS ended.");
}
