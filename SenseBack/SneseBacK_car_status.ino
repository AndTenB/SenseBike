#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>

const int TRIGGER_PIN1 = 21;
const int ECHO_PIN1 = 20;
const int TRIGGER_PIN2 = 3;
const int ECHO_PIN2 = 10;

const float ACTIVATION_DISTANCE = 200.0; // 100 cm für Sensor2
const float PASSING_DISTANCE_THRESHOLD = 100.0; // 100 cm für Sensor1

enum VehicleState { IDLE, APPROACHING, PASSING };
VehicleState vehicleState = IDLE;

int vehicleCount = 0; // Zähler für die erkannten Fahrzeuge

typedef struct struct_message {
    float distance1;
    float distance2;
    VehicleState vehicleState;
    int vehicleCount;
} struct_message;

struct_message myData;

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    Serial.print("Send Status: ");
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

void setup() {
    Serial.begin(115200);
    pinMode(TRIGGER_PIN1, OUTPUT);
    pinMode(ECHO_PIN1, INPUT);
    pinMode(TRIGGER_PIN2, OUTPUT);
    pinMode(ECHO_PIN2, INPUT);
    WiFi.mode(WIFI_STA);
    delay(1000);
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        return;
    }
    esp_now_register_send_cb(OnDataSent);
    delay(1000);
    esp_now_peer_info_t peerInfo;
    memset(&peerInfo, 0, sizeof(peerInfo));
    uint8_t broadcastAddress[] = {0x84, 0xFC, 0xE6, 0x65, 0x9A, 0xC8}; // Ändere dies zur MAC-Adresse des Empfängers
    memcpy(peerInfo.peer_addr, broadcastAddress, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    esp_now_add_peer(&peerInfo);
}

void loop() {
    // Sensor 2 messen
    float distance2 = measureDistance(TRIGGER_PIN2, ECHO_PIN2);

    delay(100);  // Längere Pause, um Interferenzen zu vermeiden

    // Sensor 1 messen
    float distance1 = measureDistance(TRIGGER_PIN1, ECHO_PIN1);

    // Zustandsmaschine zur Erkennung von Fahrzeugen
    switch (vehicleState) {
        case IDLE:
            if (distance2 <= ACTIVATION_DISTANCE) {
                vehicleState = APPROACHING;
            }
            break;
        
        case APPROACHING:
            if (distance1 <= PASSING_DISTANCE_THRESHOLD) {
                vehicleState = PASSING;
            } else if (distance2 > ACTIVATION_DISTANCE) {
                vehicleState = IDLE;
            }
            break;
        
        case PASSING:
            if (distance1 > PASSING_DISTANCE_THRESHOLD) {
                vehicleState = IDLE;
                vehicleCount++; // Erhöhe den Fahrzeugzähler, wenn ein Überholvorgang abgeschlossen ist
            }
            break;
    }

    myData.distance1 = distance1;
    myData.distance2 = distance2;
    myData.vehicleState = vehicleState;
    myData.vehicleCount = vehicleCount;

    // Ausgabe der Zustände und Entfernungen
    switch (vehicleState) {
        case IDLE:
            Serial.printf("Sensor 2 Distance: %.2f cm, No vehicle approaching or within safe distance. Vehicle Count: %d\n", distance2, vehicleCount);
            break;
        case APPROACHING:
            Serial.printf("Sensor 2 Distance: %.2f cm, Vehicle is approaching. Vehicle Count: %d\n", distance2, vehicleCount);
            break;
        case PASSING:
            Serial.printf("Sensor 1 Distance: %.2f cm, Sensor 2 Distance: %.2f cm, Vehicle is passing. Vehicle Count: %d\n", distance1, distance2, vehicleCount);
            break;
    }

    esp_now_send(0, (uint8_t *) &myData, sizeof(myData));
    delay(200);
}

float measureDistance(int trigPin, int echoPin) {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(10);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    long duration = pulseIn(echoPin, HIGH);
    float distance = duration * 0.034 / 2;
    return distance;
}
