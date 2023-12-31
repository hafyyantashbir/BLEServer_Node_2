#include <Arduino.h>

//library RTC
#include <Wire.h>
#include "RTClib.h"

//library Json
#include <ArduinoJson.h>

//Library NRF24L01
#include <SPI.h>
#include <RF24.h>
#include <RF24Network.h>
#include "printf.h"

//Library BLE
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLEAdvertisedDevice.h>

//konfigurasi RTC
RTC_DS3231 rtc;
char days[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

//Konfifurasi BLE
#define SERVICE_UUID "2c87d42d-56a6-4a17-a72a-370785c0042c"
#define CHARACTERISTIC_UUID "b5c358e1-44ab-4c88-a742-fa484a01c383"

//konfigurasi NRF24L01
RF24 radio(4, 5); //(pin CE, pin CSN)
RF24Network network(radio);      // Network uses that radio
uint8_t dataBuffer[MAX_PAYLOAD_SIZE];  //MAX_PAYLOAD_SIZE is defined in RF24Network_config.h
#define LED_BUILTIN 2

//alamat node
const uint16_t this_node = 02;   // alamat node ini (NODE_2) dalam format Octal
const uint16_t NODE_1 = 01;  // Alamat NODE_1 dalam format Octal
const uint16_t NODE_3 = 03; // Alamat NODE_3 dalam format Octal
const uint16_t NODE_4 = 04; // Alamat NODE_4 dalam format Octal
const uint16_t NODE_5 = 00; // Alamat NODE_5 dalam format Octal

//Variabel DATA
int node_asal = 2; //data node
unsigned long berat = 250; //data kelembapan
String datakirim; //data json yang akan dikirim
String dataterima; //data json yang diterima
int count = 0; //variabel count untuk menghitung data pada json
int jumlahnode[5]; //array berisi NodeID

//variabel millis
unsigned long previousTime = 0; // Waktu sebelumnya
unsigned long intervalmillis = 10000; // Interval waktu (dalam milidetik)

//variabel RSSI node
int NODE_1_RSSI;
int NODE_3_RSSI;
int NODE_4_RSSI;
int NODE_5_RSSI;

//variabel case
int pilihan = 1;

//variabel BLE
int scanTime = 1; //In seconds

//Fungsi untuk 2 loop
//TaskHandle_t Task1;

//konfigurasi fungsi scan RSSI BLE
BLEScan* pBLEScan;
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
      if (advertisedDevice.getName() == "NODE_1")
      {
        NODE_1_RSSI = advertisedDevice.getRSSI();
      }
      if (advertisedDevice.getName() == "NODE_3")
      {
        NODE_3_RSSI = advertisedDevice.getRSSI();
      }
      if (advertisedDevice.getName() == "NODE_4")
      {
        NODE_4_RSSI = advertisedDevice.getRSSI();
      }
      if (advertisedDevice.getName() == "NODE_5")
      {
        NODE_5_RSSI = advertisedDevice.getRSSI();
      }
      //Serial.printf("Advertised Device: %s \n", advertisedDevice.toString().c_str());
    }
};

//program loop 2
//void loop2( void * parameter) {
//  for (;;) {
//    unsigned long currentTime = millis(); // Waktu saat ini
//
//    if (currentTime - previousTime >= intervalmillis) {
//      previousTime = currentTime; // Perbarui waktu sebelumnya
//      Serial.println("MODE : SCANNING......");
//      BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
//      Serial.print("RSSI NODE 1 : " + String(NODE_1_RSSI));
//      Serial.println(" || RSSI NODE 3 : " + String(NODE_3_RSSI));
//    }
//  }
//}

void setup() {
  Serial.begin(115200);
  StaticJsonDocument<512> doc; // buat document Json

  //Fungsi untuk 2 loop
  //  xTaskCreatePinnedToCore(
  //    loop2,
  //    "BLE_SCANNING",
  //    1000,
  //    NULL,
  //    1,
  //    &Task1,
  //    0);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  if (! rtc.begin()) {
    Serial.println("Could not find RTC! Check circuit.");
    while (1);
  }

  //fungsi setup untuk NRF24L01
  while (!Serial) {
    // some boards need this because of native USB capability
  }
  Serial.println(F("RF24Network/examples/helloworld_rx/"));

  if (!radio.begin()) {
    Serial.println(F("Radio hardware not responding!"));
    while (1) {
      // hold in infinite loop
    }
  }
  radio.setChannel(90);
  network.begin(/*node address*/ this_node);
  printf_begin();        // needed for RF24* libs' internal printf() calls
  radio.printDetails();  // requires printf support

  //fungsi setup BLE
  BLEDevice::init("NODE_2");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer ->createService(SERVICE_UUID);
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);

  pCharacteristic->setValue("Hello this is NODE 2");
  pService->start();
  BLEDevice::startAdvertising();
  Serial.println("BLE READY!!!");

  pBLEScan = BLEDevice::getScan(); //create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);  // less or equal setInterval value

  //scan BLE
  Serial.println("SCANNING......");
  BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
  Serial.print("RSSI NODE 1 : " + String(NODE_1_RSSI));
  Serial.print(" || RSSI NODE 3 : " + String(NODE_3_RSSI));
  Serial.print(" || RSSI NODE 4 : " + String(NODE_4_RSSI));
  Serial.println(" || RSSI NODE 5 : " + String(NODE_5_RSSI));
}

