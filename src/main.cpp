#include <ThingsCloudWiFiManager.h>
#include <ThingsCloudMQTT.h>
#include <ArduinoJson.h>
#include "DHT.h"

//======================================================
// 设置 ssid / password，连接到你的 WiFi AP。上传公开仓库前不要填写真实值。
const char *ssid = "YOUR_WIFI_SSID";
const char *password = "YOUR_WIFI_PASSWORD";
// 在 ThingsCloud 控制台的设备详情页中复制连接信息。上传公开仓库前不要填写真实值。
// https://console.thingscloud.xyz
#define THINGSCLOUD_MQTT_HOST "YOUR_THINGSCLOUD_MQTT_HOST"
#define THINGSCLOUD_DEVICE_ACCESS_TOKEN "YOUR_THINGSCLOUD_DEVICE_ACCESS_TOKEN"
#define THINGSCLOUD_PROJECT_KEY "YOUR_THINGSCLOUD_PROJECT_KEY"
//======================================================

ThingsCloudMQTT client(
    THINGSCLOUD_MQTT_HOST,
    THINGSCLOUD_DEVICE_ACCESS_TOKEN,
    THINGSCLOUD_PROJECT_KEY);

const unsigned long report_interval = 35000;
unsigned long report_timer = 0;

#define DHTPIN 4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

const uint32_t DSP_BAUD_RATE = 9600;
const int DSP_RX_PIN = 16;
const int DSP_TX_PIN = 17;
const size_t DSP_LINE_MAX = 192;

struct DspStatus
{
  int led = 0;
  int beep = 0;
  int relay = 0;
  int motor_enable = 0;
  int motor_dir = 1;
  int motor_speed = 0;
  int adc_mv = 0;
  int alarm = 0;
  bool valid = false;
};

DspStatus dspStatus;
char dspLine[DSP_LINE_MAX];
size_t dspLineLen = 0;

void pubAttributes();
void pollDspSerial();
void handleDspLine(const char *line);
void handleCloudAttributes(const JsonObject &obj);
bool appendCommandField(JsonObject &dst, const JsonObject &src, const char *key);
int jsonToInt(JsonVariantConst value);

void setup()
{
  Serial.begin(115200);
  Serial2.begin(DSP_BAUD_RATE, SERIAL_8N1, DSP_RX_PIN, DSP_TX_PIN);

  client.enableDebuggingMessages();
  client.setWifiCredentials(ssid, password);

  dht.begin();
}

int jsonToInt(JsonVariantConst value)
{
  if (value.is<bool>())
  {
    return value.as<bool>() ? 1 : 0;
  }
  return value.as<int>();
}

void handleDspLine(const char *line)
{
  StaticJsonDocument<256> doc;
  DeserializationError err = deserializeJson(doc, line);
  if (err)
  {
    Serial.print(F("DSP JSON parse failed: "));
    Serial.println(err.c_str());
    return;
  }

  JsonObject obj = doc.as<JsonObject>();
  bool updated = false;

  if (obj.containsKey("led"))
  {
    dspStatus.led = jsonToInt(obj["led"]);
    updated = true;
  }
  if (obj.containsKey("beep"))
  {
    dspStatus.beep = jsonToInt(obj["beep"]);
    updated = true;
  }
  if (obj.containsKey("relay"))
  {
    dspStatus.relay = jsonToInt(obj["relay"]);
    updated = true;
  }
  if (obj.containsKey("motor_enable"))
  {
    dspStatus.motor_enable = jsonToInt(obj["motor_enable"]);
    updated = true;
  }
  if (obj.containsKey("motor_dir"))
  {
    dspStatus.motor_dir = jsonToInt(obj["motor_dir"]);
    updated = true;
  }
  if (obj.containsKey("motor_speed"))
  {
    dspStatus.motor_speed = jsonToInt(obj["motor_speed"]);
    updated = true;
  }
  if (obj.containsKey("adc_mv"))
  {
    dspStatus.adc_mv = jsonToInt(obj["adc_mv"]);
    updated = true;
  }
  if (obj.containsKey("alarm"))
  {
    dspStatus.alarm = jsonToInt(obj["alarm"]);
    updated = true;
  }

  if (updated)
  {
    dspStatus.valid = true;
  }
}

