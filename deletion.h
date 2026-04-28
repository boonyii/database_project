#ifndef DELETION_H
#define DELETION_H

#include <string>
#include "bplustree.h"
#include "storage.h"
#include "record.h"


// Task 3: Query statistics
struct QueryStats {
    int indexNodesAccessed = 0;   // internal index nodes visited
    int leafNodesVisited   = 0;   // leaves scanned (optional, not always required)
    int dataBlocksAccessed = 0;   // unique data blocks touched
    int gamesDeleted       = 0;   // number of records deleted
    double sumFT           = 0.0; // sum of FT_PCT_home for deleted records
    double elapsedQuerySec = 0.0; // runtime of deletion query

    int bruteForceDeletions = 0;  // records found in brute-force scan
    int bruteForceBlocks    = 0;  // blocks scanned in brute-force
    double elapsedBruteSec  = 0.0;// runtime of brute-force scan
};

// Public Task 3 functions

void rebalanceAfterInternalDelete(BPlusNode* node);

// Delete all records with FT_PCT_home > threshold using the B+ tree
void deleteOverthreshold(float threshold, QueryStats& stats, const FileHeader& hdr);

// Brute-force scan of disk.bin for comparison
void bruteForceScan(const std::string& binFile, float threshold, QueryStats& stats);

// Print Task 3 statistics in assignment format
void printQueryStats(const QueryStats& stats);

#endif // DELETION_H