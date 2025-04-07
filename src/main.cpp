#define LED_PIN GPIO_NUM_2
#define SDA_PIN GPIO_NUM_11
#define SCL_PIN GPIO_NUM_12

#include <WiFi.h>
#include <Arduino_MQTT_Client.h>
#include <ThingsBoard.h>
#include "DHT20.h"
#include "Wire.h"
#include <ArduinoOTA.h>
#include "scheduler.h"

constexpr char WIFI_SSID[] = "Oreki";
constexpr char WIFI_PASSWORD[] = "hardware";

constexpr char TOKEN[] = "vkro43vom3n5p5js6ftl";

constexpr char THINGSBOARD_SERVER[] = "app.coreiot.io";
constexpr char DEVICE_PROFILE[] = "Temperature Sensor";

constexpr uint16_t THINGSBOARD_PORT = 1883U;

constexpr uint32_t MAX_MESSAGE_SIZE = 1024U;
constexpr uint32_t SERIAL_DEBUG_BAUD = 9600U;

constexpr char BLINKING_INTERVAL_ATTR[] = "blinkingInterval";
constexpr char LED_MODE_ATTR[] = "led1Mode";
constexpr char LED_STATE_ATTR[] = "led1State";

volatile bool attributesChanged = false;
volatile int led1Mode = 0;
volatile bool led1State = false;

constexpr uint16_t BLINKING_INTERVAL_MS_MIN = 10U;
constexpr uint16_t BLINKING_INTERVAL_MS_MAX = 60000U;
volatile uint16_t blinkingInterval = 1000U;

uint32_t previousStateChange;

constexpr int16_t telemetrySendInterval = 10000U;
uint32_t previousDataSend;

constexpr std::array<const char *, 2U> SHARED_ATTRIBUTES_LIST = {
  LED_STATE_ATTR,
  BLINKING_INTERVAL_ATTR
};

WiFiClient wifiClient;
Arduino_MQTT_Client mqttClient(wifiClient);
ThingsBoard tb(mqttClient, MAX_MESSAGE_SIZE);

DHT20 dht20;

// RPC_Response setLedSwitchState(const RPC_Data &data) {
//     Serial.println("Received Switch state");
//     bool newState = data;
//     Serial.print("Switch state change: ");
//     Serial.println(newState);
//     digitalWrite(LED_PIN, newState);
//     attributesChanged = true;
//     return RPC_Response("setLedSwitchValue", newState);
// }

// const std::array<RPC_Callback, 1U> callbacks = {
//   RPC_Callback{ "setLedSwitchValue", setLedSwitchState }
// };

void processSharedAttributes(const Shared_Attribute_Data &data) {
  for (auto it = data.begin(); it != data.end(); ++it) {
    if (strcmp(it->key().c_str(), BLINKING_INTERVAL_ATTR) == 0) {
      const uint16_t new_interval = it->value().as<uint16_t>();
      if (new_interval >= BLINKING_INTERVAL_MS_MIN && new_interval <= BLINKING_INTERVAL_MS_MAX) {
        blinkingInterval = new_interval;
        Serial.print("Blinking interval is set to: ");
        Serial.println(new_interval);
      }
    } else if (strcmp(it->key().c_str(), LED_STATE_ATTR) == 0) {
      led1State = it->value().as<bool>();
      digitalWrite(LED_PIN, led1State);
      Serial.print("LED state is set to: ");
      Serial.println(led1State);
    }
  }
  attributesChanged = true;
}

const Shared_Attribute_Callback attributes_callback(&processSharedAttributes, SHARED_ATTRIBUTES_LIST.cbegin(), SHARED_ATTRIBUTES_LIST.cend());
const Attribute_Request_Callback attribute_shared_request_callback(&processSharedAttributes, SHARED_ATTRIBUTES_LIST.cbegin(), SHARED_ATTRIBUTES_LIST.cend());

void InitWiFi() {
  Serial.println("Connecting to AP ...");
  // Attempting to establish a connection to the given WiFi network
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    // Delay 500ms until a connection has been successfully established
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to AP");
}

const bool reconnect() {
  // Check to ensure we aren't connected yet
  const wl_status_t status = WiFi.status();
  if (status == WL_CONNECTED) {
    return true;
  }
  // If we aren't establish a new connection to the given WiFi network
  InitWiFi();
  return true;
}

void task1(){
  Serial.println("Hello Task 1");
}

void task2(){
  Serial.println("Hello Task 2");
}

