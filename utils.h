#ifdef __cplusplus
extern "C" {
#endif
#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>

// Encode/decode date (dd/mm/yyyy) into 3-byte packed days since 2000-01-01
void encode_date(const char *datestr, uint8_t out[3]);
void decode_date(const uint8_t in[3], char *outbuf, int buflen);

// Encode/decode percentages (0.0–1.0) into uint16_t fixed *1000
uint16_t encode_pct(float pct);
float decode_pct(uint16_t val);

// Team ID helpers
uint8_t compress_team_id(uint32_t full_id);
uint32_t expand_team_id(uint8_t short_id);

#endif
#ifdef __cplusplus
}
#endif