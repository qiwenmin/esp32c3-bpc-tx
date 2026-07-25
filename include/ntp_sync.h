#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include "config.h"

class NtpSync {
public:
    bool wifiConnect() {
        Serial.printf("[WiFi] Connecting to %s\n", WIFI_SSID);
        WiFi.mode(WIFI_STA);
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

        unsigned long start = millis();
        while (WiFi.status() != WL_CONNECTED) {
            if (millis() - start > WIFI_TIMEOUT_S * 1000UL) {
                Serial.println("[WiFi] Connection timed out");
                WiFi.disconnect();
                return false;
            }
            delay(100);
        }

        Serial.printf("[WiFi] Connected, IP: %s\n", WiFi.localIP().toString().c_str());
        return true;
    }

    bool wifiIsConnected() {
        return WiFi.status() == WL_CONNECTED;
    }

    bool ntpSync() {
        Serial.printf("[NTP] Syncing with %s...\n", NTP_HOST);
        configTime(CST_OFFSET, 0, NTP_HOST);

        // 第一次同步：等待 SNTP 响应 (最多 10 秒)
        struct tm t;
        for (int i = 0; i < 100; i++) {
            if (getLocalTime(&t, 100) && t.tm_year > (2020 - 1900)) {
                Serial.printf("[NTP] First sync OK, calibrating...\n");
                break;
            }
        }

        // 记录第一次时间，用于对比
        struct tm t_prev = t;
        delay(2000);

        // 二次校准：重新同步，等待时间变化
        configTime(CST_OFFSET, 0, NTP_HOST);
        for (int i = 0; i < 100; i++) {
            if (getLocalTime(&t, 100) && t.tm_year > (2020 - 1900)) {
                // 等待时间与第一次不同（说明第二次 SNTP 响应已生效）
                if (memcmp(&t, &t_prev, sizeof(struct tm)) != 0) {
                    Serial.printf("[NTP] OK: %04d-%02d-%02d %02d:%02d:%02d\n",
                        t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
                        t.tm_hour, t.tm_min, t.tm_sec);
                    return true;
                }
            }
        }
        Serial.println("[NTP] Sync failed");
        return false;
    }

    // 获取当前 CST 时间，填入 cst[7]: year, month, day, hour, minute, second, weekday
    bool getCST(int cst[7]) {
        struct tm t;
        if (!getLocalTime(&t, 100)) return false;
        cst[0] = t.tm_year + 1900;
        cst[1] = t.tm_mon + 1;
        cst[2] = t.tm_mday;
        cst[3] = t.tm_hour;
        cst[4] = t.tm_min;
        cst[5] = t.tm_sec;
        // tm_wday: 0=Sun..6=Sat -> BPC 需要 0=Mon..6=Sun
        cst[6] = (t.tm_wday == 0) ? 6 : t.tm_wday - 1;
        return true;
    }

    // 等待直到秒数进入 BPC 边界 (0, 20, 40)，返回该时刻的 CST
    bool waitForBoundary(int cst[7]) {
        Serial.println("[BPC] Waiting for frame boundary (sec 0/20/40)...");
        // 先等离开当前边界
        int s[7];
        getCST(s);
        while (s[5] == 0 || s[5] == 20 || s[5] == 40) {
            delay(50);
            getCST(s);
        }
        // 再等到进入下一个边界
        while (true) {
            getCST(s);
            if (s[5] == 0 || s[5] == 20 || s[5] == 40) {
                Serial.printf("[BPC] Aligned: %02d:%02d:%02d\n", s[3], s[4], s[5]);
                for (int i = 0; i < 7; i++) cst[i] = s[i];
                return true;
            }
            delay(10);
        }
    }

    bool ensureWifi() {
        if (wifiIsConnected()) return true;
        Serial.println("[WiFi] Disconnected, reconnecting...");
        return wifiConnect();
    }
};

extern NtpSync ntp;
