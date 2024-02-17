#include <Arduino.h>

//library loadcell
#include "HX711.h"

//library RTC
#include <Wire.h>
#include "RTClib.h"

//library Json
#include <ArduinoJson.h>

//Library NRF24L01
#include <SPI.h>
#include <RF24.h>
#include <RF24Network.h>
#include <RF24Mesh.h>
#include "printf.h"

//konfigurasi loadcell
#define DOUT  27
#define CLK  26
HX711 scale(DOUT, CLK);

//konfigurasi stack size
SET_LOOP_TASK_STACK_SIZE(32*1024); // 64KB

//konfigurasi RTC
RTC_DS3231 rtc;
char days[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

//konfigurasi NRF24L01
RF24 radio(4, 5); //(pin CE, pin CSN)
RF24Network network(radio);      // Network uses that radio
RF24Mesh mesh(radio, network);
uint8_t dataBuffer[MAX_PAYLOAD_SIZE];  //MAX_PAYLOAD_SIZE is defined in RF24Network_config.h

//alamat node
#define this_node 2

//Variabel DATA
int node_asal = 2; //data node
float calibration_factor = 1135.00;
int berat; //data berat
String datakirim; //data json yang akan dikirim

//variabel millis
unsigned long currentMillis = 0;

//Fungsi untuk 2 loop
TaskHandle_t Task1;

//program loop 2
void bacasensor( void * parameter) {
 for (;;) {
    scale.set_scale(calibration_factor);
    berat = scale.get_units(), 5;
    Serial.println("Running on Core : "+String(xPortGetCoreID())+", Berat : "+String(berat));
 }
}

void setup() {
  Serial.begin(115200);

  scale.set_scale();
  scale.tare();

  if (! rtc.begin()) {
    Serial.println("Tidak dapat menemukan RTC! Periksa sirkuit.");
    while (1);
  }

  //fungsi setup untuk NRF24L01
  while (!Serial) {
    // some boards need this because of native USB capability
  }
  mesh.setNodeID(this_node); //Set the Node ID
  Serial.println(F("Menghubungkan ke jaringan..."));

  if (!mesh.begin()){
    if (radio.isChipConnected()){
      do {
        // mesh.renewAddress() will return MESH_DEFAULT_ADDRESS on failure to connect
        Serial.println(F("Gagal terhubung ke jaringan.\nMenghubungkan ke jaringan..."));
      } while (mesh.renewAddress() == MESH_DEFAULT_ADDRESS);
    } else {
      Serial.println(F("NRF24L01 tidak merespon."));
      while (1) {
        // hold in an infinite loop
      }
    }
  }
  printf_begin();
  radio.printDetails();  // print detail konfigurasi NRF24L01

  //Fungsi untuk 2 loop
   xTaskCreatePinnedToCore(
     bacasensor,    /* Task function. */
     "baca_sensor", /* name of task. */
     32768,         /* Stack size of task */
     NULL,          /* parameter of the task */
     1,             /* priority of the task */
     &Task1,        /* Task handle to keep track of created task */
     0);            /* pin task to core 0 */

  // print memori stack keseluruhan
  Serial.printf("Arduino Stack was set to %d bytes", getArduinoLoopTaskStackSize());
  // print sisa memori stack pada void setup
  Serial.printf("\nSetup() - Free Stack Space: %d", uxTaskGetStackHighWaterMark(NULL));
}

void loop() {
  // print sisa memori stack pada void loop
  Serial.printf("\nLoop() - Free Stack Space: %d", uxTaskGetStackHighWaterMark(NULL));
  
  mesh.update();
  DateTime now = rtc.now();
  StaticJsonDocument<128> doc; // Json Document

  // Mengirim data ke master
  if (millis() - currentMillis >= 250) {
    currentMillis = millis();
    doc["NodeID"] = String(node_asal);
    doc["Berat"] = String(berat);
    doc["Unixtime"] = String(now.unixtime());
    datakirim = "";
    serializeJson(doc, datakirim);
    char kirim_loop[datakirim.length() + 1];
    datakirim.toCharArray(kirim_loop, sizeof(kirim_loop));

    if (!mesh.write(&kirim_loop, '2', sizeof(kirim_loop))) {
      if (!mesh.checkConnection()) {
        Serial.println("Memperbaharui Alamat");
        if (mesh.renewAddress() == MESH_DEFAULT_ADDRESS) {
          mesh.begin();
        }
      } else {
        Serial.println("Gagal Mengirim ke Master, Tes jaringan OK");
      }
    } else {
      Serial.print("Berhasil Mengirim ke Master : ");
      Serial.println(datakirim);
    }
  }
}
