#include <Arduino.h>
#include <MyNode.h>
#include <BLEDevice.h>

#define MAC "A2:93:38:CB:51:69"

BLEClient* pMyClient = BLEDevice::createClient();

BLERemoteService* pMyRemoteService;
BLERemoteCharacteristic* pMyRemoteCharacteristic;
BLERemoteCharacteristic* pMyRemoteCharacteristic2;

void handleInput(const char* topic, const char* payload);
void handleVolume(const char* topic, const int payload);

unsigned long mDisconnectTime = 0;

void setup() {
  Serial.begin(115200);

  myNode.setup();

  BLEDevice::init("");

  char topic[32];
  sprintf(topic, "%s/input", WiFi.getHostname());
  myNode.add_topic(topic, handleInput);

  sprintf(topic, "%s/volume", WiFi.getHostname());
  myNode.add_topic(topic, handleVolume);
}

void loop() {
  myNode.loop();
  delay(10);

  if ((mDisconnectTime != 0) && (millis() > mDisconnectTime))
  {
    pMyClient->disconnect();
    mDisconnectTime = 0;
  }
}

static void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify)
{
  char topic[32];
  if ((pData[0] == 0x7e) && (pData[1] == 0x0f) && (pData[5] == 0x02))
  {
    sprintf(topic, "%s/status/volume", WiFi.getHostname());
    myNode.publish(topic, (int)pData[6]);
    sprintf(topic, "%s/status/input", WiFi.getHostname());
    myNode.publish(topic, (int)pData[4]);
  }
  //0x7e 0x10 0x1f 0x01 0x00 0x06 0x31 0x32 0x33 0x34 0x00 0x32 0x31 0x02 0x01 0xe3
  if ((pData[0] == 0x7e) && (pData[1] == 0x10) && (pData[2] == 0x1f))
  {
    sprintf(topic, "%s/status/volume", WiFi.getHostname());
    myNode.publish(topic, (int)pData[5]);
  }
}

void connect()
{
  if (pMyClient->isConnected() == false)
  {
    pMyClient->connect(BLEAddress(MAC));
  }

  pMyRemoteService = pMyClient->getService("0000ae00-0000-1000-8000-00805f9b34fb");
  pMyRemoteCharacteristic = pMyRemoteService->getCharacteristic("0000ae10-0000-1000-8000-00805f9b34fb");
  pMyRemoteCharacteristic2 = pMyRemoteService->getCharacteristic("0000ae04-0000-1000-8000-00805f9b34fb");

  if(pMyRemoteCharacteristic2->canNotify())
  {
    pMyRemoteCharacteristic2->registerForNotify(notifyCallback);
  }
}

void setVolume(int volume)
{
  if ((volume < 32) && (volume > 0))
  {
    uint8_t data[] = {0x7e, 0x0f, 0x1d, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xba};
    uint8_t sum = 0;

    data[3] = volume;

    for (int i = 0; i < sizeof(data) - 1; ++i)
    {
      sum += data[i];
    }

    data[sizeof(data) - 1] = sum;
    pMyRemoteCharacteristic->writeValue(data, sizeof(data), true);
  }
}

void handleInput(const char* topic, const char* payload)
{
  connect();

  if (strcmp(payload, "aux") == 0)
  {
    setVolume(4);
    uint8_t data[] = {0x7e ,0x05 ,0x16 ,0x00 ,0x99};
    pMyRemoteCharacteristic->writeValue(data, sizeof(data), true);
  }
  if (strcmp(payload, "bt") == 0)
  {
    uint8_t data[] = {0x7e ,0x05 ,0x14, 0x00, 0x97};
    pMyRemoteCharacteristic->writeValue(data, sizeof(data), true);
    setVolume(10);
  }

  mDisconnectTime = millis() + 2000;
}

void handleVolume(const char* topic, const int payload)
{
  connect();
  setVolume(payload);
  mDisconnectTime = millis() + 2000;
}