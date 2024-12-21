// main.cpp
#include "BpTree.h"
#include "io.h"
#include <iostream>
#include <vector>
#include <array>
#include <chrono>
#include <cassert>
#include <cstdint>
#include <algorithm>
#include <map>          
#include <queue>       
#include <cmath>      




int main(int argc, char* argv[]) {
    using KeyType = uint32_t; 
    using VectorType = std::array<float, Dimension>;

    if (argc < 4) {
        std::cerr << "Usage: " << argv[0] 
                  << " <input_data_file> <queries_file> <output_knn_file>\n";
        return 1;
    }

    std::string input_data_file = argv[1];
    std::string queries_file = argv[2];
    std::string output_knn_file = argv[3];

    std::vector<std::vector<float>> raw_data;
    std::cout << "Dimension: " << Dimension << std::endl;
    bool data_read_success = ReadBin(input_data_file, Dimension, raw_data);
    if (!data_read_success) {
        std::cerr << "Error: Failed to read input data file. Exiting.\n";
        return 1;
    }
    std::cout << "Read " << raw_data.size() << " vectors from " << input_data_file << std::endl;
    uint32_t N = raw_data.size();
    if (N == 0) {
        std::cerr << "No data to process. Exiting.\n";
        return 1;
    }

    std::cout << "Number of vectors read: " << N << std::endl;

    BpTree<KeyType, 64> bptree;
    std::cout << "Initialized B+ tree." << std::endl;

    auto insert_start = std::chrono::high_resolution_clock::now();
    for (uint32_t i = 0; i < N; ++i) {
        KeyType key = i; 
        VectorType vec;
        assert(raw_data[i].size() == Dimension);
        for (int d = 0; d < Dimension; ++d) {
            vec[d] = raw_data[i][d];
        }
        bptree.Insert(key, vec);
        if ((i + 1) % 100000 == 0 || i == N - 1) { // Log at every 100k insertions and last insertion
            std::cout << "Inserted " << (i + 1) << " / " << N << " vectors." << std::endl;
        }
    }
    auto insert_end = std::chrono::high_resolution_clock::now();
    auto insert_duration = std::chrono::duration_cast<std::chrono::seconds>(insert_end - insert_start).count();
    std::cout << "Insertion completed in " << insert_duration << " seconds." << std::endl;

    // Step 3: Read Queries
    std::vector<std::vector<float>> raw_queries;
    bool queries_read_success = ReadBin(queries_file, Dimension, raw_queries);
    if (!queries_read_success) {
        std::cerr << "Error: Failed to read queries file. Exiting.\n";
        return 1;
    }
    std::cout << "Read " << raw_queries.size() << " queries from " << queries_file << std::endl;
    uint32_t Q = raw_queries.size();
    if (Q == 0) {
        std::cerr << "No queries to process. Exiting.\n";
        return 1;
    }
    std::cout << "Number of queries read: " << Q << std::endl;
    std::cout << "Queries size: " << raw_queries[0].size() << std::endl;

    // Convert queries to VectorType
    std::vector<VectorType> queries(Q, VectorType());
    for (uint32_t i = 0; i < Q; ++i) {
        assert(raw_queries[i].size() == Dimension);
        for (int d = 0; d < Dimension; ++d) {
            queries[i][d] = raw_queries[i][d];
        }
    }

    const int K = 100;
    std::vector<std::vector<uint32_t>> knn_results(Q, std::vector<uint32_t>(K, 0));

    // Step 4: Perform KNN Searches
    auto knn_start = std::chrono::high_resolution_clock::now();
    for (uint32_t i = 0; i < Q; ++i) {
        //std::vector<KeyType> neighbors;
        //bool success = bptree.FindKNN(queries[i], K, neighbors);
        //if (success && neighbors.size() == K) {
            //knn_results[i] = neighbors;
        //} else {
            //std::cerr << "KNN search failed for query " << i << std::endl;
        //}

        //if ((i + 1) % 1000 == 0 || i == Q - 1) { // Log at every 1000 queries and last query
            //std::cout << "Processed " << (i + 1) << " / " << Q << " queries." << std::endl;
        //}
    }
    auto knn_end = std::chrono::high_resolution_clock::now();
    auto knn_duration = std::chrono::duration_cast<std::chrono::seconds>(knn_end - knn_start).count();
    std::cout << "KNN search completed in " << knn_duration << " seconds." << std::endl;

    // Step 5: Save KNN results
    SaveKNN(knn_results, output_knn_file);

    std::cout << "Benchmark completed successfully." << std::endl;

    return 0;
}
