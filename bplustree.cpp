#include "bplustree.h"
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

//const int MAX_KEYS = 4; //max number of keys a node can hold (n) before needing to split
/*
struct BPlusNode { //Structure for B+ tree node
    bool is_leaf; // true if leaf node, false if internal node
    int key_count; // current number of keys
    float keys[MAX_KEYS]; // sorted keys for node - FT_PCT_home values
    std::vector<long> pointers; // record offsets or child node indices
    BPlusNode* next; // links leaves for chaining
    std::vector<BPlusNode*> children; // for internal nodes
    BPlusNode* parent; // pointer to parent node

BPlusNode(bool leaf)
        : is_leaf(leaf), key_count(0), next(nullptr), parent(nullptr) { // <-- add parent(nullptr)
        pointers.resize(MAX_KEYS + 1, -1);
        children.resize(MAX_KEYS + 2, nullptr);
        // Optional hygiene (not required): zero keys array
        for (int i = 0; i < MAX_KEYS; ++i) keys[i] = 0.0f;
    }
};*/

BPlusNode* root = nullptr;

BPlusNode* findLeaf(BPlusNode* node, float key) {
    while (node && !node->is_leaf) {
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

void insertIntoParent(BPlusNode* left, float key, BPlusNode* right) {
    // Root case
    if (left->parent == nullptr) {
        BPlusNode* newRoot = new BPlusNode(false);
        newRoot->keys[0] = key;
        newRoot->key_count = 1;
        newRoot->children[0] = left;
        newRoot->children[1] = right;
        left->parent = newRoot;
        right->parent = newRoot;
        root = newRoot;
        return;
    }

    BPlusNode* parent = left->parent;

    // Locate left in parent's children
    int leftIndex = 0;
    while (leftIndex <= parent->key_count && parent->children[leftIndex] != left) {
        leftIndex++;
    }
    if (leftIndex > parent->key_count) {
        std::cerr << "[Error] insertIntoParent: left child not found in parent.\n";
        return;
    }

    //parent has space
    if (parent->key_count < MAX_KEYS) {
        for (int i = parent->key_count; i > leftIndex; --i) {
            parent->keys[i] = parent->keys[i - 1];
            parent->children[i + 1] = parent->children[i];
        }
        parent->keys[leftIndex] = key;
        parent->children[leftIndex + 1] = right;
        parent->key_count++;
        right->parent = parent;
        return;
    }

    //parent is full: merge-split-promote
    float tempKeys[MAX_KEYS + 1];
    BPlusNode* tempChildren[MAX_KEYS + 2];

    for (int i = 0; i <= parent->key_count; ++i) tempChildren[i] = parent->children[i];
    for (int i = 0; i < parent->key_count; ++i) tempKeys[i] = parent->keys[i];

    // Insert at leftIndex
    for (int i = parent->key_count; i > leftIndex; --i) {
        tempKeys[i] = tempKeys[i - 1];
        tempChildren[i + 1] = tempChildren[i];
    }
    tempKeys[leftIndex] = key;
    tempChildren[leftIndex + 1] = right;

    int totalKeys = parent->key_count + 1;     // == MAX_KEYS + 1
    int midIndex = totalKeys / 2;
    float promoteKey = tempKeys[midIndex];

    // Left (reuse parent)
    parent->key_count = midIndex;
    for (int i = 0; i < parent->key_count; ++i) parent->keys[i] = tempKeys[i];
    for (int i = 0; i <= parent->key_count; ++i) {
        parent->children[i] = tempChildren[i];
        if (parent->children[i]) parent->children[i]->parent = parent;
    }
    for (int i = parent->key_count + 1; i < MAX_KEYS + 2; ++i) {
        parent->children[i] = nullptr;
    }

    // Right (new internal)
    BPlusNode* newInternal = new BPlusNode(false);
    newInternal->key_count = totalKeys - midIndex - 1;
    for (int i = 0; i < newInternal->key_count; ++i)
        newInternal->keys[i] = tempKeys[midIndex + 1 + i];
    for (int i = 0; i <= newInternal->key_count; ++i) {
        newInternal->children[i] = tempChildren[midIndex + 1 + i];
        if (newInternal->children[i]) newInternal->children[i]->parent = newInternal;
    }
    newInternal->parent = parent->parent;

    // Promote upward
    insertIntoParent(parent, promoteKey, newInternal);
}


void splitInternal(BPlusNode* node) {
    int midIndex = node->key_count / 2;
    float midKey = node->keys[midIndex];

    BPlusNode* newInternal = new BPlusNode(false);
    newInternal->key_count = node->key_count - midIndex - 1;

    // Move keys and children to new internal node
    for (int i = 0; i < newInternal->key_count; i++) {
        newInternal->keys[i] = node->keys[midIndex + 1 + i];
    }
    for (int i = 0; i <= newInternal->key_count; i++) {
        newInternal->children[i] = node->children[midIndex + 1 + i];
        if (newInternal->children[i]) {
            newInternal->children[i]->parent = newInternal;
        }
    }

    // Update key count of original node
    node->key_count = midIndex;

    // Set parent pointer for new internal node
    newInternal->parent = node->parent;

    // Promote middle key to parent
    insertIntoParent(node, midKey, newInternal);
}

// Split a full leaf node
void splitLeaf(BPlusNode* leaf) {
    int splitIndex = (MAX_KEYS + 1) / 2; // ceil((MAX_KEYS+1)/2)
    BPlusNode* newLeaf = new BPlusNode(true);

    // Move upper half to newLeaf
    newLeaf->key_count = leaf->key_count - splitIndex;
    for (int i = 0; i < newLeaf->key_count; i++) {
        newLeaf->keys[i] = leaf->keys[splitIndex + i];
        newLeaf->pointers[i] = leaf->pointers[splitIndex + i];
        leaf->keys[splitIndex + i] = 0.0f;
        leaf->pointers[splitIndex + i] = -1;
    }
    leaf->key_count = splitIndex;

    // Link leaves
    newLeaf->next = leaf->next;
    leaf->next = newLeaf;

    // Set parent for the new leaf
    newLeaf->parent = leaf->parent;

    // Promote first key of the new leaf into parent
    insertIntoParent(leaf, newLeaf->keys[0], newLeaf);
}

// Insert key into leaf node
void insertIntoLeaf(BPlusNode* leaf, float key, long offset) {
    // Split first if full
    if (leaf->key_count == MAX_KEYS) {
        splitLeaf(leaf);
        // Decide correct leaf after split using separator (first key in right leaf)
        if (key >= leaf->next->keys[0]) {
            leaf = leaf->next;
        }
    }

    // Insert in sorted order (now guaranteed leaf->key_count < MAX_KEYS)
    int i = leaf->key_count - 1;
    while (i >= 0 && leaf->keys[i] > key) {
        leaf->keys[i + 1] = leaf->keys[i];
        leaf->pointers[i + 1] = leaf->pointers[i];
        i--;
    }
    leaf->keys[i + 1] = key;
    leaf->pointers[i + 1] = offset;
    leaf->key_count++;
}

// Public insert
void bplustree_insert(float key, long offset) {
    if (!root) {
        // Tree is empty, create root as leaf
        root = new BPlusNode(true);
        root->keys[0] = key;
        root->pointers[0] = offset;
        root->key_count = 1;
        return;
    }
    BPlusNode* leaf = findLeaf(root, key);
    insertIntoLeaf(leaf, key, offset);
}

// Build B+ tree from binary file
void bplustree_build(const std::string& binFile) {
    std::ifstream datafile(binFile, std::ios::binary);

    FileHeader hdr;
    datafile.read(reinterpret_cast<char*>(&hdr), sizeof(FileHeader));

    Record rec;
    long offset = datafile.tellg();
    while (datafile.read(reinterpret_cast<char*>(&rec), sizeof(Record))) {
        float key = decode_pct(rec.ft_pct);  // use utils.c decode
        bplustree_insert(key, offset);
        offset = datafile.tellg();
    }
    datafile.close();
}

// Print B+ tree
void printTree(BPlusNode* node, int level) {
    if (!node) return;
    // Label node type
    std::cout << "Level " << level 
              << (node->is_leaf ? " (Leaf)" : " (Internal)") 
              << " | Keys: [";

    for (int i = 0; i < node->key_count; i++) {
        std::cout << node->keys[i];
        if (i < node->key_count - 1) std::cout << ", ";
    }
    std::cout << "]";
    // Show extra info
    if (node->is_leaf) {
        std::cout << " | Records: " << node->key_count;
        if (node->next) std::cout << " | Next leaf starts at key " << node->next->keys[0];
    } else {
        std::cout << " | Children: " << (node->key_count + 1);
    }
    std::cout << "\n";
    // Recurse into children if internal
    if (!node->is_leaf) {
        for (int i = 0; i <= node->key_count; i++) {
            printTree(node->children[i], level + 1);
        }
    }
}

// Count total nodes in the tree
int countNodes(BPlusNode* node) {
    if (!node) return 0;
    int count = 1; // count this node
    if (!node->is_leaf) {
        for (int i = 0; i <= node->key_count; i++) {
            count += countNodes(node->children[i]);
        }
    }
    return count;
}

// get height (levels) of the tree
int treeHeight(BPlusNode* node) {
    if (!node) return 0;
    int height = 0;
    while (node) {
        height++;
        if (!node->is_leaf) node = node->children[0];
        else break;
    }
    return height;
}

// Print only the keys in the root node
void printRootKeys(BPlusNode* root) {
    std::cout << "[Task 2] Root node keys: ";
    for (int i = 0; i < root->key_count; i++) {
        std::cout << root->keys[i];
        if (i < root->key_count - 1) std::cout << ", ";
    }
    std::cout << "\n";
}

// Gather and print Task 2 stats
void gatherStats(BPlusNode* root, int n) {
    std::cout << "\n--- B+ Tree Statistics ---\n";
    std::cout << "Max keys per node (n): " << n << "\n";
    std::cout << "Number of nodes: " << countNodes(root) << "\n";
    std::cout << "Number of levels: " << treeHeight(root) << "\n";
    printRootKeys(root);
    std::cout << "----------------------------\n";
}



void collectNodes(BPlusNode* root, std::vector<BPlusNode*>& nodes) {
    if (!root) return;
    std::queue<BPlusNode*> q;
    q.push(root);
    while (!q.empty()) {
        BPlusNode* node = q.front(); q.pop();
        nodes.push_back(node);
        if (!node->is_leaf) {
            for (int i = 0; i <= node->key_count; i++) {
                if (node->children[i]) q.push(node->children[i]);
            }
        }
    }
}

void saveTreeToDisk(const std::string& filename, BPlusNode* root) {
    std::vector<BPlusNode*> nodes;
    collectNodes(root, nodes);

    // Map each pointer to its index
    std::unordered_map<BPlusNode*, long> idMap;
    for (size_t i = 0; i < nodes.size(); i++) {
        idMap[nodes[i]] = static_cast<long>(i);
    }

    std::ofstream out(filename, std::ios::binary);
    for (BPlusNode* node : nodes) {
        DiskBPlusNode diskNode{};
        diskNode.is_leaf = node->is_leaf;
        diskNode.key_count = node->key_count;
        for (int i = 0; i < node->key_count; i++) {
            diskNode.keys[i] = node->keys[i];
        }
        if (node->is_leaf) {
            for (int i = 0; i < node->key_count; i++) {
                diskNode.pointers[i] = node->pointers[i]; // record offsets
            }
            diskNode.next = node->next ? idMap[node->next] : -1;
        } else {
            for (int i = 0; i <= node->key_count; i++) {
                diskNode.pointers[i] = node->children[i] ? idMap[node->children[i]] : -1;
            }
            diskNode.next = -1;
        }
        out.write(reinterpret_cast<char*>(&diskNode), sizeof(DiskBPlusNode));
    }
    out.close();
}
bool fileExistsAndNotEmpty(const std::string& filename) {
    struct stat buffer{};
    if (stat(filename.c_str(), &buffer) != 0) return false; // file not found
    return buffer.st_size > 0;
}