void pollDspSerial()
{
  while (Serial2.available() > 0)
  {
    char c = (char)Serial2.read();

    if (c == '\r')
    {
      continue;
    }

    if (c == '\n')
    {
      dspLine[dspLineLen] = '\0';
      if (dspLineLen > 0)
      {
        Serial.print(F("DSP -> ESP32: "));
        Serial.println(dspLine);
        handleDspLine(dspLine);
      }
      dspLineLen = 0;
      continue;
    }

    if (dspLineLen < DSP_LINE_MAX - 1)
    {
      dspLine[dspLineLen++] = c;
    }
    else
    {
      dspLineLen = 0;
    }
  }
}

bool appendCommandField(JsonObject &dst, const JsonObject &src, const char *key)
{
  if (!src.containsKey(key))
  {
    return false;
  }

  dst[key] = jsonToInt(src[key]);
  return true;
}

void handleCloudAttributes(const JsonObject &obj)
{
  StaticJsonDocument<192> cmdDoc;
  JsonObject cmd = cmdDoc.to<JsonObject>();
  bool hasCommand = false;

  hasCommand |= appendCommandField(cmd, obj, "led");
  hasCommand |= appendCommandField(cmd, obj, "beep");
  hasCommand |= appendCommandField(cmd, obj, "relay");
  hasCommand |= appendCommandField(cmd, obj, "motor_enable");
  hasCommand |= appendCommandField(cmd, obj, "motor_dir");
  hasCommand |= appendCommandField(cmd, obj, "motor_speed");
  hasCommand |= appendCommandField(cmd, obj, "alarm_clear");

  if (!hasCommand)
  {
    return;
  }

  serializeJson(cmdDoc, Serial2);
  Serial2.print('\n');

  Serial.print(F("ESP32 -> DSP: "));
  serializeJson(cmdDoc, Serial);
  Serial.println();
}

void pubAttributes()
{
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  StaticJsonDocument<512> obj;

  if (!isnan(h) && !isnan(t))
  {
    obj["temperature"] = t;
    obj["humidity"] = h;

    Serial.print(F("Humidity: "));
    Serial.print(h);
    Serial.print(F("%  Temperature: "));
    Serial.println(t);
  }
  else
  {
    Serial.println(F("Failed to read from DHT sensor."));
  }

  if (dspStatus.valid)
  {
    obj["adc_voltage"] = dspStatus.adc_mv / 1000.0;
    obj["led"] = dspStatus.led;
    obj["beep"] = dspStatus.beep;
    obj["relay"] = dspStatus.relay;
    obj["motor_enable"] = dspStatus.motor_enable;
    obj["motor_dir"] = dspStatus.motor_dir;
    obj["motor_speed"] = dspStatus.motor_speed;
    obj["alarm"] = dspStatus.alarm;
  }

  if (obj.size() == 0)
  {
    return;
  }

  char attributes[512];
  serializeJson(obj, attributes, sizeof(attributes));
  Serial.print(F("Report attributes: "));
  Serial.println(attributes);
  client.reportAttributes(attributes);
}

// 必须实现这个回调函数，当 MQTT 连接成功后执行该函数。
void onMQTTConnect()
{
  client.onAttributesGetResponse([](const String &topic, const JsonObject &obj)
                                 {
                                   if (obj["result"] == 1 && obj.containsKey("attributes"))
                                   {
                                     handleCloudAttributes(obj["attributes"]);
                                   }
                                 });

  client.onAttributesPush([](const JsonObject &obj)
                          { handleCloudAttributes(obj); });

  client.getAttributes();

  client.executeDelayed(1000 * 5, []()
                        { pubAttributes(); });
}

void loop()
{
  client.loop();
  pollDspSerial();

  if (millis() - report_timer > report_interval)
  {
    report_timer = millis();
    pubAttributes();
  }
}
