#ifndef BPLUSTREE_H
#define BPLUSTREE_H

#include <vector>
#include <string>
#include "record.h"

// Maximum number of keys per node
const int MAX_KEYS = 4;

// B+ tree node structure
struct BPlusNode {
    bool is_leaf;
    int key_count;
    float keys[MAX_KEYS];
    std::vector<long> pointers;          // record offsets (leaf)
    BPlusNode* next;                     // leaf chain
    std::vector<BPlusNode*> children;    // internal node children
    BPlusNode* parent;

    BPlusNode(bool leaf)
        : is_leaf(leaf), key_count(0), next(nullptr), parent(nullptr) {
        pointers.resize(MAX_KEYS + 1, -1);
        children.resize(MAX_KEYS + 2, nullptr);
        for (int i = 0; i < MAX_KEYS; ++i) keys[i] = 0.0f;
    }
};

// Global root pointer
extern BPlusNode* root;

// Core operations
BPlusNode* findLeaf(BPlusNode* node, float key);
void bplustree_insert(float key, long offset);
void bplustree_build(const std::string& binFile);

// Stats and debugging
void printTree(BPlusNode* node, int level = 0);
int countNodes(BPlusNode* node);
int treeHeight(BPlusNode* node);
void printRootKeys(BPlusNode* root);
void gatherStats(BPlusNode* root, int n);

// Disk persistence
struct DiskBPlusNode {
    bool is_leaf;
    int key_count;
    float keys[MAX_KEYS];
    long pointers[MAX_KEYS + 1];   // record offsets (leaf) or child IDs
    long next;                     // next leaf index or -1
};

void saveTreeToDisk(const std::string& filename, BPlusNode* root);
bool fileExistsAndNotEmpty(const std::string& filename);

#endif // BPLUSTREE_H
