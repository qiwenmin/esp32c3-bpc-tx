#pragma once

// ── WiFi (credentials in credentials.h) ──
#include "credentials.h"
#define WIFI_TIMEOUT_S  20          // WiFi 连接超时(秒)
#define WIFI_RETRY_MS   10000       // WiFi 重试间隔(ms)

// ── NTP ──
#define NTP_HOST        "pool.ntp.org"
#define CST_OFFSET      (8 * 3600)  // UTC+8
#define NTP_RESYNC_MS   1800000UL   // NTP 重同步间隔(ms), 30 分钟

// ── BPC 引脚与载波 ──
#define BPC_PIN         3           // BPC 载波输出 GPIO
#define CARRIER_FREQ    68500       // 68.5kHz
#define PWM_RESOLUTION  10          // 10-bit (0-1023)
#define PWM_DUTY        512         // 50%

// ── LED (板载, 低电平点亮) ──
#define LED_PIN         8
#define LED_ACTIVE_LOW  true

// ── 串口 ──
#define SERIAL_BAUD     115200
