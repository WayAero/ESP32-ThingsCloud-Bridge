# ESP32 ThingsCloud Bridge

ESP32 侧桥接程序。它负责连接 Wi-Fi 和 ThingsCloud，通过 `Serial2` 接收 DSP28335 的 JSON 状态，并把 ThingsCloud 属性下发转发给 DSP。

## 学习参考声明

本项目仅供课程设计、嵌入式学习和工程复现参考。

它配套 DSP28335 综合工程使用，主要解决旧 DSP 教程与现代云平台联调时缺少完整桥接示例的问题。代码中保留了 ThingsCloud 属性上报、属性下发、DSP JSON 串口协议和 DHT22 温湿度上报的完整流程。

本仓库不包含真实 Wi-Fi 密码或 ThingsCloud 设备密钥。实际使用时请填写自己的设备参数，不要把真实凭据提交到公开仓库。

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

这是实际联调中遇到过的问题。DSP 接到 RX0/TX0 时，电脑串口可能能看到日志，但当前程序不会从 RX0/TX0 读取 DSP JSON。正确做法是接到 `Serial2` 对应的 GPIO16/GPIO17。

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

## 实践问题总结

### 1. ThingsCloud JSON 和 DSP 串口 JSON 不是同一层

ThingsCloud 属性下发时使用标准紧凑 JSON，不需要在末尾添加 `\n`。

DSP 串口协议需要 `\n` 作为一条命令的结束符。

本程序负责在转发时追加串口换行：

```cpp
serializeJson(cmdDoc, Serial2);
Serial2.print('\n');
```

### 2. DSP 状态字段使用扁平属性

DSP 每秒输出一行状态 JSON。ESP32 解析后上报为 ThingsCloud 设备属性。

这样云端页面可以直接查看：

```text
adc_voltage
led
beep
relay
motor_enable
motor_dir
motor_speed
alarm
```

不需要自定义数据流，也不需要 ThingsCloud 命令下发 topic。

### 3. 下发控制只转发已支持字段

程序只把这些字段转发给 DSP：

```text
led
beep
relay
motor_enable
motor_dir
motor_speed
alarm_clear
```

其他云端字段不会转发，避免误把无关属性发给 DSP。

### 4. DHT22 读取失败不清空 DSP 状态

DHT22 偶发读取失败时，只跳过本次温湿度上报。

DSP 状态仍会继续解析和上报，避免因为一个传感器失败影响整条控制链路。

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

本仓库不包含真实 Wi-Fi 密码或 ThingsCloud 设备密钥。

ESP32 工程中的 Wi-Fi 和 ThingsCloud 参数使用占位符。实际使用时需要在 ESP32 工程中填写自己的参数。

