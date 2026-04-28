#ifdef __cplusplus
extern "C" {
#endif
#ifndef STORAGE_H
#define STORAGE_H

#include "record.h"
#include <stdint.h>

typedef struct {
    uint32_t block_size;
    uint32_t record_size;
    uint32_t total_records;
    uint32_t records_per_block;
    uint32_t num_blocks;
} FileHeader;

int build_storage(const char *txtfile, const char *imgfile, int block_size);
int print_stats(const char *imgfile);
int dump_records(const char *imgfile, int count);

#endif
#ifdef __cplusplus
}
#endif