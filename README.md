# SC3020/CZ4031 Project 1 - AY25/26 S1 - SCEL Group 19

## Introduction
This project is to design and implement the following two 
components of a database management system, storage and indexing.  
- **(1)** For the storage component, the following settings are assumed. 
    - the data is stored on disk;  
    - to avoid the risk of corrupting the file system on your disk, you can use either a disk image file or a dummy binary file to simulate a disk **(bin files will be used)**; 
- **(2)** For the indexing component, the following settings are assumed. 
    - a B+ tree is used; the B+ tree should be stored on disk; and 
    - for building the B+ tree, you can use either the method of iteratively inserting the records OR the bulk loading method; 

## Task Requirements
- **Task 1**: Design and implement the storage component based on the 
settings described in Part 1 and store the data (which is about NBA games and will be described in Part 4).  
    - Describe the content of a record, a block, and a database file; 
    - Report the following statistics: the size of a record; the number of records; the number of records stored in a block; the number of blocks for storing the data; 
- **Task 2**: Design and implement the indexing component based on the settings described in Part 1 and build a B+ tree on the data described in Task 1 with the attribute "FT_PCT_home" as the key.  
    - Report the following statistics: the parameter n of the B+ tree; the number of nodes of the B+ tree; the number of levels of the B+ tree; the content of the root node (only the keys); 
- **Task 3**: Delete those movies with the attribute “FT_PCT_home” above 0.9 using the B+ tree 
    - Report the following statistics: the number of index nodes the process accesses; the number of data blocks the process accesses; the 
number of games deleted; the average of “FT_PCT_home” of the 
records that are returned; the running time of the retrieval process; the number of data blocks that would be accessed by a brute-force linear scan method (i.e., it scans the data blocks one by one) and its running time (for comparison); 
    - Also report the following statistics of the updated B+ tree: the number of nodes of the B+ tree; the number of levels of the B+ tree; the content of the root node (only the keys); 

---

## Prerequisites

- A C++17‑capable compiler:
  - **Linux/macOS**: `g++` ≥ 9 or Clang ≥ 10
  - **Windows**: Visual Studio 2019+ (MSVC) or MinGW‑w64
- Build tools:
  - On Linux/macOS: install via package manager (`sudo apt install g++` or `brew install gcc`)
  - On Windows: install [Visual Studio Build Tools](https://visualstudio.microsoft.com/visual-cpp-build-tools/) or [MinGW‑w64](http://mingw-w64.org/)
- Input file: `games.txt` (provided with assignment)

---
## Included Files

- `games.txt` – input dataset
- `main.exe` – prebuilt Windows executable
- `build.bat` – batch file to compile the project (Windows)
- `main.cpp` - main file
- `record.c / record.h` - Task 1 - Record
- `storage.c / storage.h` - Task 1 - Storage
- `utils.c / utils.h` - Task 1 - Utilities
- `bplustree.cpp / bplustree.h` - Task 2 - B+ Tree Implementation
- `deletion.cpp / deletion.h` - Task 3 - Deletion from B+ Tree
- `README.md` – installation & usage guide

---

## Building

### Using g++ (Linux/macOS/MinGW on Windows)
- **1**: Download all required build files into same folder
- **2**: Open console and navigate to folder location
- **3**: Build Command
```bash
g++ main.cpp storage.c bplustree.cpp deletion.cpp record.c utils.c -o main