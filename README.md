# ESP32-C3 BPC 时间信号发射器

基于 ESP32-C3 SuperMini 的 BPC（BPC201）低频时码发射器，通过 68.5kHz 载波广播中国标准时间（CST, UTC+8）。

许可证：MIT

## 硬件

- ESP32-C3 SuperMini 开发板
- GPIO3 输出 68.5kHz 载波（LEDC PWM, APB 时钟 80MHz）
- 板载 LED（GPIO8）指示工作状态

## 功能

- WiFi 自动连接（支持断线重连，指数退避）
- NTP 双次同步校准，消除单次采样误差
- 每 20 秒发射一帧 BPC 时间编码（20 个四进制码元）
- 每 30 分钟 NTP 重新同步
- LED 状态指示：慢闪=连接中，快闪=对时中，双闪=重试，三闪=异常

## 编译

使用 PlatformIO，目标板 `esp32-c3-devkitm-1`：

```bash
pio run
```

## WiFi 配置

首次使用需创建 `include/credentials.h`：

```c
#pragma once
#define WIFI_SSID     "your-ssid"
#define WIFI_PASSWORD "your-password"
```

参考 `include/credentials.h.example`。

## 致谢

本项目灵感来源于 [MiniProjectDIY/ESP32_BPC_Transmitter](https://github.com/MiniProjectDIY/ESP32_BPC_Transmitter)，感谢原作者的创意与开源贡献。

## 项目结构

```
src/main.cpp           主程序
include/config.h       全局配置（引脚、频率、NTP 等）
include/credentials.h  WiFi 凭据（不提交）
include/bpc_tx.h       载波发射控制
include/bpc_encoder.h  BPC 时间编码
include/ntp_sync.h     WiFi 连接与 NTP 对时
include/led_status.h   LED 状态指示
```
