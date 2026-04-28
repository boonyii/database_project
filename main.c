#include "storage.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage:\n");
        fprintf(stderr, "  %s build <input.txt> <output.img> <blocksize>\n", argv[0]);
        fprintf(stderr, "  %s stats <output.img>\n", argv[0]);
        fprintf(stderr, "  %s dump <output.img> <count>\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "build") == 0) {
        if (argc != 5) { fprintf(stderr, "build requires input, output, blocksize\n"); return 1; }
        return build_storage(argv[2], argv[3], atoi(argv[4])) ? 0 : 1;
    }
    else if (strcmp(argv[1], "stats") == 0) {
        return print_stats(argv[2]) ? 0 : 1;
    }
    else if (strcmp(argv[1], "dump") == 0) {
        if (argc != 4) { fprintf(stderr, "dump requires output.img and count\n"); return 1; }
        return dump_records(argv[2], atoi(argv[3])) ? 0 : 1;
    }

    fprintf(stderr, "Unknown command.\n");
    return 1;
}
