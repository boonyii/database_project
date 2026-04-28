

//Replicate Indexing and Storage of a DBMS
/*
This project is to design and implement the following two
components of a database management system, storage and indexing.
(1) For the storage component, the following settings are assumed.
• the data is stored on disk;
• to avoid the risk of corrupting the file system on your disk, you can use
either a disk image file or a dummy binary file to simulate a disk;
(2) For the indexing component, the following settings are assumed.
• a B+ tree is used; the B+ tree should be stored on disk; and
• for building the B+ tree, you can use either the method of iteratively
inserting the records OR the bulk loading method;


*/

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
#include "bplustree.h"
#include "deletion.h"
#include "unordered_set"
#include "chrono"

/**/


int main() {
    int choice;
    do {
        std::cout << "\n=== Menu ===\n";
        std::cout << "1. Task 1: Build Storage\n";
        std::cout << "2. Task 2: Build B+ Tree Index\n";
        std::cout << "3. Task 3: Deletion Query\n";
        std::cout << "4. Run All Tasks in sequence\n";
        std::cout << "0. Exit\n";
        std::cout << "Enter choice: ";
        std::cin >> choice;

        if (choice == 1 || choice == 4) {
            // === Task 1: Build storage ===
            std::cout << "=== TASK 1: Build Storage ===\n";
            build_storage("games.txt", "disk.bin", 4096);
            print_stats("disk.bin");
        }
        if (choice == 2 || choice == 4) {    
            // === Task 2: Build B+ tree index ===
            std::cout << "\n=== TASK 2: Build B+ Tree ===\n";
            bplustree_build("disk.bin");
            gatherStats(root, MAX_KEYS);   // print initial stats
        }
        if (choice == 3 || choice == 4) {
            // === Task 3: Deletion query and stats ===
            std::cout << "\n=== TASK 3: Deletion Query ===\n";
            QueryStats stats;

            // Load header for block size info
            FileHeader hdr;
            std::ifstream datafile("disk.bin", std::ios::binary);
            datafile.read(reinterpret_cast<char*>(&hdr), sizeof(FileHeader));
            datafile.close();

            float threshold = 0.9f;
            deleteOverthreshold(threshold, stats, hdr);
            bruteForceScan("disk.bin", threshold, stats);
            printQueryStats(stats);

            // Updated tree stats after deletion
            std::cout << "\n=== Updated B+ Tree Stats ===\n";
            gatherStats(root, MAX_KEYS);

            // === Save tree to disk ===
            std::cout << "\n=== Writing B+ Tree to bplustree.bin ===\n";
            saveTreeToDisk("bplustree.bin", root);
            if (fileExistsAndNotEmpty("bplustree.bin"))
                std::cout << "B+ Tree index file written successfully.\n";
            else
                std::cerr << "[ERROR] Index file missing or empty.\n";
        }
    } while (choice != 0);

    return 0;

}

