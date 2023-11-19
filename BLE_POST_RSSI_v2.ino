
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#define SSID "Xiaomi_9F1F"//"ESP32" //"VICTOR-2G""brisa-2591743"
#define PASSWORD "darlan10"//"esp32-s3" //"lilyezoey2015" //  //  //"3vijhau1"
int16_t counter = 0;
int16_t quadrante = -1;
int16_t cont_interacao= 600;
struct BeaconRSSI {
  int rssi;
  String mac_address;
  boolean hasBeenRead;
};
BeaconRSSI beaconRSSIs[] = {
  { -135, "ea:f7:7b:60:97:4e", false },
  { -135, "c4:01:12:14:28:8a", false },
  { -135, "fc:15:e2:04:45:8b", false },
  { -135, "e7:66:37:f2:0c:a4", false }
};
struct BeaconRead {
  int rssi_A;
  int rssi_B;
  int rssi_C;
  int rssi_D;
};

BeaconRead beaconRead[600];

BLEScan* pBLEScan;
void setup() {
  Serial.begin(115200);
}

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    String macDispositivoEncontrado = advertisedDevice.getAddress().toString().c_str();

    for (int i = 0; i < 4; i++) {
      if (beaconRSSIs[i].mac_address == macDispositivoEncontrado && beaconRSSIs[i].hasBeenRead == false) {
        beaconRSSIs[i].rssi = advertisedDevice.getRSSI();
        beaconRSSIs[i].hasBeenRead = true;
        Serial.print("Beacon encontrado: ");
        Serial.println(macDispositivoEncontrado);
      }
    }
  };
};

void scanBLE() {
  BLEDevice::init("");
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  BLEScanResults foundDevices = pBLEScan->start(1);
}

//------------------Metódo Post ----------------
void sendJSONviaPOST() {

  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Problemas ao conectar ao Wi-Fi");
  }
  Serial.println("Wi-Fi connected");


  int k = 0;
  while (k < cont_interacao) {

    StaticJsonDocument<128> jsonDoc;  // Tamanho máximo do JSON compacto (ajuste conforme necessário)
    jsonDoc["beaconA_rssi"] = beaconRead[k].rssi_A;
    jsonDoc["beaconB_rssi"] = beaconRead[k].rssi_B;
    jsonDoc["beaconC_rssi"] = beaconRead[k].rssi_C;
    jsonDoc["beaconD_rssi"] = beaconRead[k].rssi_D;
    jsonDoc["quadrante"] = quadrante;
    // Serializa o JSON compacto para uma string
    String jsonStr;
    serializeJson(jsonDoc, jsonStr);
    Serial.println(jsonStr);
    // Defina o endereço do servidor e o cabeçalho da requisição POST
    const char* serverUrl = "http://192.168.31.100:5000/api/rssi";
    HTTPClient http;
    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/json");

    // Envie a requisição POST com o JSON
    int httpResponseCode = http.POST(jsonStr);
    // Verifique a resposta do servidor
    if (httpResponseCode > 0) {
      Serial.print(F("Resposta do servidor: "));
      Serial.println(http.getString());
      k++;

    } else {
      Serial.print(F("Erro na requisição. Código de erro: "));
      Serial.println(httpResponseCode);
    }
     // Libere os recursos
  http.end();
  }
}

void resetValues() {
  for (int i = 0; i < 4; i++) {
    beaconRSSIs[i].hasBeenRead = false;
    beaconRSSIs[i].rssi = -135;
  }
}

void loop() {
  if (Serial.available()) {
    String mensagem = Serial.readString();
    int index = mensagem.indexOf(' ');
    if (index != -1) {
      String comando = mensagem.substring(0, index);
      quadrante = mensagem.substring(index + 1).toInt();  //define quadrante onde esp32 está posisionado
      Serial.print("Quadrante selecionado é:");
      Serial.println(quadrante);

      if (comando == "start" && quadrante > -1) {
        while (counter < cont_interacao) {
          scanBLE();
          if (beaconRSSIs[0].hasBeenRead && beaconRSSIs[1].hasBeenRead && beaconRSSIs[2].hasBeenRead && beaconRSSIs[3].hasBeenRead) {
            Serial.println("Obtida leitura dos 4 beacons");
            beaconRead[counter].rssi_A = beaconRSSIs[0].rssi;
            beaconRead[counter].rssi_B = beaconRSSIs[1].rssi;
            beaconRead[counter].rssi_C = beaconRSSIs[2].rssi;
            beaconRead[counter].rssi_D = beaconRSSIs[3].rssi;
            resetValues();  //reseta valores iniciais
            counter++;
          } else {
            Serial.println("Não foi possível fazer a leitura de todos os beacons");
            resetValues();  //reseta valores iniciais
          }
        }
        sendJSONviaPOST();
      }
      counter = 0;
    }
    delay(1000);
  }
}