float temperature = 0;
float humidity = 0;
TaskHandle_t Task_getSampleDHT20_Handle = NULL;
void Task_getSampleDHT20(void *pvParameters) {
  while (1) {
      if (millis() - dht20.lastRead() >= 5000) {
          int status = dht20.read();
          temperature = dht20.getTemperature();
          humidity = dht20.getHumidity();
          Serial.print("Time: ");
          Serial.print(millis() / 1000.0, 2);
          Serial.print(" -> ");
          Serial.print("Temperature: ");
          Serial.println(temperature, 1);
          Serial.print("Time: ");
          Serial.print(millis() / 1000.0, 2);
          Serial.print(" -> ");
          Serial.print("Humidity: ");
          Serial.println(humidity, 1);
          Serial.println("------------------------------");
          tb.sendTelemetryData("temperature", temperature);
          tb.sendTelemetryData("humidity", humidity);
      }
      vTaskDelay(pdMS_TO_TICKS(5000));
  }
}

TaskHandle_t Task_checkWifi_Handle = NULL;
void Task_checkWifi(void *pvParameters) {
  while (1) {
      if (!reconnect()) {
          return;
      }

      if (!tb.connected()) {
        Serial.print("Connecting to: ");
        Serial.print(THINGSBOARD_SERVER);
        Serial.print(" with token ");
        Serial.println(TOKEN);
        if (!tb.connect(THINGSBOARD_SERVER, TOKEN, THINGSBOARD_PORT, DEVICE_PROFILE)) {
            Serial.println("Failed to connect");
            return;
        }

        tb.sendAttributeData("macAddress", WiFi.macAddress().c_str());

        // Serial.println("Subscribing for RPC...");
        // if (!tb.RPC_Subscribe(callbacks.cbegin(), callbacks.cend())) {
        //   Serial.println("Failed to subscribe for RPC");
        //   return;
        // }

        if (!tb.Shared_Attributes_Subscribe(attributes_callback)) {
            Serial.println("Failed to subscribe for shared attribute updates");
            return;
        }

        Serial.println("Subscribe done");

        if (!tb.Shared_Attributes_Request(attribute_shared_request_callback)) {
            Serial.println("Failed to request for shared attributes");
            return;
        }
      }

      if (attributesChanged) {
          attributesChanged = false;
          tb.sendAttributeData(LED_STATE_ATTR, digitalRead(LED_PIN));
      }

      // if (led1Mode == 1 && millis() - previousStateChange > blinkingInterval) {
      //     previousStateChange = millis();
      //     digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      //     Serial.print("LED state changed to: ");
      //     Serial.println(!digitalRead(LED_PIN));
      // }

      if (millis() - previousDataSend > telemetrySendInterval) {
          previousDataSend = millis();

          tb.sendAttributeData("rssi", WiFi.RSSI());
          tb.sendAttributeData("channel", WiFi.channel());
          tb.sendAttributeData("bssid", WiFi.BSSIDstr().c_str());
          tb.sendAttributeData("localIp", WiFi.localIP().toString().c_str());
          tb.sendAttributeData("ssid", WiFi.SSID().c_str());
      }

      tb.loop();
      vTaskDelay(pdMS_TO_TICKS(10000));
  }
}

TaskHandle_t Task_getLedStatus_Handle = NULL;
void Task_getLedStatus(void *pvParameters) {
    while (1) {
        if (led1Mode == 1 && millis() - previousStateChange > blinkingInterval) {
          previousStateChange = millis();
          digitalWrite(LED_PIN, !digitalRead(LED_PIN));
          Serial.print("LED state changed to: ");
          Serial.println(!digitalRead(LED_PIN));
          Serial.println("------------------------------");
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}



void setup() {
    Serial.begin(SERIAL_DEBUG_BAUD);
    pinMode(LED_PIN, OUTPUT);
    delay(1000);
    InitWiFi();

    Wire.begin(SDA_PIN, SCL_PIN);
    dht20.begin();
    
    // SCH_Init();
    // // SCH_Add_Task(task1, 300, 200);
    // // SCH_Add_Task(task2, 200, 500);
    // SCH_Add_Task(Task_getSampleDHT20, 100, 500);

    xTaskCreate(Task_getSampleDHT20, "Task_getSampleDHT20", 5000, NULL, 1, &Task_getSampleDHT20_Handle);
    xTaskCreate(Task_getLedStatus, "Task_getLedStatus", 1000, NULL, 1, &Task_getLedStatus_Handle);
    xTaskCreate(Task_checkWifi, "Task_checkWifi", 10000, NULL, 1, &Task_checkWifi_Handle);
}
void loop() {}
