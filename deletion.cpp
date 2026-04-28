#include "deletion.h"
#include "iostream"
#include "fstream"
#include "sstream"
#include "vector"
#include "cmath"
#include "ctime"
#include "queue"
#include "unordered_map"
#include "sys/stat.h"
#include "storage.h"
#include "record.h"
#include "utils.h"
#include "unordered_set"
#include "chrono"


// global set to track unique blocks[Task 3]
std::unordered_set<int> blocksSeen;
// Brute force scan raw count
int rawCount = 0;

extern void rebalanceAfterInternalDelete(BPlusNode* node);

BPlusNode* findLeaf(BPlusNode* node, float key,QueryStats& stats) {
    while (node && !node->is_leaf) {
        stats.indexNodesAccessed++;
        int i = 0;
        while (i < node->key_count && key >= node->keys[i]) ++i;
        BPlusNode* child = node->children[i];
        if (!child) {
            std::cerr << "[ERROR] findLeaf: null child at index " << i << "\n";
            return node; // bail out safely
        }
        node = child;
    }
    return node;
}

// Minimum occupancy for leaves and internals (underflow)
int minLeafOccupancy() { return (MAX_KEYS + 1) / 2; }
int minInternalOccupancy() { return (MAX_KEYS + 1) / 2; } // keys; children = keys+1


//deletes 1 entry from leaf
void deleteEntryFromLeaf(BPlusNode* leaf, int pos)
{
    
    for(int i=pos;i<leaf->key_count-1;i++)
    {
        leaf->keys[i]=leaf->keys[i+1];
        leaf->pointers[i]=leaf->pointers[i+1];
    }
    leaf->key_count--;
}

// Find this node's index in its parent children array
int indexInParent(BPlusNode* node) {
    BPlusNode* p = node->parent;
    if (!p) return -1;
    for (int i = 0; i <= p->key_count; ++i) {
        if (p->children[i] == node) return i;
    }
    return -1; // not found -> bug
}

//Get siblings under the same parent
BPlusNode* leftSibling(BPlusNode* node) {
    int idx = indexInParent(node);
    if (idx <= 0) return nullptr;
    return node->parent->children[idx - 1];
}
BPlusNode* rightSibling(BPlusNode* node) {
    int idx = indexInParent(node);
    if (idx < 0) return nullptr;
    if (idx >= node->parent->key_count) return nullptr;
    return node->parent->children[idx + 1];
}
// Borrow one entry from the left leaf into 'leaf'
bool borrowFromLeftLeaf(BPlusNode* leaf, BPlusNode* left) {
    if (!left || !leaf->parent) return false;
    if (left->key_count <= minLeafOccupancy()) return false; // can't lend

    // Make room at front of leaf
    for (int i = leaf->key_count; i > 0; --i) {
        leaf->keys[i]     = leaf->keys[i - 1];
        leaf->pointers[i] = leaf->pointers[i - 1];
    }
    // Move left's rightmost entry
    leaf->keys[0]     = left->keys[left->key_count - 1];
    leaf->pointers[0] = left->pointers[left->key_count - 1];
    leaf->key_count++;
    left->key_count--;

    // Update parent separator between (left, leaf)
    int iLeaf = indexInParent(leaf);
    if (iLeaf - 1 >= 0) {
        leaf->parent->keys[iLeaf - 1] = leaf->keys[0];
    }
    return true;
}