void loop() {
  network.update();  // Check the network regularly
  DateTime now = rtc.now();
  StaticJsonDocument<512> doc; // Json Document

  //scan ble
  unsigned long currentTime = millis(); // Waktu saat ini

  // if (currentTime - previousTime >= intervalmillis) {
  //   previousTime = currentTime; // Perbarui waktu sebelumnya
  //   Serial.println("MODE : SCANNING......");
  //   //BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
  //   Serial.print("RSSI NODE 1 : " + String(NODE_1_RSSI));
  //   Serial.print(" || RSSI NODE 3 : " + String(NODE_3_RSSI));
  //   Serial.print(" || RSSI NODE 4 : " + String(NODE_4_RSSI));
  //   Serial.println(" || RSSI NODE 5 : " + String(NODE_5_RSSI));
  // }
  
  while (network.available()) {
    RF24NetworkHeader header;  // If so, grab it and print it out
    uint16_t payloadSize = network.peek(header);     // Use peek() to get the size of the payload
    char terima_loop[payloadSize]; //buat variabel untuk nerima data array
    network.read(header, &terima_loop, payloadSize);
    dataterima = "";
    for (uint32_t i = 0; i < payloadSize; i++){
      dataterima +=terima_loop[i];
    }
    Serial.println(dataterima);
    DeserializationError error = deserializeJson(doc, dataterima);

    if (error) {
      Serial.print("deserializeJson() failed: ");
      Serial.println(error.c_str());
      return;
    }

    for (int i = 0; i <= 4; i++) {
      jumlahnode[i] = NULL;
      count = 0;
    }
    for (int i = 0; i <= 4; i++) {
      JsonObject parsing = doc[i];
      int NodeID = parsing["NodeID"];
      if (NodeID != 0) {
        count++;
        jumlahnode[i] += NodeID;
      }
    }
    //==================================================POSISI NODE KE - 1==================================================
    if (count == 1 && jumlahnode[0] == 5) {
      Serial.print("Received packet from NODE 5");
      JsonArray jsonarray = doc.to<JsonArray>();
      JsonObject jsonobject = jsonarray.createNestedObject();
      jsonobject["NodeID"] = node_asal;
      jsonobject["Berat"] = berat;
      jsonobject["Unxtime"] = now.unixtime();
      serializeJson(doc, datakirim);
      datakirim = "";
      char kirim_loop[datakirim.length() + 1];
      datakirim.toCharArray(kirim_loop,sizeof(kirim_loop));
      BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
      network.update();
      if (NODE_1_RSSI >= NODE_3_RSSI && NODE_1_RSSI >= NODE_4_RSSI) {
        RF24NetworkHeader header(/*to node*/ NODE_1);
        bool NODE_1 = network.write(header, &kirim_loop, sizeof(kirim_loop));
        Serial.println(NODE_1 ? F("DATA TERKIRIM KE NODE 1") : F("GAGAL TERKIRIM KE NODE 1"));
        if(!NODE_1){
          digitalWrite(LED_BUILTIN, HIGH);
          delay(100);
        }
        digitalWrite(LED_BUILTIN, LOW);
      }else if (NODE_3_RSSI >= NODE_1_RSSI && NODE_3_RSSI >= NODE_4_RSSI) {
        RF24NetworkHeader header(/*to node*/ NODE_3);
        bool NODE_3 = network.write(header, &kirim_loop, sizeof(kirim_loop));
        Serial.println(NODE_3 ? F("DATA TERKIRIM KE NODE 3") : F("GAGAL TERKIRIM KE NODE 3"));
        if(!NODE_3){
          digitalWrite(LED_BUILTIN, HIGH);
          delay(100);
        }
        digitalWrite(LED_BUILTIN, LOW);
      }else if (NODE_4_RSSI >= NODE_1_RSSI && NODE_4_RSSI >= NODE_3_RSSI) {
        RF24NetworkHeader header(/*to node*/ NODE_4);
        bool NODE_4 = network.write(header, &kirim_loop, sizeof(kirim_loop));
        Serial.println(NODE_4 ? F("DATA TERKIRIM KE NODE 4") : F("GAGAL TERKIRIM KE NODE 4"));
        if(!NODE_4){
          digitalWrite(LED_BUILTIN, HIGH);
          delay(100);
        }
        digitalWrite(LED_BUILTIN, LOW);
      }
    }
    //==================================================POSISI NODE KE - 2==================================================
    if (count == 1 && jumlahnode[0] == 1) {
      Serial.print("Received packet from NODE 1");
      JsonObject NodeID_1 = doc[0];
      int aNodeID_1 = NodeID_1["NodeID"];
      int Suhu = NodeID_1["Suhu"];
      int Kelembapan = NodeID_1["Kelembapan"];
      uint32_t Unixtime = NodeID_1["Unixtome"];
      JsonArray jsonarray = doc.to<JsonArray>();
      JsonObject jsonobject = jsonarray.createNestedObject();
      jsonobject["NodeID"] = aNodeID_1;
      jsonobject["Suhu"] = Suhu;
      jsonobject["Kelembapan"] = Kelembapan;
      jsonobject["Unixtime"] = Unixtime;
      JsonObject jsonobject1 = jsonarray.createNestedObject();
      jsonobject1["NodeID"] = node_asal;
      jsonobject1["Berat"] = berat;
      jsonobject1["Unixtome"] = now.unixtime();
      datakirim = "";
      serializeJson(doc, datakirim);
      char kirim_loop[datakirim.length() + 1];
      datakirim.toCharArray(kirim_loop,sizeof(kirim_loop));
      BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
      if (NODE_3_RSSI >= NODE_4_RSSI) {
        network.update();
        RF24NetworkHeader header(/*to node*/ NODE_3);
        bool NODE_3 = network.write(header, &kirim_loop, sizeof(kirim_loop));
        Serial.println(NODE_3 ? F("DATA TERKIRIM KE NODE 3") : F("GAGAL TERKIRIM KE NODE 3"));
        if(!NODE_3){
          digitalWrite(LED_BUILTIN, HIGH);
          delay(100);
        }
        digitalWrite(LED_BUILTIN, LOW);
      }else if (NODE_4_RSSI >= NODE_3_RSSI) {
        network.update();
        RF24NetworkHeader header(/*to node*/ NODE_4);
        bool NODE_4 = network.write(header, &kirim_loop, sizeof(kirim_loop));
        Serial.println(NODE_4 ? F("DATA TERKIRIM KE NODE 4") : F("GAGAL TERKIRIM KE NODE 4"));
        if(!NODE_4){
          digitalWrite(LED_BUILTIN, HIGH);
          delay(100);
        }
        digitalWrite(LED_BUILTIN, LOW);
      }
    }
    if (count == 1 && jumlahnode[0] == 3) {
      Serial.print("Received packet from NODE 3");
      JsonObject NodeID_3 = doc[0];
      int aNodeID_3 = NodeID_3["NodeID"];
      int usX = NodeID_3["usX"];
      int usY = NodeID_3["usY"];
      int usZ = NodeID_3["usZ"];
      uint32_t aUnixtime = NodeID_3["Unixtime"];
      JsonArray jsonarray = doc.to<JsonArray>();
      JsonObject jsonobject = jsonarray.createNestedObject();
      jsonobject["NodeID"] = aNodeID_3;
      jsonobject["usX"] = usX;
      jsonobject["usY"] = usY;
      jsonobject["usZ"] = usZ;
      jsonobject["Unixtome"] = aUnixtime;
      JsonObject jsonobject1 = jsonarray.createNestedObject();
      jsonobject1["NodeID"] = node_asal;
      jsonobject1["Berat"] = berat;
      jsonobject1["Unixtime"] = now.unixtime();
      datakirim = "";
      serializeJson(doc, datakirim);
      char kirim_loop[datakirim.length() + 1];
      datakirim.toCharArray(kirim_loop,sizeof(kirim_loop));
      BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
      if (NODE_1_RSSI >= NODE_4_RSSI) {
        network.update();
        RF24NetworkHeader header(/*to node*/ NODE_1);
        bool NODE_1 = network.write(header, &kirim_loop, sizeof(kirim_loop));
        Serial.println(NODE_1 ? F("DATA TERKIRIM KE NODE 1") : F("GAGAL TERKIRIM KE NODE 1"));
        if(!NODE_1){
          digitalWrite(LED_BUILTIN, HIGH);
          delay(100);
        }
        digitalWrite(LED_BUILTIN, LOW);
      }else if (NODE_4_RSSI >= NODE_1_RSSI) {
        network.update();
        RF24NetworkHeader header(/*to node*/ NODE_4);
        bool NODE_4 = network.write(header, &kirim_loop, sizeof(kirim_loop));
        Serial.println(NODE_4 ? F("DATA TERKIRIM KE NODE 4") : F("GAGAL TERKIRIM KE NODE 4"));
        if(!NODE_4){
          digitalWrite(LED_BUILTIN, HIGH);
          delay(100);
        }
        digitalWrite(LED_BUILTIN, LOW);
      }
    }
    if (count == 1 && jumlahnode[0] == 4) {
      Serial.print("Received packet from NODE 4");
      JsonObject NodeID_4 = doc[0];
      int aNodeID_4 = NodeID_4["NodeID"];
      int TofX = NodeID_4["TofX"];
      int TofY = NodeID_4["TofY"];
      int TofZ = NodeID_4["TofZ"];
      uint32_t bUnixtime = NodeID_4["Unixtime"];
      JsonArray jsonarray = doc.to<JsonArray>();
      JsonObject jsonobject = jsonarray.createNestedObject();
      jsonobject["NodeID"] = aNodeID_4;
      jsonobject["TofX"] = TofX;
      jsonobject["TofY"] = TofY;
      jsonobject["TofZ"] = TofZ;
      jsonobject["Unixtime"] = bUnixtime;
      JsonObject jsonobject1 = jsonarray.createNestedObject();
      jsonobject1["NodeID"] = node_asal;
      jsonobject1["Berat"] = berat;
      jsonobject1["Unixtime"] = bUnixtime;
      datakirim = "";
      serializeJson(doc, datakirim);
      char kirim_loop[datakirim.length() + 1];
      datakirim.toCharArray(kirim_loop,sizeof(kirim_loop));
      BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
      if (NODE_1_RSSI >= NODE_3_RSSI) {
        network.update();
        RF24NetworkHeader header(/*to node*/ NODE_1);
        bool NODE_1 = network.write(header, &kirim_loop, sizeof(kirim_loop));
        Serial.println(NODE_1 ? F("DATA TERKIRIM KE NODE 1") : F("GAGAL TERKIRIM KE NODE 1"));
        if(!NODE_1){
          digitalWrite(LED_BUILTIN, HIGH);
          delay(100);
        }
        digitalWrite(LED_BUILTIN, LOW);
      }else if (NODE_3_RSSI >= NODE_1_RSSI) {
        network.update();
        RF24NetworkHeader header(/*to node*/ NODE_3);
        bool NODE_3 = network.write(header, &kirim_loop, sizeof(kirim_loop));
        Serial.println(NODE_3 ? F("DATA TERKIRIM KE NODE 3") : F("GAGAL TERKIRIM KE NODE 3"));
        if(!NODE_3){
          digitalWrite(LED_BUILTIN, HIGH);
          delay(100);
        }
        digitalWrite(LED_BUILTIN, LOW);
      }
    }
    //==================================================POSISI NODE KE - 3==================================================
    if (count == 2) {
      Serial.print("Received packet from NODE " + jumlahnode[1]);
      if (jumlahnode[0] == 1 && jumlahnode[1] == 3) {
        JsonObject NodeID_1 = doc[0];
        int bNodeID_1 = NodeID_1["NodeID"];
        int Suhu = NodeID_1["Suhu"];
        int Kelembapan = NodeID_1["Kelembapan"];
        uint32_t cUnixtime = NodeID_1["Unixtime"];
        JsonObject NodeID_3 = doc[1];
        int bNodeID_3 = NodeID_3["NodeID"];
        int usX = NodeID_3["usX"];
        int usY = NodeID_3["usY"];
        int usZ = NodeID_3["usZ"];
        uint32_t dUnixtime = NodeID_3["Unixtime"];
        JsonArray jsonarray = doc.to<JsonArray>();
        JsonObject jsonobject = jsonarray.createNestedObject();
        jsonobject["NodeID"] = bNodeID_1;
        jsonobject["Suhu"] = Suhu;
        jsonobject["Kelembapan"] = Kelembapan;
        jsonobject["Unixtime"] = cUnixtime;
        JsonObject jsonobject1 = jsonarray.createNestedObject();
        jsonobject1["NodeID"] = bNodeID_3;
        jsonobject1["usX"] = usX;
        jsonobject1["usY"] = usY;
        jsonobject1["usZ"] = usZ;
        jsonobject1["Unixtime"] = dUnixtime;
        JsonObject jsonobject2 = jsonarray.createNestedObject();
        jsonobject2["NodeID"] = node_asal;
        jsonobject2["Berat"] = berat;
        jsonobject2["Unixtime"] = now.unixtime();
        datakirim = "";
        serializeJson(doc, datakirim);
        char kirim_loop[datakirim.length() + 1];
        datakirim.toCharArray(kirim_loop,sizeof(kirim_loop));
        network.update();
        RF24NetworkHeader header(/*to node*/ NODE_4);
        bool NODE_4 = network.write(header, &kirim_loop, sizeof(kirim_loop));
        Serial.println(NODE_4 ? F("DATA TERKIRIM KE NODE 4") : F("GAGAL TERKIRIM KE NODE 4"));
        if(!NODE_4){
          digitalWrite(LED_BUILTIN, HIGH);
          delay(100);
        }
        digitalWrite(LED_BUILTIN, LOW);
      }
      if (jumlahnode[0] == 3 && jumlahnode[1] == 1) {
        JsonObject NodeID_3 = doc[0];
        int cNodeID_3 = NodeID_3["NodeID"];
        int usX = NodeID_3["usX"];
        int usY = NodeID_3["usY"];
        int usZ = NodeID_3["usZ"];
        uint32_t eUnixtime = NodeID_3["Unixtime"];
        JsonObject NodeID_1 = doc[1];
        int cNodeID_1 = NodeID_1["NodeID"];
        int Suhu = NodeID_1["Suhu"];
        int Kelembapan = NodeID_1["Kelembapan"];
        uint32_t fUnixtime = NodeID_1["Unxtime"];
        JsonArray jsonarray = doc.to<JsonArray>();
        JsonObject jsonobject = jsonarray.createNestedObject();
        jsonobject["NodeID"] = cNodeID_3;
        jsonobject["usX"] = usX;
        jsonobject["usY"] = usY;
        jsonobject["usZ"] = usZ;
        jsonobject["Unixtime"] = eUnixtime;
        JsonObject jsonobject1 = jsonarray.createNestedObject();
        jsonobject1["NodeID"] = cNodeID_1;
        jsonobject1["Suhu"] = Suhu;;
        jsonobject1["Kelembapan"] = Kelembapan;
        jsonobject1["Unixtime"] = fUnixtime;
        JsonObject jsonobject2 = jsonarray.createNestedObject();
        jsonobject2["NodeID"] = node_asal;
        jsonobject2["Berat"] = berat;
        jsonobject2["Unixtime"] = now.unixtime();
        datakirim = "";
        serializeJson(doc, datakirim);
        char kirim_loop[datakirim.length() + 1];
        datakirim.toCharArray(kirim_loop,sizeof(kirim_loop));
        network.update();
        RF24NetworkHeader header(/*to node*/ NODE_4);
        bool NODE_4 = network.write(header, &kirim_loop, sizeof(kirim_loop));
        Serial.println(NODE_4 ? F("DATA TERKIRIM KE NODE 4") : F("GAGAL TERKIRIM KE NODE 4"));
        if(!NODE_4){
          digitalWrite(LED_BUILTIN, HIGH);
          delay(100);
        }
        digitalWrite(LED_BUILTIN, LOW);
      }
      if (jumlahnode[0] == 1 && jumlahnode[1] == 4) {
        JsonObject NodeID_1 = doc[0];
        int dNodeID_1 = NodeID_1["NodeID"];
        int Suhu = NodeID_1["Suhu"];
        int Kelembapan = NodeID_1["Kelembapan"];
        uint32_t gUnixtime = NodeID_1["Unixtime"];
        JsonObject NodeID_4 = doc[1];
        int bNodeID_4 = NodeID_4["NodeID"];
        int TofX = NodeID_4["TofX"];
        int TofY = NodeID_4["TofY"];
        int TofZ = NodeID_4["TofZ"];
        uint32_t hUnixtime = NodeID_4["Unixtime"];
        JsonArray jsonarray = doc.to<JsonArray>();
        JsonObject jsonobject = jsonarray.createNestedObject();
        jsonobject["NodeID"] = dNodeID_1;
        jsonobject["Suhu"] = Suhu;
        jsonobject["Kelembapan"] = Kelembapan;
        jsonobject["Unixtime"] = gUnixtime;
        JsonObject jsonobject1 = jsonarray.createNestedObject();
        jsonobject1["NodeID"] = bNodeID_4;
        jsonobject1["TofX"] = TofX;
        jsonobject1["TofY"] = TofY;
        jsonobject1["TofZ"] = TofZ;
        jsonobject1["Unixtime"] = hUnixtime;
        JsonObject jsonobject2 = jsonarray.createNestedObject();
        jsonobject2["NodeID"] = node_asal;
        jsonobject2["Berat"] = berat;
        jsonobject2["Unixtime"] = now.unixtime();
        datakirim = "";
        serializeJson(doc, datakirim);
        char kirim_loop[datakirim.length() + 1];
        datakirim.toCharArray(kirim_loop,sizeof(kirim_loop));
        network.update();
        RF24NetworkHeader header(/*to node*/ NODE_3);
        bool NODE_3 = network.write(header, &kirim_loop, sizeof(kirim_loop));
        Serial.println(NODE_3 ? F("DATA TERKIRIM KE NODE 3") : F("GAGAL TERKIRIM KE NODE 3"));
        if(!NODE_3){
          digitalWrite(LED_BUILTIN, HIGH);
          delay(100);
        }
        digitalWrite(LED_BUILTIN, LOW);
      }
      if (jumlahnode[0] == 4 && jumlahnode[1] == 1) {
        JsonObject NodeID_4 = doc[0];
        int cNodeID_4 = NodeID_4["NodeID"];
        int TofX = NodeID_4["TofX"];
        int TofY = NodeID_4["TofY"];
        int TofZ = NodeID_4["TofZ"];
        uint32_t iUnixtime = NodeID_4["Unixtime"];
        JsonObject NodeID_1 = doc[1];
        int eNodeID_1 = NodeID_1["NodeID"];
        int Suhu = NodeID_1["Suhu"];
        int Kelembapan = NodeID_1["Kelembapan"];
        uint32_t jUnixtime = NodeID_1["Unixtime"];
        JsonArray jsonarray = doc.to<JsonArray>();
        JsonObject jsonobject = jsonarray.createNestedObject();
        jsonobject["NodeID"] = cNodeID_4;
        jsonobject["TofX"] = TofX;
        jsonobject["TofY"] = TofY;
        jsonobject["TofZ"] = TofZ;
        jsonobject["Unixtime"] = iUnixtime;
        JsonObject jsonobject1 = jsonarray.createNestedObject();
        jsonobject1["NodeID"] = eNodeID_1;
        jsonobject1["Suhu"] = Suhu;
        jsonobject1["Kelembapan"] = Kelembapan;
        jsonobject1["Unixtime"] = jUnixtime;
        JsonObject jsonobject2 = jsonarray.createNestedObject();
        jsonobject2["NodeID"] = node_asal;
        jsonobject2["Berat"] = berat;
        jsonobject2["Unixtime"] = now.unixtime();
        datakirim = "";
        serializeJson(doc, datakirim);
        char kirim_loop[datakirim.length() + 1];
        datakirim.toCharArray(kirim_loop,sizeof(kirim_loop));
        network.update();
        RF24NetworkHeader header(/*to node*/ NODE_3);
        bool NODE_3 = network.write(header, &kirim_loop, sizeof(kirim_loop));
        Serial.println(NODE_3 ? F("DATA TERKIRIM KE NODE 3") : F("GAGAL TERKIRIM KE NODE 3"));
        if(!NODE_3){
          digitalWrite(LED_BUILTIN, HIGH);
          delay(100);
        }
        digitalWrite(LED_BUILTIN, LOW);
      }
      if (jumlahnode[0] == 4 && jumlahnode[1] == 3) {
        JsonObject NodeID_4 = doc[0];
        int dNodeID_4 = NodeID_4["NodeID"];
        int TofX = NodeID_4["TofX"];
        int TofY = NodeID_4["TofY"];
        int TofZ = NodeID_4["TofZ"];
        uint32_t kUnixtime = NodeID_4["Unixtime"];
        JsonObject NodeID_3 = doc[1];
        int dNodeID_3 = NodeID_3["NodeID"];
        int usX = NodeID_3["usX"];
        int usY = NodeID_3["usY"];
        int usZ = NodeID_3["usZ"];
        uint32_t lUnixtime = NodeID_3["Unixtime"];
        JsonArray jsonarray = doc.to<JsonArray>();
        JsonObject jsonobject = jsonarray.createNestedObject();
        jsonobject["NodeID"] = dNodeID_4;
        jsonobject["TofX"] = TofX;
        jsonobject["TofY"] = TofY;
        jsonobject["TofZ"] = TofZ;
        jsonobject["Unixtime"] = kUnixtime;
        JsonObject jsonobject1 = jsonarray.createNestedObject();
        jsonobject1["NodeID"] = dNodeID_3;
        jsonobject1["usX"] = usX;
        jsonobject1["usY"] = usY;
        jsonobject1["usZ"] = usZ;
        jsonobject1["Unixtime"] = lUnixtime;
        JsonObject jsonobject2 = jsonarray.createNestedObject();
        jsonobject2["NodeID"] = node_asal;
        jsonobject2["Berat"] = berat;
        jsonobject2["Unixtime"] = now.unixtime();
        datakirim = "";
        serializeJson(doc, datakirim);
        char kirim_loop[datakirim.length() + 1];
        datakirim.toCharArray(kirim_loop,sizeof(kirim_loop));
        network.update();
        RF24NetworkHeader header(/*to node*/ NODE_1);
        bool NODE_1 = network.write(header, &kirim_loop, sizeof(kirim_loop));
        Serial.println(NODE_1 ? F("DATA TERKIRIM KE NODE 1") : F("GAGAL TERKIRIM KE NODE 1"));
        if(!NODE_1){
          digitalWrite(LED_BUILTIN, HIGH);
          delay(100);
        }
        digitalWrite(LED_BUILTIN, LOW);
      }
      if (jumlahnode[0] == 3 && jumlahnode[1] == 4) {
        JsonObject NodeID_3 = doc[0];
        int eNodeID_3 = NodeID_3["NodeID"];
        int usX = NodeID_3["usX"];
        int usY = NodeID_3["usY"];
        int usZ = NodeID_3["usZ"];
        uint32_t mUnixtime = NodeID_3["Unixtime"];
        JsonObject NodeID_4 = doc[1];
        int eNodeID_4 = NodeID_4["NodeID"];
        int TofX = NodeID_4["TofX"];
        int TofY = NodeID_4["TofY"];
        int TofZ = NodeID_4["TofZ"];
        uint32_t nUnixtime = NodeID_4["Unixtime"];
        JsonArray jsonarray = doc.to<JsonArray>();
        JsonObject jsonobject = jsonarray.createNestedObject();
        jsonobject["NodeID"] = eNodeID_3;
        jsonobject["usX"] = usX;
        jsonobject["usY"] = usY;
        jsonobject["usZ"] = usZ;
        jsonobject["Unixtime"] = mUnixtime;
        JsonObject jsonobject1 = jsonarray.createNestedObject();
        jsonobject1["NodeID"] = eNodeID_4;
        jsonobject1["TofX"] = TofX;
        jsonobject1["TofY"] = TofY;
        jsonobject1["TofZ"] = TofZ;
        jsonobject1["Unixtime"] = nUnixtime;
        JsonObject jsonobject2 = jsonarray.createNestedObject();
        jsonobject2["NodeID"] = node_asal;
        jsonobject2["Berat"] = berat;
        jsonobject2["Unixtime"] = now.unixtime();
        datakirim = "";
        serializeJson(doc, datakirim);
        char kirim_loop[datakirim.length() + 1];
        datakirim.toCharArray(kirim_loop,sizeof(kirim_loop));
        network.update();
        RF24NetworkHeader header(/*to node*/ NODE_1);
        bool NODE_1 = network.write(header, &kirim_loop, sizeof(kirim_loop));
        Serial.println(NODE_1 ? F("DATA TERKIRIM KE NODE 1") : F("GAGAL TERKIRIM KE NODE 1"));
        if(!NODE_1){
          digitalWrite(LED_BUILTIN, HIGH);
          delay(100);
        }
        digitalWrite(LED_BUILTIN, LOW);
      }
    }
//==================================================POSISI NODE KE - 4==================================================  
    if( count == 3){
      Serial.print("Received packet from NODE " + jumlahnode[2]);
      if(jumlahnode[0] == 1 && jumlahnode[1] == 3 && jumlahnode[2] == 4){
        JsonObject NodeID_1 = doc[0];
        int fNodeID_1 = NodeID_1["NodeID"];
        int Suhu = NodeID_1["Suhu"];
        int Kelembapan = NodeID_1["Kelembapan"];
        uint32_t oUnixtime = NodeID_1["Unixtime"];
        JsonObject NodeID_3 = doc[1];
        int fNodeID_3 = NodeID_3["NodeID"];
        int usX = NodeID_3["usX"];
        int usY = NodeID_3["usY"];
        int usZ = NodeID_3["usZ"];
        uint32_t pUnixtime = NodeID_3["Unixtime"];
        JsonObject NodeID_4 = doc[2];
        int fNodeID_4 = NodeID_4["NodeID"];
        int TofX = NodeID_4["TofX"];
        int TofY = NodeID_4["TofY"];
        int TofZ = NodeID_4["TofZ"];
        uint32_t qUnixtime = NodeID_4["Unixtime"];
        JsonArray jsonarray = doc.to<JsonArray>();
        JsonObject jsonobject = jsonarray.createNestedObject();
        jsonobject["NodeID"] = fNodeID_1;
        jsonobject["Suhu"] = Suhu;
        jsonobject["Kelembapan"] = Kelembapan;
        jsonobject["Unixtime"] = oUnixtime;
        JsonObject jsonobject1 = jsonarray.createNestedObject();
        jsonobject1["NodeID"] = fNodeID_3;
        jsonobject1["usX"] = usX;
        jsonobject1["usY"] = usY;
        jsonobject1["usZ"] = usZ;
        jsonobject1["Unixtime"] = pUnixtime;
        JsonObject jsonobject2 = jsonarray.createNestedObject();
        jsonobject2["NodeID"] = fNodeID_4;
        jsonobject2["TofX"] = TofX;
        jsonobject2["TofY"] = TofY;
        jsonobject2["TofZ"] = TofZ;
        jsonobject2["Unixtime"] = qUnixtime;
        JsonObject jsonobject3 = jsonarray.createNestedObject();
        jsonobject3["NodeID"] = node_asal;
        jsonobject3["Berat"] = berat;
        jsonobject3["Unixtime"] = now.unixtime();
        datakirim = "";
        serializeJson(doc, datakirim);
        char kirim_loop[datakirim.length() + 1];
        datakirim.toCharArray(kirim_loop,sizeof(kirim_loop));
        network.update();
        RF24NetworkHeader header(/*to node*/ NODE_5);
        bool NODE_5 = network.write(header, &kirim_loop, sizeof(kirim_loop));
        Serial.println(NODE_5 ? F("DATA TERKIRIM KE NODE 5") : F("GAGAL TERKIRIM KE NODE 5"));
        if(!NODE_5){
          digitalWrite(LED_BUILTIN, HIGH);
          delay(100);
        }
        digitalWrite(LED_BUILTIN, LOW);
      }
      if(jumlahnode[0] == 1 && jumlahnode[1] == 4 && jumlahnode[2] == 3){
        JsonObject NodeID_1 = doc[0];
        int gNodeID_1 = NodeID_1["NodeID"];
        int Suhu = NodeID_1["Suhu"];
        int Kelembapan = NodeID_1["Kelembapan"];
        uint32_t rUnixtime = NodeID_1["Unixtime"];
        JsonObject NodeID_4 = doc[1];
        int gNodeID_4 = NodeID_4["NodeID"];
        int TofX = NodeID_4["TofX"];
        int TofY = NodeID_4["TofY"];
        int TofZ = NodeID_4["TofZ"];
        uint32_t sUnixtime = NodeID_4["Unixtime"];
        JsonObject NodeID_3 = doc[2];
        int gNodeID_3 = NodeID_3["NodeID"];
        int usX = NodeID_3["usX"];
        int usY = NodeID_3["usY"];
        int usZ = NodeID_3["usZ"];
        uint32_t tUnixtime = NodeID_4["Unixtime"];
        JsonArray jsonarray = doc.to<JsonArray>();
        JsonObject jsonobject = jsonarray.createNestedObject();
        jsonobject["NodeID"] = gNodeID_1;
        jsonobject["Suhu"] = Suhu;
        jsonobject["Kelembapan"] = Kelembapan;
        jsonobject["Unixtime"] = rUnixtime;
        JsonObject jsonobject1 = jsonarray.createNestedObject();
        jsonobject1["NodeID"] = gNodeID_4;
        jsonobject1["TofX"] = TofX;
        jsonobject1["TofY"] = TofY;
        jsonobject1["TofZ"] = TofZ;
        jsonobject1["Unixtime"] = sUnixtime;
        JsonObject jsonobject2 = jsonarray.createNestedObject();
        jsonobject2["NodeID"] = gNodeID_3;
        jsonobject2["usX"] = usX;
        jsonobject2["usY"] = usY;
        jsonobject2["usZ"] = usZ;
        jsonobject2["Unixtime"] = tUnixtime;
        JsonObject jsonobject3 = jsonarray.createNestedObject();
        jsonobject3["NodeID"] = node_asal;
        jsonobject3["Berat"] = berat;
        jsonobject3["Unixtime"] = now.unixtime();
        datakirim = "";
        serializeJson(doc, datakirim);
        char kirim_loop[datakirim.length() + 1];
        datakirim.toCharArray(kirim_loop,sizeof(kirim_loop));
        network.update();
        RF24NetworkHeader header(/*to node*/ NODE_5);
        bool NODE_5 = network.write(header, &kirim_loop, sizeof(kirim_loop));
        Serial.println(NODE_5 ? F("DATA TERKIRIM KE NODE 5") : F("GAGAL TERKIRIM KE NODE 5"));
        if(!NODE_5){
          digitalWrite(LED_BUILTIN, HIGH);
          delay(100);
        }
        digitalWrite(LED_BUILTIN, LOW);
      }
      if(jumlahnode[0] == 3 && jumlahnode[1] == 1 && jumlahnode[2] == 4){
        JsonObject NodeID_3 = doc[0];
        int hNodeID_3 = NodeID_3["NodeID"];
        int usX = NodeID_3["usX"];
        int usY = NodeID_3["usY"];
        int usZ = NodeID_3["usZ"];
        uint32_t uUnixtime = NodeID_3["Unixtime"];
        JsonObject NodeID_1 = doc[1];
        int hNodeID_1 = NodeID_1["NodeID"];
        int Suhu = NodeID_1["Suhu"];
        int Kelembapan = NodeID_1["kelembapan"];
        uint32_t vUnixtime = NodeID_1["Unixtime"];
        JsonObject NodeID_4 = doc[2];
        int hNodeID_4 = NodeID_4["NodeID"];
        int TofX = NodeID_4["TofX"];
        int TofY = NodeID_4["TofY"];
        int TofZ = NodeID_4["TofZ"];
        uint32_t wUnixtime = NodeID_4["Unixtime"];
        JsonArray jsonarray = doc.to<JsonArray>();
        JsonObject jsonobject = jsonarray.createNestedObject();
        jsonobject["NodeID"] = hNodeID_3;
        jsonobject["usX"] = usX;
        jsonobject["usY"] = usY;
        jsonobject["usZ"] = usZ;
        jsonobject["Unixtime"] = uUnixtime;
        JsonObject jsonobject1 = jsonarray.createNestedObject();
        jsonobject1["NodeID"] = hNodeID_1;
        jsonobject1["Suhu"] = Suhu;
        jsonobject1["Kelembapan"] = Kelembapan;
        jsonobject1["Unixtime"] = vUnixtime;
        JsonObject jsonobject2 = jsonarray.createNestedObject();
        jsonobject2["NodeID"] = hNodeID_4;
        jsonobject2["TofX"] = TofX;
        jsonobject2["TofY"] = TofY;
        jsonobject2["TofZ"] = TofZ;
        jsonobject2["Unixtime"] = wUnixtime;
        JsonObject jsonobject3 = jsonarray.createNestedObject();
        jsonobject3["NodeID"] = node_asal;
        jsonobject3["Berat"] = berat;
        jsonobject3["Unixtime"] = now.unixtime();
        datakirim = "";
        serializeJson(doc, datakirim);
        char kirim_loop[datakirim.length() + 1];
        datakirim.toCharArray(kirim_loop,sizeof(kirim_loop));
        network.update();
        RF24NetworkHeader header(/*to node*/ NODE_5);
        bool NODE_5 = network.write(header, &kirim_loop, sizeof(kirim_loop));
        Serial.println(NODE_5 ? F("DATA TERKIRIM KE NODE 5") : F("GAGAL TERKIRIM KE NODE 5"));
        if(!NODE_5){
          digitalWrite(LED_BUILTIN, HIGH);
          delay(100);
        }
        digitalWrite(LED_BUILTIN, LOW);
      }
      if(jumlahnode[0] == 3 && jumlahnode[1] == 4 && jumlahnode[2] == 1){
        JsonObject NodeID_3 = doc[0];
        int iNodeID_3 = NodeID_3["NodeID"];
        int usX = NodeID_3["usX"];
        int usY = NodeID_3["usY"];
        int usZ = NodeID_3["usZ"];
        uint32_t xUnixtime = NodeID_3["Unixtime"];
        JsonObject NodeID_4 = doc[1];
        int iNodeID_4 = NodeID_4["NodeID"];
        int TofX = NodeID_4["TofX"];
        int TofY = NodeID_4["TofY"];
        int TofZ = NodeID_4["TofZ"];
        uint32_t yUnixtime = NodeID_4["Unixtime"];
        JsonObject NodeID_1 = doc[2];
        int iNodeID_1 = NodeID_1["NodeID"];
        int Suhu = NodeID_1["Suhu"];
        int Kelembapan = NodeID_1["Kelembapan"];
        uint32_t zUnixtime = NodeID_1["Unixtime"];
        JsonArray jsonarray = doc.to<JsonArray>();
        JsonObject jsonobject = jsonarray.createNestedObject();
        jsonobject["NodeID"] = iNodeID_3;
        jsonobject["usX"] = usX;
        jsonobject["usY"] = usY;
        jsonobject["usZ"] = usZ;
        jsonobject["Unixtime"] = xUnixtime;
        JsonObject jsonobject1 = jsonarray.createNestedObject();
        jsonobject1["NodeID"] = iNodeID_4;
        jsonobject1["TofX"] = TofX;
        jsonobject1["TofY"] = TofY;
        jsonobject1["TofZ"] = TofZ;
        jsonobject1["Unixtime"] = yUnixtime;
        JsonObject jsonobject2 = jsonarray.createNestedObject();
        jsonobject2["NodeID"] = iNodeID_1;
        jsonobject2["Suhu"] = Suhu;
        jsonobject2["Kelembapan"] = Kelembapan;
        jsonobject2["Unixtime"] = zUnixtime;
        JsonObject jsonobject3 = jsonarray.createNestedObject();
        jsonobject3["NodeID"] = node_asal;
        jsonobject3["Berat"] = berat;
        jsonobject3["Unixtime"] = now.unixtime();
        datakirim = "";
        serializeJson(doc, datakirim);
        char kirim_loop[datakirim.length() + 1];
        datakirim.toCharArray(kirim_loop,sizeof(kirim_loop));
        network.update();
        RF24NetworkHeader header(/*to node*/ NODE_5);
        bool NODE_5 = network.write(header, &kirim_loop, sizeof(kirim_loop));
        Serial.println(NODE_5 ? F("DATA TERKIRIM KE NODE 5") : F("GAGAL TERKIRIM KE NODE 5"));
        if(!NODE_5){
          digitalWrite(LED_BUILTIN, HIGH);
          delay(100);
        }
        digitalWrite(LED_BUILTIN, LOW);
      }
      if(jumlahnode[0] == 4 && jumlahnode[1] == 3 && jumlahnode[2] == 1){
        JsonObject NodeID_4 = doc[0];
        int jNodeID_4 = NodeID_4["NodeID"];
        int TofX = NodeID_4["TofX"];
        int TofY = NodeID_4["TofY"];
        int TofZ = NodeID_4["TofZ"];
        uint32_t aaUnixtime = NodeID_4["Unixtime"];
        JsonObject NodeID_3 = doc[1];
        int jNodeID_3 = NodeID_3["NodeID"];
        int usX = NodeID_3["usX"];
        int usY = NodeID_3["usY"];
        int usZ = NodeID_3["usZ"];
        uint32_t abUnixtime = NodeID_3["Unixtime"];
        JsonObject NodeID_1 = doc[2];
        int jNodeID_1 = NodeID_1["NodeID"];
        int Suhu = NodeID_1["Suhu"];
        int Kelembapan = NodeID_1["Kelembapan"];
        uint32_t acUnixtime = NodeID_1["Unixtime"];
        JsonArray jsonarray = doc.to<JsonArray>();
        JsonObject jsonobject = jsonarray.createNestedObject();
        jsonobject["NodeID"] = jNodeID_4;
        jsonobject["TofX"] = TofX;
        jsonobject["TofY"] = TofY;
        jsonobject["TofZ"] = TofZ;
        jsonobject["Unixtime"] = aaUnixtime;
        JsonObject jsonobject1 = jsonarray.createNestedObject();
        jsonobject1["NodeID"] = jNodeID_3;
        jsonobject1["usX"] = usX;
        jsonobject1["usY"] = usY;
        jsonobject1["usZ"] = usZ;
        jsonobject1["Unixtime"] = abUnixtime;
        JsonObject jsonobject2 = jsonarray.createNestedObject();
        jsonobject2["NodeID"] = jNodeID_1;
        jsonobject2["Suhu"] = Suhu;
        jsonobject2["Kelembapan"] = Kelembapan;
        jsonobject2["Unixtime"] = acUnixtime;
        JsonObject jsonobject3 = jsonarray.createNestedObject();
        jsonobject3["NodeID"] = node_asal;
        jsonobject3["Berat"] = berat;
        jsonobject3["Unixtime"] = now.unixtime();
        datakirim = "";
        serializeJson(doc, datakirim);
        char kirim_loop[datakirim.length() + 1];
        datakirim.toCharArray(kirim_loop,sizeof(kirim_loop));
        network.update();
        RF24NetworkHeader header(/*to node*/ NODE_5);
        bool NODE_5 = network.write(header, &kirim_loop, sizeof(kirim_loop));
        Serial.println(NODE_5 ? F("DATA TERKIRIM KE NODE 5") : F("GAGAL TERKIRIM KE NODE 5"));
        if(!NODE_5){
          digitalWrite(LED_BUILTIN, HIGH);
          delay(100);
        }
        digitalWrite(LED_BUILTIN, LOW);
      }
      if(jumlahnode[0] == 4 && jumlahnode[1] == 1 && jumlahnode[2] == 3){
        JsonObject NodeID_4 = doc[0];
        int kNodeID_4 = NodeID_4["NodeID"];
        int TofX = NodeID_4["TofX"];
        int TofY = NodeID_4["TofY"];
        int TofZ = NodeID_4["TofZ"];
        uint32_t adUnixtime = NodeID_4["Unixtime"];
        JsonObject NodeID_1 = doc[1];
        int kNodeID_1 = NodeID_1["NodeID"];
        int Suhu = NodeID_1["Suhu"];
        int Kelembapan = NodeID_1["Kelembapan"];
        uint32_t aeUnixtime = NodeID_1["Unixtime"];
        JsonObject NodeID_3 = doc[2];
        int kNodeID_3 = NodeID_3["NodeID"];
        int usX = NodeID_3["usX"];
        int usY = NodeID_3["usY"];
        int usZ = NodeID_3["usZ"];
        uint32_t afUnixtime = NodeID_3["Unixtime"];
        JsonArray jsonarray = doc.to<JsonArray>();
        JsonObject jsonobject = jsonarray.createNestedObject();
        jsonobject["NodeID"] = kNodeID_4;
        jsonobject["TofX"] = TofX;
        jsonobject["TofY"] = TofY;
        jsonobject["TofZ"] = TofZ;
        jsonobject["Unixtime"] = adUnixtime;
        JsonObject jsonobject1 = jsonarray.createNestedObject();
        jsonobject1["NodeID"] = kNodeID_1;
        jsonobject1["Suhu"] = Suhu;
        jsonobject1["Kelembapan"] = Kelembapan;
        jsonobject1["Unixtime"] = aeUnixtime;
        JsonObject jsonobject2 = jsonarray.createNestedObject();
        jsonobject2["NodeID"] = kNodeID_3;
        jsonobject2["usX"] = usX;
        jsonobject2["usY"] = usY;
        jsonobject2["usZ"] = usZ;
        jsonobject2["Unixtime"] = afUnixtime;
        JsonObject jsonobject3 = jsonarray.createNestedObject();
        jsonobject3["NodeID"] = node_asal;
        jsonobject3["Berat"] = berat;
        jsonobject3["Unixtime"] = now.unixtime();
        datakirim = "";
        serializeJson(doc, datakirim);
        char kirim_loop[datakirim.length() + 1];
        datakirim.toCharArray(kirim_loop,sizeof(kirim_loop));
        network.update();
        RF24NetworkHeader header(/*to node*/ NODE_5);
        bool NODE_5 = network.write(header, &kirim_loop, sizeof(kirim_loop));
        Serial.println(NODE_5 ? F("DATA TERKIRIM KE NODE 5") : F("GAGAL TERKIRIM KE NODE 5"));
        if(!NODE_5){
          digitalWrite(LED_BUILTIN, HIGH);
          delay(100);
        }
        digitalWrite(LED_BUILTIN, LOW);
      }
    }
  }
  //pBLEScan->clearResults();   // delete results fromBLEScan buffer to release memory
}
