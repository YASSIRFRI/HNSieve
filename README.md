# HNSieve: Efficient Filtering for HNSW with B+ Tree Integration

## Overview
**HNSieve** enhances the filtering mechanism of HNSWLib by integrating a custom-built B+ tree for pre-filtering data based on attribute ranges. This project optimizes approximate nearest neighbor (ANN) searches by narrowing the search space efficiently before deciding the search strategy:
- **Brute-Force Search** for small filtered subsets.
- **ANN Search** for larger subsets, followed by post-filtering.

By leveraging a B+ tree for pre-filtering and HNSW for ANN searches, this project combines the strengths of both structures to achieve high-performance and precise data queries.

---

## Features
1. **B+ Tree Implementation**:
   - A custom in-memory B+ tree optimized for range queries.
   - Supports efficient insertion, deletion, and querying operations.

2. **HNSWLib Integration**:
   - Pre-filters data points based on attribute ranges before ANN search.
   - Reduces unnecessary computations by narrowing down candidates.

3. **Dynamic Search Decision**:
   - Switches between brute-force and ANN search based on the size of the filtered subset.

4. **Performance Optimization**:
   - Minimizes memory usage and query time.
   - Ensures scalability for large datasets.

---

## Installation

### Prerequisites
- HNSWLib (available at [HNSWLib GitHub Repository](https://github.com/nmslib/hnswlib))
- A C++ compiler (e.g., GCC, Clang)



## Usage

### Compilation

#### 1. Compile the Main Program

The main program integrates the B+ tree with HNSW for efficient filtering and ANN searches.

**Source Files:**
- `main.cpp`
- `BpTree.h`
- `hnswlib\`
- Any additional source files required by your project (e.g., `io.h`, `io.cpp`)

**Compilation Command:**
```bash
g++ -O3 -std=c++14 -I./hnswlib -o hnsieve_main main.cpp 
```



**Run the Main Program:**

After compiling the main program, you can run it with the following command:

```bash
./hnsieve_main
```


**Compile the Test Program:**

The test program validates the B+ tree implementation by performing insertion, deletion, and querying operations.

**Source Files:**
- `bruteforce.cpp`

**Compilation Command:**
```bash
g++ -O3 -std=c++14 -o hnsieve_test bruteforce.cpp
```


**Run the Test Program:**

After compiling the test program, you can run it with the following command:

```bash
./hnsieve_test
```


