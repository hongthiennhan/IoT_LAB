#ifdef ESP8266
#include <ESP8266WiFi.h>
#else
#ifdef ESP32
#include <WiFi.h>
#include <WiFiClientSecure.h>
#endif // ESP32
#endif // ESP8266

#include <Arduino.h>
#include <Arduino_MQTT_Client.h>
#include <Server_Side_RPC.h>
#include <ThingsBoard.h>
#include <DHT20.h>
#include <Wire.h>


// Whether the given script is using encryption or not,
// generally recommended as it increases security (communication with the server is not in clear text anymore),
// it does come with an overhead tough as having an encrypted session requires a lot of memory,
// which might not be avaialable on lower end devices.
#define ENCRYPTED false


// constexpr char WIFI_SSID[] = "ACLAB-IOT";
// constexpr char WIFI_PASSWORD[] = "12345678";
constexpr char WIFI_SSID[] = "nhatvu";
constexpr char WIFI_PASSWORD[] = "25122003";

// See https://thingsboard.io/docs/getting-started-guides/helloworld/
// to understand how to obtain an access token
constexpr char TOKEN[] = "vkro43vom3n5p5js6ftl";

// Thingsboard we want to establish a connection too
constexpr char THINGSBOARD_SERVER[] = "app.coreiot.io";

// MQTT port used to communicate with the server, 1883 is the default unencrypted MQTT port,
// whereas 8883 would be the default encrypted SSL MQTT port
#if ENCRYPTED
constexpr uint16_t THINGSBOARD_PORT = 8883U;
#else
constexpr uint16_t THINGSBOARD_PORT = 1883U;
#endif

// Maximum size packets will ever be sent or received by the underlying MQTT client,
// if the size is to small messages might not be sent or received messages will be discarded
constexpr uint16_t MAX_MESSAGE_SEND_SIZE = 256U;
constexpr uint16_t MAX_MESSAGE_RECEIVE_SIZE = 256U;

// Baud rate for the debugging serial connection.
// If the Serial output is mangled, ensure to change the monitor speed accordingly to this variable
constexpr uint32_t SERIAL_DEBUG_BAUD = 115200U;

#if ENCRYPTED
// See https://comodosslstore.com/resources/what-is-a-root-ca-certificate-and-how-do-i-download-it/
// on how to get the root certificate of the server we want to communicate with,
// this is needed to establish a secure connection and changes depending on the website.
constexpr char ROOT_CERT[] = R"(-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
)";
#endif

constexpr const char RPC_JSON_METHOD[] = "example_json";
constexpr const char RPC_TEMPERATURE_METHOD[] = "example_set_temperature";
constexpr const char RPC_SWITCH_METHOD[] = "setSwitch";
constexpr const char RPC_TEMPERATURE_KEY[] = "temp";
constexpr const char RPC_SWITCH_KEY[] = "switch";
constexpr uint8_t MAX_RPC_SUBSCRIPTIONS = 3U;
constexpr uint8_t MAX_RPC_RESPONSE = 5U;


// Initialize underlying client, used to establish a connection
#if ENCRYPTED
WiFiClientSecure espClient;
#else
WiFiClient espClient;
#endif
// Initalize the Mqtt client instance
Arduino_MQTT_Client mqttClient(espClient);
// Initialize used apis
Server_Side_RPC<MAX_RPC_SUBSCRIPTIONS, MAX_RPC_RESPONSE> rpc;
const std::array<IAPI_Implementation*, 1U> apis = {
    &rpc
};
// Initialize ThingsBoard instance with the maximum needed buffer size
ThingsBoard tb(mqttClient, MAX_MESSAGE_RECEIVE_SIZE, MAX_MESSAGE_SEND_SIZE, Default_Max_Stack_Size, apis);

// Statuses for subscribing to rpc
bool subscribed = false;


/// @brief Initalizes WiFi connection,
// will endlessly delay until a connection has been successfully established

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
#if ENCRYPTED
  espClient.setCACert(ROOT_CERT);
#endif
}

/// @brief Reconnects the WiFi uses InitWiFi if the connection has been removed
/// @return Returns true as soon as a connection has been established again
bool reconnect() {
    // Check to ensure we aren't connected yet
    const wl_status_t status = WiFi.status();
    if (status == WL_CONNECTED) {
        return true;
    }

    // If we aren't establish a new connection to the given WiFi network
    InitWiFi();
    return true;
}

/// @brief Processes function for RPC call "example_json"
/// JsonVariantConst is a JSON variant, that can be queried using operator[]
/// See https://arduinojson.org/v5/api/jsonvariant/subscript/ for more details
/// @param data Data containing the rpc data that was called and its current value
/// @param response Data containgin the response value, any number, string or json, that should be sent to the cloud. Useful for getMethods
// void processGetJson(const JsonVariantConst &data, JsonDocument &response) {
//   Serial.println("Received the json RPC method");

