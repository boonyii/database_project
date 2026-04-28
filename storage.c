#include "storage.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int build_storage(const char *txtfile, const char *imgfile, int block_size) {
    FILE *fin = fopen(txtfile, "r");
    if (!fin) { perror("open input"); return 0; }
    FILE *fout = fopen(imgfile, "wb+");
    if (!fout) { perror("open output"); fclose(fin); return 0; }

    FileHeader hdr = {0};
    hdr.block_size = block_size;
    hdr.record_size = sizeof(Record);

    // reserve header space
    fseek(fout, sizeof(FileHeader), SEEK_SET);

    // skip header line
    char line[512];
    fgets(line, sizeof(line), fin);

    Record rec;
    int rec_count = 0;
    while (fgets(line, sizeof(line), fin)) {
        if (!parse_record(line, &rec)) continue;
        fwrite(&rec, sizeof(Record), 1, fout);
        rec_count++;
    }

    hdr.total_records = rec_count;
    hdr.records_per_block = block_size / sizeof(Record);
    hdr.num_blocks = (rec_count + hdr.records_per_block - 1) / hdr.records_per_block;

    rewind(fout);
    fwrite(&hdr, sizeof(hdr), 1, fout);

    fclose(fin);
    fclose(fout);
    return 1;
}

int print_stats(const char *imgfile) {
    FILE *f = fopen(imgfile, "rb");
    if (!f) { perror("open"); return 0; }

    FileHeader hdr;
    fread(&hdr, sizeof(hdr), 1, f);

    printf("Record size: %u bytes\n", hdr.record_size);
    printf("Total records: %u\n", hdr.total_records);
    printf("Block size: %u bytes\n", hdr.block_size);
    printf("Records per block: %u\n", hdr.records_per_block);
    printf("Total data blocks: %u\n", hdr.num_blocks);

    fclose(f);
    return 1;
}

int dump_records(const char *imgfile, int count) {
    FILE *f = fopen(imgfile, "rb");
    if (!f) { perror("open"); return 0; }

    FileHeader hdr;
    fread(&hdr, sizeof(hdr), 1, f);

    Record rec;
    for (int i = 0; i < count && i < (int)hdr.total_records; i++) {
        fread(&rec, sizeof(rec), 1, f);
        print_record(&rec);
    }

    fclose(f);
    return 1;
}
