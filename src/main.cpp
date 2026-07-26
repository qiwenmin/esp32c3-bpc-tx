#include <Arduino.h>
#include "config.h"
#include "led_status.h"
#include "ntp_sync.h"
#include "bpc_encoder.h"
#include "bpc_tx.h"

// 全局实例定义（头文件中为 extern 声明）
BpcTx    bpcTx;
NtpSync  ntp;
LedStatus led;

static uint32_t frameCount = 0;
static uint32_t lastNtpSyncFrame = 0;

static void printFrame(uint8_t num, const int cst[7], const uint16_t dur[20]) {
    Serial.printf("[TX] Frame #%04u  %04d-%02d-%02d %02d:%02d:%02d  dur:",
        num, cst[0], cst[1], cst[2], cst[3], cst[4], cst[5]);
    for (int i = 0; i < 20; i++) {
        Serial.printf(" %u", dur[i]);
    }
    Serial.println();
}

void setup() {
    Serial.begin(SERIAL_BAUD);
    while (!Serial) ;
    delay(500);
    Serial.println("========================================");
    Serial.println("  BPC Transmitter - ESP32-C3 SuperMini");
    Serial.println("========================================");

    led.begin();
    bpcTx.begin();

    // ── WiFi ──
    led.set(LedPattern::SLOW_BLINK);
    Serial.println("[INIT] Connecting WiFi...");
    while (!ntp.wifiConnect()) {
        led.set(LedPattern::DOUBLE_BLINK);
        Serial.printf("[WiFi] Retrying in %ds...\n", WIFI_RETRY_MS / 1000);
        delay(WIFI_RETRY_MS);
        led.set(LedPattern::SLOW_BLINK);
    }

    // ── NTP ──
    led.set(LedPattern::FAST_BLINK);
    Serial.println("[INIT] Syncing NTP...");
    while (!ntp.ntpSync()) {
        led.set(LedPattern::DOUBLE_BLINK);
        Serial.printf("[NTP] Retrying in 5s...\n");
        delay(5000);
        led.set(LedPattern::FAST_BLINK);
    }
    lastNtpSyncFrame = frameCount;

    // ── 初始化完成 ──
    led.set(LedPattern::ON);
    int cst[7];
    ntp.getCST(cst);
    Serial.printf("[INIT] Current time: %04d-%02d-%02d %02d:%02d:%02d CST\n",
        cst[0], cst[1], cst[2], cst[3], cst[4], cst[5]);

    // 对齐 BPC 帧边界
    ntp.waitForBoundary(cst);
    Serial.println("[INIT] Starting continuous transmission...");
}

void loop() {
    int cst[7];
    if (!ntp.getCST(cst)) {
        Serial.println("[ERR] Failed to get time");
        delay(1000);
        return;
    }

    // 编码
    uint16_t durations[20];
    bpc_encode(cst, durations);

    // 发射
    frameCount++;
    printFrame(frameCount, cst, durations);
    bpcTx.transmitFrame(durations);

    bool needRealign = false;

    // 帧间隙：WiFi 检查
    if (!ntp.wifiIsConnected()) {
        Serial.println("[WiFi] Lost connection, reconnecting...");
        led.set(LedPattern::DOUBLE_BLINK);
        ntp.ensureWifi();
        // WiFi 重连后立即重同步 NTP
        led.set(LedPattern::FAST_BLINK);
        if (ntp.ntpSync()) {
            Serial.println("[NTP] Resync after WiFi reconnection OK");
        }
        needRealign = true;
    }

    // 每 NTP_RESYNC_FRAMES 帧 NTP 重同步 (~30 分钟)
    if (frameCount - lastNtpSyncFrame >= NTP_RESYNC_FRAMES) {
        Serial.printf("[NTP] Resync (after %lu frames)\n", frameCount);
        led.set(LedPattern::FAST_BLINK);
        if (!ntp.ntpSync()) {
            Serial.println("[NTP] Resync failed, continuing TX");
            led.set(LedPattern::TRIPLE_BLINK);
        } else {
            lastNtpSyncFrame = frameCount;
            led.set(LedPattern::ON);
        }
        needRealign = true;
    }

    // WiFi 重连或 NTP 对时后，重新对齐帧边界
    if (needRealign) {
        int boundary[7];
        ntp.waitForBoundary(boundary);
    }

}
