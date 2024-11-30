#include "ESP32_NOW.h"
#include "WiFi.h"

#include <esp_mac.h>  // For the MAC2STR and MACSTR macros

#include <vector>

#include <ESP32Servo.h>
Servo servo1;//initializing servo motors
Servo servo2;
#include <Stepper.h>

const int stepsPerRevolution = 2048;//amount of steps for a full spin on stepper motors
Stepper myStepper(stepsPerRevolution, 13, 14, 12, 27);

//global variables
char receivedMessage[10];
bool messageReceived = false;

//define wifi chanel
#define ESPNOW_WIFI_CHANNEL 6


// Creating a new class that inherits from the ESP_NOW_Peer class is required.

class ESP_NOW_Peer_Class : public ESP_NOW_Peer {
public:
  // Constructor
  ESP_NOW_Peer_Class(const uint8_t *mac_addr, uint8_t channel, wifi_interface_t iface, const uint8_t *lmk) : ESP_NOW_Peer(mac_addr, channel, iface, lmk) {}

  // Destructor
  ~ESP_NOW_Peer_Class() {}

  // Function to register the master peer
  bool add_peer() {
    if (!add()) {
      log_e("Failed to register the broadcast peer");
      return false;
    }
    return true;
  }

  // Function to print the received messages from the master
  void onReceive(const uint8_t *data, size_t len, bool broadcast) {
    Serial.printf("Received a message from master " MACSTR " (%s)\n", MAC2STR(addr()), broadcast ? "broadcast" : "unicast");
    Serial.printf("  Message: %s\n", (char *)data);

    snprintf(receivedMessage, sizeof(receivedMessage), "%s", (char *)data);
    messageReceived = true;
  }
};


// List of all the masters. It will be populated when a new master is registered
std::vector<ESP_NOW_Peer_Class> masters;


// Callback called when an unknown peer sends a message
void register_new_master(const esp_now_recv_info_t *info, const uint8_t *data, int len, void *arg) {
  if (memcmp(info->des_addr, ESP_NOW.BROADCAST_ADDR, 6) == 0) {
    Serial.printf("Unknown peer " MACSTR " sent a broadcast message\n", MAC2STR(info->src_addr));
    Serial.println("Registering the peer as a master");

    ESP_NOW_Peer_Class new_master(info->src_addr, ESPNOW_WIFI_CHANNEL, WIFI_IF_STA, NULL);

    masters.push_back(new_master);
    if (!masters.back().add_peer()) {
      Serial.println("Failed to register the new master");
      return;
    }
  } else {
    // The slave will only receive broadcast messages
    log_v("Received a unicast message from " MACSTR, MAC2STR(info->src_addr));
    log_v("Igorning the message");
  }
}

//function for motors
void motorDrive (int dis) {
    myStepper.step(-dis); //move forward amount of distance
    delay(500);
    servo1.write(90); //hit object off
    servo2.write(80);
    delay(1000);
    servo1.write(0);
    servo2.write(170);
    delay(1000);
    myStepper.step(dis); //move back to original spot
    delay(500);
}

/* Main */

void setup() {
  //servo
  servo1.attach(26);
  servo2.attach(25);
  servo1.write(0);
  servo2.write(170);
  //stepper
  myStepper.setSpeed(10);

  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  // Initialize the Wi-Fi module
  WiFi.mode(WIFI_STA);
  WiFi.setChannel(ESPNOW_WIFI_CHANNEL);
  while (!WiFi.STA.started()) {
    delay(100);
  }

  Serial.println("ESP-NOW Example - Broadcast Slave");
  Serial.println("Wi-Fi parameters:");
  Serial.println("  Mode: STA");
  Serial.println("  MAC Address: " + WiFi.macAddress());
  Serial.printf("  Channel: %d\n", ESPNOW_WIFI_CHANNEL);

  // Initialize the ESP-NOW protocol
  if (!ESP_NOW.begin()) {
    Serial.println("Failed to initialize ESP-NOW");
    Serial.println("Reeboting in 5 seconds...");
    delay(5000);
    ESP.restart();
  }

  // Register the new peer callback
  ESP_NOW.onNewPeer(register_new_master, NULL);

  Serial.println("Setup complete. Waiting for a master to broadcast a message...");
}

void loop() {
  if(messageReceived) {
    if(strcmp(receivedMessage, "metal") == 0) {
      motorDrive(1600); // call func to drive object to position
    }
    else if(strcmp(receivedMessage, "glass") == 0) {
      motorDrive(3250);
    }
    else if(strcmp(receivedMessage, "plastic") == 0) {
      motorDrive(4800); 
    }
    messageReceived = false;
  }
  delay(1000);
}