// Borrow one entry from the right leaf into 'leaf'
bool borrowFromRightLeaf(BPlusNode* leaf, BPlusNode* right) {
    if (!right || !leaf->parent) return false;
    if (right->key_count <= minLeafOccupancy()) return false; // can't lend

    // Append right's leftmost entry to leaf
    leaf->keys[leaf->key_count]     = right->keys[0];
    leaf->pointers[leaf->key_count] = right->pointers[0];
    leaf->key_count++;

    // Shift right left
    for (int i = 0; i < right->key_count - 1; ++i) {
        right->keys[i]     = right->keys[i + 1];
        right->pointers[i] = right->pointers[i + 1];
    }
    right->key_count--;

    // Update parent separator between (leaf, right) to right's new first key
    int iLeaf = indexInParent(leaf);
    if (iLeaf < leaf->parent->key_count) {
        leaf->parent->keys[iLeaf] = right->keys[0];
    }
    return true;
}
bool rebalanceAfterLeafDelete(BPlusNode* leaf) {
    if (!leaf) return true;

    // Root as leaf can be allowed fewer keys; defer special-case until merging phase
    if (leaf == root) return true;

    if (leaf->key_count >= minLeafOccupancy()) return true; // no underflow

    BPlusNode* left  = leftSibling(leaf);
    BPlusNode* right = rightSibling(leaf);

    // Try borrowing (cheap, preferred)
    if (borrowFromLeftLeaf(leaf, left))  return true;
    if (borrowFromRightLeaf(leaf, right)) return true;

    return false; // couldn't borrow

}

// Merging

void mergeLeaves(BPlusNode* left, BPlusNode* right) {
    if (!left || !right) return;
    BPlusNode* parent = left->parent;
    if (!parent || right->parent != parent) {
        std::cerr << "[ERROR] mergeLeaves: mismatched or null parent\n";
        return;
    }

    // Move keys/pointers
    for (int i = 0; i < right->key_count; ++i) {
        left->keys[left->key_count + i]     = right->keys[i];
        left->pointers[left->key_count + i] = right->pointers[i];
    }
    left->key_count += right->key_count;

    // Fix leaf chain
    left->next = right->next;

    // Remove 'right' child and its separator (idxRight-1)
    int idxRight = indexInParent(right);
    if (idxRight < 0) { std::cerr << "[ERROR] mergeLeaves: right not found in parent\n"; return; }

    // Shift children left from idxRight+1 → idxRight
    for (int i = idxRight + 1; i <= parent->key_count; ++i) {
        parent->children[i - 1] = parent->children[i];
    }
    parent->children[parent->key_count] = nullptr;

    // Shift keys left from (idxRight) → (idxRight-1) i.e., remove separator at idxRight-1
    for (int i = idxRight; i < parent->key_count - 1; ++i) {
        parent->keys[i - 1] = parent->keys[i];
    }
    parent->key_count--;

    delete right;

    if (parent != root && parent->key_count < minInternalOccupancy()) {
        rebalanceAfterInternalDelete(parent);
    }
}

void mergeInternals(BPlusNode* left, BPlusNode* right, int sepIdx) {
     if (!left || !right) return;
    BPlusNode* parent = left->parent;
    if (!parent || right->parent != parent) {
        std::cerr << "[ERROR] mergeInternals: mismatched or null parent\n";
        return;
    }
    if (sepIdx < 0 || sepIdx >= parent->key_count) {
        std::cerr << "[ERROR] mergeInternals: invalid sepIdx\n";
        return;
    }

    // Base offset before appending
    int base = left->key_count;

    // 1) Bring down separator between left|right
    left->keys[base] = parent->keys[sepIdx];
    // 2) Append right's keys
    for (int i = 0; i < right->key_count; ++i) {
        left->keys[base + 1 + i] = right->keys[i];
    }
    // 3) Append right's children (there are right->key_count + 1 children)
    for (int i = 0; i <= right->key_count; ++i) {
        left->children[base + 1 + i] = right->children[i];
        if (left->children[base + 1 + i]) {
            left->children[base + 1 + i]->parent = left;
        }
    }
    left->key_count = base + 1 + right->key_count;

    // 4) Remove separator and 'right' child from parent
    // Right is at index sepIdx+1 among children
    for (int i = sepIdx; i < parent->key_count - 1; ++i) {
        parent->keys[i] = parent->keys[i + 1];
    }
    for (int i = sepIdx + 1; i < parent->key_count; ++i) {
        parent->children[i] = parent->children[i + 1];
    }
    parent->children[parent->key_count] = nullptr;
    parent->key_count--;

    // 5) Delete right
    delete right;

    // 6) Rebalance parent if underflow
    if (parent != root && parent->key_count < minInternalOccupancy()) {
        rebalanceAfterInternalDelete(parent);
    }
}

