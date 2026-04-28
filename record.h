#ifdef __cplusplus
extern "C" {
#endif
#ifndef RECORD_H
#define RECORD_H

#include <stdint.h>

// Compact packed record for one NBA game row (17 bytes)
#pragma pack(push, 1)
typedef struct {
    uint8_t  date[3];    // 3 bytes: days since 2000-01-01
    uint32_t team_id;    
    uint8_t  pts;        // 1 byte: 0–255
    uint16_t fg_pct;     // 2 bytes: fixed-point scaled *1000
    uint16_t ft_pct;     // 2 bytes: fixed-point scaled *1000
    uint16_t fg3_pct;    // 2 bytes: fixed-point scaled *1000
    uint8_t  ast;        // 1 byte: 0–99
    uint8_t  reb;        // 1 byte: 0–99
    uint8_t  win;        // 1 byte: 0/1
} Record;
#pragma pack(pop)

// Parse a TSV line into a Record
int parse_record(const char *line, Record *rec);

// Print a decoded Record
void print_record(const Record *rec);
#endif
#ifdef __cplusplus
}
#endif