//   // Size of the response document needs to be configured to the size of the innerDoc + 1.
//   StaticJsonDocument<JSON_OBJECT_SIZE(4)> innerDoc;
//   innerDoc["string"] = "exampleResponseString";
//   innerDoc["int"] = 5;
//   innerDoc["float"] = 5.0f;
//   innerDoc["bool"] = true;
//   response["json_data"] = innerDoc;
// }

/// @brief Processes function for RPC call "example_set_temperature"
/// JsonVariantConst is a JSON variant, that can be queried using operator[]
/// See https://arduinojson.org/v5/api/jsonvariant/subscript/ for more details
/// @param data Data containing the rpc data that was called and its current value
/// @param response Data containgin the response value, any number, string or json, that should be sent to the cloud. Useful for getMethods
// void processTemperatureChange(const JsonVariantConst &data, JsonDocument &response) {
//   Serial.println("Received the set temperature RPC method");

//   // Process data
//   const float example_temperature = data[RPC_TEMPERATURE_KEY];

//   Serial.print("Example temperature: ");
//   Serial.println(example_temperature);

//   // Ensure to only pass values do not store by copy, or if they do increase the MaxRPC template parameter accordingly to ensure that the value can be deserialized.RPC_Callback.
//   // See https://arduinojson.org/v6/api/jsondocument/add/ for more information on which variables cause a copy to be created
//   response["string"] = "exampleResponseString";
//   response["int"] = 5;
//   response["float"] = 5.0f;
//   response["double"] = 10.0;
//   response["bool"] = true;
// }

/// @brief Processes function for RPC call "example_set_switch"
/// JsonVariantConst is a JSON variant, that can be queried using operator[]
/// See https://arduinojson.org/v5/api/jsonvariant/subscript/ for more details
/// @param data Data containing the rpc data that was called and its current value
/// @param response Data containgin the response value, any number, string or json, that should be sent to the cloud. Useful for getMethods
// void processSwitchChange(const JsonVariantConst &data, JsonDocument &response) {
//   Serial.println("Received the set switch method");

//   // Process data
//   const bool switch_state = data[RPC_SWITCH_KEY];

//   Serial.print("Example switch state: ");
//   Serial.println(switch_state);

//   response.set(22.02);
// }
// void processSwitchChange(const RPC_Data &data) {
//     return RPC_Response(RPC_SWITCH_METHOD, data);
// }

// Task handles
// TaskHandle_t Task1Handle = NULL;
// TaskHandle_t Task2Handle = NULL;
TaskHandle_t Task_getSampleDHT20_Handle = NULL;
TaskHandle_t Task_sendToServer_Handle = NULL;
TaskHandle_t Task_getTemperature_Handle = NULL;
TaskHandle_t Task_getHumidity_Handle = NULL;
TaskHandle_t Task_setSwitch_Handle = NULL;
TaskHandle_t Task_helloworldTest_Handle = NULL;

uint16_t temperature = 0;
uint16_t humidity = 0;

// DHT20 Sensor
DHT20 DHT;

void Task_getSampleDHT20(void *pvParameters) {
    while (1) {
        if (millis() - DHT.lastRead() >= 3000) {
            int status = DHT.read();
            temperature = DHT.getTemperature();
            humidity = DHT.getHumidity();

            // Serial.print("DHT20 Temperature: ");
            // Serial.print(DHT.getTemperature(), 1);
            // Serial.println(" °C");

            // Serial.print("DHT20 Humidity: ");
            // Serial.print(DHT.getHumidity(), 1);
            // Serial.println(" %");

            // Serial.print("Status: ");
            // switch (status) {
            //     case DHT20_OK:
            //         Serial.println("OK");
            //         break;
            //     case DHT20_ERROR_CHECKSUM:
            //         Serial.println("Checksum error");
            //         break;
            //     case DHT20_ERROR_CONNECT:
            //         Serial.println("Connect error");
            //         break;
            //     case DHT20_MISSING_BYTES:
            //         Serial.println("Missing bytes");
            //         break;
            //     case DHT20_ERROR_BYTES_ALL_ZERO:
            //         Serial.println("All bytes read zero");
            //         break;
            //     case DHT20_ERROR_READ_TIMEOUT:
            //         Serial.println("Read time out");
            //         break;
            //     case DHT20_ERROR_LASTREAD:
            //         Serial.println("Read too fast");
            //         break;
            //     default:
            //         Serial.println("Unknown error");
            //         break;
            // }
            //Serial.println();
        }
        vTaskDelay(pdMS_TO_TICKS(3000));  // Delay 3000ms
    }
}