void borrowFromLeftInternal(BPlusNode* node, BPlusNode* left, BPlusNode* parent, int idx) {
    for (int i = node->key_count; i >= 0; --i) {
        node->children[i + 1] = node->children[i];
    }
    for (int i = node->key_count - 1; i >= 0; --i) {
        node->keys[i + 1] = node->keys[i];
    }

    // Bring down separator and adopt left’s rightmost child
    node->keys[0] = parent->keys[idx - 1];
    node->children[0] = left->children[left->key_count]; // right-most child of left
    if (node->children[0]) node->children[0]->parent = node;

    // Move left’s last key up into parent
    parent->keys[idx - 1] = left->keys[left->key_count - 1];

    left->key_count--;
    node->key_count++;
}

void borrowFromRightInternal(BPlusNode* node, BPlusNode* right, BPlusNode* parent, int idx) {
    node->keys[node->key_count] = parent->keys[idx];
    node->children[node->key_count + 1] = right->children[0];
    if (node->children[node->key_count + 1]) node->children[node->key_count + 1]->parent = node;

    // Move right’s first key up into parent
    parent->keys[idx] = right->keys[0];

    // Shift right’s keys and children left
    for (int i = 0; i < right->key_count - 1; ++i) {
        right->keys[i] = right->keys[i + 1];
        right->children[i] = right->children[i + 1];
    }
    right->children[right->key_count - 1] = right->children[right->key_count]; // last child slides
    right->key_count--;

    node->key_count++;
}

void rebalanceAfterInternalDelete(BPlusNode* node) {
    // Root special case
    if (node == root) {
        if (!node->is_leaf && node->key_count == 0) {
            root = node->children[0];
            root->parent = nullptr;
            delete node;
        }
        return;
    }

    if (node->key_count >= minInternalOccupancy()) return;

    BPlusNode* left  = leftSibling(node);
    BPlusNode* right = rightSibling(node);
    BPlusNode* parent = node->parent;
    int idx = indexInParent(node);

    // Borrow from left
    if (left && left->key_count > minInternalOccupancy()) {
        borrowFromLeftInternal(node, left, parent, idx);
        return;
    }

    // Borrow from right
    if (right && right->key_count > minInternalOccupancy()) {
        borrowFromRightInternal(node, right, parent, idx);
        return;
    }

    // Merge
    if (left) {
        mergeInternals(left, node, idx - 1);
    } else if (right) {
        mergeInternals(node, right, idx);
    }
}

