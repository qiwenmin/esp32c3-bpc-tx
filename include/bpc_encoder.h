#pragma once
#include <Arduino.h>

// BPC 四进制编码: 将 2-bit 值转为低电平持续时间 (ms)
// 0 -> 100ms, 1 -> 200ms, 2 -> 300ms, 3 -> 400ms
static inline uint16_t bits_to_ms(uint8_t msb, uint8_t lsb) {
    return (msb * 2 + lsb + 1) * 100;
}

// 从 20 个 duration 值中提取比特位
static inline void collect_bits(const uint16_t s[], uint8_t start, uint8_t end, uint8_t *out) {
    uint8_t idx = 0;
    for (uint8_t i = start; i < end; i++) {
        uint8_t pv = (s[i] / 100) - 1;
        out[idx++] = (pv >> 1) & 1;
        out[idx++] = (pv >> 0) & 1;
    }
}

static inline uint8_t odd_parity(const uint8_t *bits, uint8_t count) {
    uint8_t sum = 0;
    for (uint8_t i = 0; i < count; i++) sum += bits[i];
    return sum % 2;
}

// 编码一帧 BPC 数据，返回 20 个 duration (ms)
// cst: {year, month, day, hour, minute, second, weekday(0=Mon..6=Sun)}
static void bpc_encode(const int cst[7], uint16_t out[20]) {
    uint8_t year    = cst[0];
    uint8_t month   = cst[1];
    uint8_t day     = cst[2];
    uint8_t hour    = cst[3];
    uint8_t minute  = cst[4];
    uint8_t second  = cst[5];
    uint8_t weekday = cst[6];  // 0=Mon

    uint8_t dow   = weekday + 1;         // BPC: 1=Mon..7=Sun
    uint8_t am_pm = (hour >= 12) ? 1 : 0;
    uint8_t hour12 = hour % 12;
    uint8_t year2  = year % 100;

    uint8_t sec40_bit = (second >= 40) ? 1 : 0;
    uint8_t sec20_bit = ((second % 40) >= 20) ? 1 : 0;

    out[0]  = 0;  // 帧间隔
    out[1]  = bits_to_ms(sec40_bit, sec20_bit);
    out[2]  = bits_to_ms(0, 0);
    out[3]  = bits_to_ms((hour12 >> 3) & 1, (hour12 >> 2) & 1);
    out[4]  = bits_to_ms((hour12 >> 1) & 1, (hour12 >> 0) & 1);
    out[5]  = bits_to_ms((minute >> 5) & 1, (minute >> 4) & 1);
    out[6]  = bits_to_ms((minute >> 3) & 1, (minute >> 2) & 1);
    out[7]  = bits_to_ms((minute >> 1) & 1, (minute >> 0) & 1);
    out[8]  = bits_to_ms(0, (dow >> 2) & 1);
    out[9]  = bits_to_ms((dow >> 1) & 1, (dow >> 0) & 1);

    // P3 奇偶校验 = sec01~sec09 的奇偶
    uint8_t p3_bits[18];
    collect_bits(out, 1, 10, p3_bits);
    uint8_t p3 = odd_parity(p3_bits, 18);
    out[10] = bits_to_ms(am_pm, p3);

    out[11] = bits_to_ms(0, (day >> 4) & 1);
    out[12] = bits_to_ms((day >> 3) & 1, (day >> 2) & 1);
    out[13] = bits_to_ms((day >> 1) & 1, (day >> 0) & 1);
    out[14] = bits_to_ms((month >> 3) & 1, (month >> 2) & 1);
    out[15] = bits_to_ms((month >> 1) & 1, (month >> 0) & 1);
    out[16] = bits_to_ms((year2 >> 5) & 1, (year2 >> 4) & 1);
    out[17] = bits_to_ms((year2 >> 3) & 1, (year2 >> 2) & 1);
    out[18] = bits_to_ms((year2 >> 1) & 1, (year2 >> 0) & 1);

    uint8_t year64 = (year2 >> 6) & 1;
    uint8_t p4_bits[18];
    collect_bits(out, 11, 19, p4_bits);
    uint8_t p4 = odd_parity(p4_bits, 18);
    out[19] = bits_to_ms(year64, p4);
}
