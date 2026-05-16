# ESP32 ThingsCloud Bridge

ESP32 侧桥接程序。它负责连接 Wi-Fi 和 ThingsCloud，通过 `Serial2` 接收 DSP28335 的 JSON 状态，并把 ThingsCloud 属性下发转发给 DSP。

## 功能

- 读取 DHT22 温湿度。
- 通过 ThingsCloud MQTT 上报设备属性。
- 使用 `Serial2` 接收 DSP 每秒 JSON 状态。
- 解析 DSP 的 LED、蜂鸣器、继电器、电机、ADC、报警状态。
- 将 ThingsCloud 属性下发转成 DSP JSON 串口命令。

## 串口连接

ESP32 与 DSP 连接：

```text
DSP GPIO35 / SCITXDA -> ESP32 GPIO16 / RX2
DSP GPIO36 / SCIRXDA <- ESP32 GPIO17 / TX2
DSP GND              <-> ESP32 GND
```

注意：

- 不要把 DSP 接到 ESP32 RX0/TX0。
- RX0/TX0 留给 USB 调试串口。
- DSP 桥接使用 `GPIO16/GPIO17`。

## 配置凭据

打开 `src/main.cpp`，替换这些占位符：

```cpp
const char *ssid = "YOUR_WIFI_SSID";
const char *password = "YOUR_WIFI_PASSWORD";

#define THINGSCLOUD_MQTT_HOST "YOUR_THINGSCLOUD_MQTT_HOST"
#define THINGSCLOUD_DEVICE_ACCESS_TOKEN "YOUR_THINGSCLOUD_DEVICE_ACCESS_TOKEN"
#define THINGSCLOUD_PROJECT_KEY "YOUR_THINGSCLOUD_PROJECT_KEY"
```

不要把真实 Wi-Fi 密码或 ThingsCloud Token 提交到公开仓库。

## PlatformIO

依赖在 `platformio.ini` 中声明：

```ini
lib_deps =
    thingscloud/ThingsCloud_ESP_SDK@^1.0.14
    adafruit/DHT sensor library@^1.4.7
```

串口监视器波特率：

```text
115200
```

DSP 串口参数：

```text
9600 8N1
```

## ThingsCloud 属性

上报字段：

```text
temperature
humidity
adc_voltage
led
beep
relay
motor_enable
motor_dir
motor_speed
alarm
```

可下发字段：

```text
led
beep
relay
motor_enable
motor_dir
motor_speed
alarm_clear
```

下发示例：

```json
{"led":1}
```

```json
{"motor_enable":1,"motor_speed":7}
```

```json
{"alarm_clear":1}
```

ThingsCloud 下发的 MQTT JSON 不需要添加换行。程序转发给 DSP 时会自动追加 `\n`。

## 正常日志

收到 DSP 状态：

```text
DSP -> ESP32: {"led":0,"beep":0,"relay":0,"motor_enable":0,"motor_dir":1,"motor_speed":0,"adc_mv":2200,"alarm":0}
```

收到 ThingsCloud 属性下发并转发给 DSP：

```text
MQTT >> [attributes/push] {"led":1}
ESP32 -> DSP: {"led":1}
```

## 隐私说明

本仓库当前版本只保留占位凭据。

上传公开仓库前，不要提交真实 Wi-Fi 名称、Wi-Fi 密码、ThingsCloud Token 或 Project Key。
