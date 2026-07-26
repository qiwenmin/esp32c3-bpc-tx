#pragma once
#include <Arduino.h>
#include "driver/ledc.h"
#include "driver/gpio.h"
#include "config.h"

// ESP32-C3 LEDC 默认使用 XTAL (40MHz), 10-bit 分辨率最高只能 ~39kHz.
// 必须用 IDF API 指定 APB 时钟 (80MHz) 才能达到 68.5kHz.
// 80MHz / 1024 ≈ 78kHz, 通过小数分频精确到 ~68.5kHz.

#define LEDC_TIMER      LEDC_TIMER_0
#define LEDC_MODE       LEDC_LOW_SPEED_MODE    // ESP32-C3 只有低速模式
#define LEDC_CH         LEDC_CHANNEL_0

class BpcTx {
public:
    void begin() {
        // 先将 GPIO3 强制拉低，防止 LEDC 配置前 NMOS 意外导通
        gpio_reset_pin((gpio_num_t)BPC_PIN);
        gpio_set_direction((gpio_num_t)BPC_PIN, GPIO_MODE_OUTPUT);
        gpio_set_level((gpio_num_t)BPC_PIN, 0);

        // Timer: APB clock (80MHz), 10-bit, 68.5kHz
        // ESP32-C3 默认用 XTAL(40MHz), 10bit 下最高 ~39kHz, 必须指定 APB(80MHz)
        ledc_timer_config_t timer_cfg = {
            .speed_mode      = LEDC_MODE,
            .duty_resolution = LEDC_TIMER_10_BIT,
            .timer_num       = LEDC_TIMER,
            .freq_hz         = CARRIER_FREQ,
            .clk_cfg         = LEDC_USE_APB_CLK
        };
        esp_err_t err = ledc_timer_config(&timer_cfg);
        if (err != ESP_OK) {
            Serial.printf("[BPC] Timer config failed: %d\n", err);
            return;
        }

        uint32_t actual_freq = ledc_get_freq(LEDC_MODE, LEDC_TIMER);
        Serial.printf("[BPC] Timer OK: requested %dHz, actual %luHz\n", CARRIER_FREQ, actual_freq);

        // Channel: GPIO3
        ledc_channel_config_t ch_cfg = {
            .gpio_num   = BPC_PIN,
            .speed_mode = LEDC_MODE,
            .channel    = LEDC_CH,
            .timer_sel  = LEDC_TIMER,
            .duty       = 0,
            .hpoint     = 0
        };
        err = ledc_channel_config(&ch_cfg);
        if (err != ESP_OK) {
            Serial.printf("[BPC] Channel config failed: %d\n", err);
            return;
        }

        _configured = true;
        carrierOff();
        Serial.printf("[BPC] Carrier ready: GPIO%d @ %luHz, duty %d/%d\n",
                       BPC_PIN, actual_freq, PWM_DUTY, (1 << PWM_RESOLUTION) - 1);
    }

    void carrierOn() {
        if (!_configured) return;
        ledc_set_duty(LEDC_MODE, LEDC_CH, PWM_DUTY);
        ledc_update_duty(LEDC_MODE, LEDC_CH);
        _carrierActive = true;
    }

    void carrierOff() {
        if (!_configured) return;
        ledc_set_duty(LEDC_MODE, LEDC_CH, 0);
        ledc_update_duty(LEDC_MODE, LEDC_CH);
        _carrierActive = false;
    }

    bool isCarrierOn() const { return _carrierActive; }

    static void waitUntil(unsigned long start_ms, unsigned long target_ms) {
        while (millis() - start_ms < target_ms) {
            unsigned long remain = target_ms - (millis() - start_ms);
            if (remain > 2) {
                delay(remain - 2);
            }
        }
        while (millis() - start_ms < target_ms) { /* spin */ }
    }

    void transmitFrame(const uint16_t durations[20]) {
        for (uint8_t i = 0; i < 20; i++) {
            unsigned long secStart = millis();
            uint16_t dura = durations[i];

            if (dura == 0) {
                carrierOn();
            } else {
                carrierOff();
                waitUntil(secStart, dura);
                carrierOn();
            }

            waitUntil(secStart, 1000);
        }
    }

private:
    bool _configured = false;
    bool _carrierActive = false;
};

extern BpcTx bpcTx;