//loop to delete each entry greater than threshold
void deleteOverthreshold(float threshold, QueryStats& stats, const FileHeader& hdr) {
    //start timer for stats
    auto start = std::chrono::high_resolution_clock::now();
    // If you run multiple queries, clear this at the start
    blocksSeen.clear();
    BPlusNode* leaf = findLeaf(root, threshold,stats);
    while (leaf) {
        bool deletedSomething = false;

        //delete all > threshold in this leaf
        for (int i = 0; i < leaf->key_count;) {
            if (leaf->keys[i] > threshold) {
                // stats
                stats.gamesDeleted++;
                stats.sumFT += leaf->keys[i];

                long offset = leaf->pointers[i];
                if (offset >= 0) {
                    int blockNum = offset / hdr.block_size;
                    if (!blocksSeen.count(blockNum)) {
                        blocksSeen.insert(blockNum);
                        stats.dataBlocksAccessed++;
                    }
                }
                deleteEntryFromLeaf(leaf, i);
                deletedSomething = true;
                // i stays the same to re-check the shifted-in key at position i
            } else {
                ++i;
            }
        }
        //still count leaf visit once per leaf
        stats.dataBlocksAccessed++;

        // Rebalance only if underflow (non-root)
        BPlusNode* nextLeaf = leaf->next;
        if (leaf != root && leaf->key_count < minLeafOccupancy()) {
            // Try rebalancing via borrowing
            bool ok = rebalanceAfterLeafDelete(leaf);
            if (!ok) {
                // couldn't borrow, must merge
                // Merge — determine survivor and update nextLeaf accordingly
                BPlusNode* right = rightSibling(leaf);
                BPlusNode* leftSib = leftSibling(leaf);

                if (right) {
                    // survivor = leaf
                    mergeLeaves(leaf, right);
                    nextLeaf = leaf->next;
                } else if (leftSib) {
                    // survivor = leftSib
                    mergeLeaves(leftSib, leaf);
                    leaf = leftSib;
                    nextLeaf = leftSib->next;
                }
            }

            // after borrowing/merging, keys may have moved into 'leaf' (the survivor).
            // Re-run deletion on the survivor before advancing.
            bool moreDeleted = false;
            for (int j = 0; j < leaf->key_count;) {
                if (leaf->keys[j] > threshold) {
                    stats.gamesDeleted++;
                    stats.sumFT += leaf->keys[j];

                    long offset = leaf->pointers[j];
                    if (offset >= 0) {
                        int blockNum = offset / hdr.block_size;
                        if (!blocksSeen.count(blockNum)) {
                            blocksSeen.insert(blockNum);
                            stats.dataBlocksAccessed++;
                        }
                    }

                    deleteEntryFromLeaf(leaf, j);
                    moreDeleted = true;
                } else {
                    ++j;
                }
            }

            // if deleted more after rebalance,causing underflow,
            // handle it in the next iteration of the while(leaf) loop.
        }

        leaf = nextLeaf;
    }
    //end timer for stats
    auto end = std::chrono::high_resolution_clock::now();
    stats.elapsedQuerySec = std::chrono::duration<double>(end - start).count();
}


//Task 3 - Brute force scan for comparison
// Sequentially scans all in disk.bin to check threshold to compare with
// the B+ tree deletion results
void bruteForceScan(const std::string& binFile, float threshold, QueryStats& stats) {
    auto start = std::chrono::high_resolution_clock::now();

    std::ifstream datafile(binFile, std::ios::binary);
    FileHeader hdr;
    datafile.read(reinterpret_cast<char*>(&hdr), sizeof(FileHeader));

    Record rec;
    while (datafile.read(reinterpret_cast<char*>(&rec), sizeof(Record))) {
        float key = decode_pct(rec.ft_pct);
            if (key > threshold) {   // or >= threshold, depending on your definition
            rawCount++;
        }
    }
    datafile.close();

    auto end = std::chrono::high_resolution_clock::now();
    stats.bruteForceBlocks = hdr.num_blocks;
    stats.elapsedBruteSec = std::chrono::duration<double>(end - start).count();
}

//Task 3 - Print out stats
void printQueryStats(const QueryStats& stats) {
    std::cout << "\n--- Task 3 Statistics ---\n";
    std::cout << "Index nodes accessed: " << stats.indexNodesAccessed << "\n";
    std::cout << "Data blocks accessed: " << stats.dataBlocksAccessed << "\n";
    std::cout << "Games deleted: " << stats.gamesDeleted << "\n";
    if (stats.gamesDeleted > 0)
        std::cout << "Average FT_PCT_home returned (deleted): " << (stats.sumFT / stats.gamesDeleted) << "\n";
    std::cout << "Query running time: " << stats.elapsedQuerySec << " sec\n";
    std::cout << "Brute-force blocks accessed: " << stats.bruteForceBlocks << "\n";
    std::cout << "Brute-force running time: " << stats.elapsedBruteSec << " sec\n";
    std::cout << "Raw count of records over threshold: " << rawCount << "\n";
    std::cout << "--------------------------\n";
}