#include "utils.h"
#include <stdio.h>
#include <time.h>

// === Date encoding ===
void encode_date(const char *datestr, uint8_t out[3]) {
    int d, m, y;
    if (sscanf(datestr, "%d/%d/%d", &d, &m, &y) != 3) {
        d = 1; m = 1; y = 2000;
    }
    struct tm base = {0}, cur = {0};
    base.tm_mday = 1; base.tm_mon = 0; base.tm_year = 100; // 2000-01-01
    cur.tm_mday = d; cur.tm_mon = m - 1; cur.tm_year = y - 1900;
    time_t base_t = mktime(&base);
    time_t cur_t = mktime(&cur);
    int days = (int)((cur_t - base_t) / 86400);
    out[0] = days & 0xFF;
    out[1] = (days >> 8) & 0xFF;
    out[2] = (days >> 16) & 0xFF;
}

void decode_date(const uint8_t in[3], char *outbuf, int buflen) {
    int days = in[0] | (in[1] << 8) | (in[2] << 16);
    struct tm base = {0};
    base.tm_mday = 1; base.tm_mon = 0; base.tm_year = 100; // 2000-01-01
    time_t base_t = mktime(&base);
    time_t cur_t = base_t + (time_t)days * 86400;
    struct tm *dt = localtime(&cur_t);
    snprintf(outbuf, buflen, "%02d/%02d/%04d", dt->tm_mday, dt->tm_mon + 1, dt->tm_year + 1900);
}

// === Percentages ===
uint16_t encode_pct(float pct) {
    if (pct < 0) pct = 0;
    if (pct > 1) pct = 1;
    return (uint16_t)(pct * 1000 + 0.5);
}

float decode_pct(uint16_t val) {
    return val / 1000.0f;
}

// === Team ID compression ===
uint8_t compress_team_id(uint32_t full_id) {
    return (uint8_t)(full_id % 100); // last two digits
}

uint32_t expand_team_id(uint8_t short_id) {
    return 1610612700u + short_id;
}
