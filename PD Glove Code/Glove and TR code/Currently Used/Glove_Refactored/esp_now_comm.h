#ifndef ESP_NOW_COMM_H
#define ESP_NOW_COMM_H

#include "WiFi.h"
#include <esp_now.h>
#include "data_storage.h"

// ESP-NOW destination MAC address
uint8_t broadcastAddress[] = {0x64, 0xB7, 0x08, 0x90, 0x41, 0x58};
esp_now_peer_info_t peerInfo;

bool sending_error = false;
bool RECEIVED_COMMAND = false;

// Updated callback structure for newer ESP-NOW API
void OnDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *incomingData, int len) {
  memcpy(&gloveData, incomingData, sizeof(gloveData));
  RECEIVED_COMMAND = true;
}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
  if (status != ESP_NOW_SEND_SUCCESS) {
    sending_error = true;
  }
}

// Initialize ESP-NOW communication
bool initESPNow() {
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return false;
  }
  
  // Register callbacks
  esp_now_register_recv_cb(OnDataRecv);
  esp_now_register_send_cb(OnDataSent);
  
  // Register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return false;
  }
  
  Serial.println("ESP-NOW initialized successfully");
  return true;
}

// Send data to the peer
bool sendESPNowData() {
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&gloveData, sizeof(gloveData));
  
  if (result == ESP_OK) {
    Serial.println("Sent Successfully");
    return true;
  } else {
    Serial.println("Error sending the data");
    sending_error = true;
    return false;
  }
}

// Enable or disable WiFi as needed
void setWiFiMode(bool enable) {
  if (enable) {
    WiFi.mode(WIFI_STA);
    Serial.println("WiFi activated");
    // Re-initialize ESP-NOW after WiFi is turned on
    initESPNow();
  } else {
    WiFi.mode(WIFI_OFF);
    Serial.println("WiFi is OFF");
  }
}

#endif // ESP_NOW_COMM_H