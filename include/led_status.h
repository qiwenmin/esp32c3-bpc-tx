#pragma once
#include <Arduino.h>
#include "config.h"

enum class LedPattern : uint8_t {
    OFF,
    ON,
    SLOW_BLINK,     // 500ms - WiFi 连接中
    FAST_BLINK,     // 150ms - NTP 同步中
    DOUBLE_BLINK,   // 双闪 - 失败重试
    TRIPLE_BLINK,   // 闪3下后常亮
};

// 全局状态，由定时器 ISR 读取
static volatile LedPattern _led_pattern = LedPattern::OFF;
static volatile bool       _led_hw_on   = false;
static volatile uint8_t    _led_triple_cnt = 0;
static volatile uint32_t   _led_pattern_start = 0;

static void IRAM_ATTR ledTimerIsr() {
    uint32_t now = millis();
    uint32_t elapsed = now - _led_pattern_start;

    switch (_led_pattern) {
        case LedPattern::OFF:
            _led_hw_on = false;
            break;
        case LedPattern::ON:
            _led_hw_on = true;
            break;
        case LedPattern::SLOW_BLINK:
            _led_hw_on = ((elapsed / 500) % 2 == 0);
            break;
        case LedPattern::FAST_BLINK:
            _led_hw_on = ((elapsed / 150) % 2 == 0);
            break;
        case LedPattern::DOUBLE_BLINK: {
            uint32_t phase = elapsed % 700;
            _led_hw_on = (phase < 100) || (phase >= 200 && phase < 300);
            break;
        }
        case LedPattern::TRIPLE_BLINK: {
            uint32_t phase = elapsed % 1000;
            _led_hw_on = (phase < 100) || (phase >= 200 && phase < 300) || (phase >= 400 && phase < 500);
            // 1秒后转为常亮
            if (elapsed >= 1000) {
                _led_pattern = LedPattern::ON;
                _led_hw_on = true;
            }
            break;
        }
    }

    // 直接寄存器操作，避免 digitalWrite 开销
    if (LED_ACTIVE_LOW ? _led_hw_on : !_led_hw_on) {
        GPIO.out_w1tc.val = (1 << LED_PIN);
    } else {
        GPIO.out_w1ts.val = (1 << LED_PIN);
    }
}

class LedStatus {
public:
    void begin() {
        pinMode(LED_PIN, OUTPUT);
        digitalWrite(LED_PIN, LED_ACTIVE_LOW ? HIGH : LOW);

#if ESP_ARDUINO_VERSION_MAJOR >= 3
        // ESP32 Arduino Core 3.x (IDF 5.x): 新 timer API
        _timer = timerBegin(100);  // 100Hz → 10ms 周期
        timerAttachInterrupt(_timer, &ledTimerIsr);
#else
        // ESP32 Arduino Core 2.x: 旧 timer API
        // APB 时钟 80MHz, 分频 800 → 100kHz, 计数到 1000 → 10ms 中断
        _timer = timerBegin(0, 800, true);
        timerAttachInterrupt(_timer, &ledTimerIsr, true);
        timerAlarmWrite(_timer, 1000, true);  // 每1000个tick = 10ms
        timerAlarmEnable(_timer);
#endif
    }

    void set(LedPattern p) {
        noInterrupts();
        _led_pattern = (volatile LedPattern)p;
        _led_pattern_start = millis();
        interrupts();
    }

    // 无需 update()，定时器自动处理
    void update() {}

private:
    hw_timer_t *_timer = nullptr;
};

extern LedStatus led;