void Task_getTemperature(void *pvParameters) {
    while (1) {  
        Serial.print("Time: ");
        Serial.print(millis());
        Serial.print(" -> ");
        temperature = DHT.getTemperature();
        Serial.print("DHT20 Temperature: ");
        Serial.print(temperature, 1);
        Serial.println(" °C");
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void Task_getHumidity(void *pvParameters) {
    while (1) {
        Serial.print("Time: ");
        Serial.print(millis());
        Serial.print(" -> ");
        humidity = DHT.getHumidity();
        Serial.print("DHT20 Humidity: ");
        Serial.print(humidity, 1);
        Serial.println(" %");
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void Task_sendToServer(void *pvParameters) {
    while (1) {
        // if (!reconnect()) {
        //     return;
        // }
        if (WiFi.status() == WL_CONNECT_FAILED) {
            Serial.print("Time: ");
            Serial.print(millis());
            Serial.print(" -> ");
            Serial.println("Connection Failed.");
            InitWiFi();
        }
        if (!tb.connected()) {
            // Reconnect to the ThingsBoard server,
            // if a connection was disrupted or has not yet been established
            Serial.printf("Connecting to: (%s) with token (%s)\n", THINGSBOARD_SERVER, TOKEN);
            if (!tb.connect(THINGSBOARD_SERVER, TOKEN, THINGSBOARD_PORT)) {
                Serial.println("Failed to connect");
                // return;
            }
        } else {
            tb.sendTelemetryData("temperature", temperature);
            tb.sendTelemetryData("humidity", humidity);
            tb.sendTelemetryData("long", 106.80633605864662);
            tb.sendTelemetryData("lat", 10.880018410410052);
            Serial.print("Time: ");
            Serial.print(millis());
            Serial.print(" -> ");
            Serial.println("Send data to server.");
        }
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

// void Task_setSwitch(void *pvParameters) {
//     while (1) {
//         if (!reconnect()) {
//             return;
//         }
    
//         if (!tb.connected()) {
//             // Reconnect to the ThingsBoard server,
//             // if a connection was disrupted or has not yet been established
//             Serial.printf("Connecting to: (%s) with token (%s)\n", THINGSBOARD_SERVER, TOKEN);
//             if (!tb.connect(THINGSBOARD_SERVER, TOKEN, THINGSBOARD_PORT)) {
//                 Serial.println("Failed to connect");
//                 return;
//             }
//         }
//         if (!subscribed) {
//             const std::array<RPC_Callback, MAX_RPC_SUBSCRIPTIONS> callbacks = {
//                 // Internal size can be 0, because if we use the JsonDocument as a JsonVariant and then set the value we do not require additional memory
//                 RPC_Callback{ RPC_SWITCH_METHOD,         processSwitchChange }
//             };
//             // Perform a subscription. All consequent data processing will happen in
//             // processTemperatureChange() and processSwitchChange() functions,
//             // as denoted by callbacks array.
//             if (!rpc.RPC_Subscribe(callbacks.cbegin(), callbacks.cend())) {
//                 Serial.println("Failed to subscribe for RPC");
//                 return;
//             }

//             // Serial.println(processSwitchChange);
//             subscribed = true;
//         }
//         vTaskDelay(100);
//     }
// }

void Task_helloworldTest(void *pvParameters) {
    while (1) {
        Serial.print("Time: ");
        Serial.print(millis());
        Serial.print(" -> ");
        Serial.println("helloword");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void setup() {
    Serial.begin(115200);
    Wire.begin();  // Initialize I2C
    InitWiFi();
    // Initialize DHT20
    if (!DHT.begin()) {
        Serial.println("Failed to initialize DHT20 sensor!");
        while (1);
    }
    Serial.println("DHT20 sensor initialized.");

    // Create Tasks
    xTaskCreate(Task_helloworldTest, "Task_helloworldTest", 1000, NULL, 2, &Task_helloworldTest_Handle);
    xTaskCreate(Task_getSampleDHT20, "Task_getSampleDHT20", 3000, NULL, 2, &Task_getSampleDHT20_Handle);
    xTaskCreate(Task_getTemperature, "Task_getTemperature", 2000, NULL, 1, &Task_getTemperature_Handle);
    xTaskCreate(Task_getHumidity, "Task_getHumidity", 2000, NULL, 1, &Task_getHumidity_Handle);
    // xTaskCreate(Task_setSwitch, "Task_setSwitch", 100, NULL, 1, &Task_setSwitch_Handle);
    
    xTaskCreate(Task_sendToServer, "Task_sendToServer", 5000, NULL, 1, &Task_sendToServer_Handle);
}

void loop() {
    // Empty - FreeRTOS handles tasks
}