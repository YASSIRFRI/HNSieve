
//single_threaded.cpp


// main.cpp
#include "BpTree.h"
#include "hnswlib/hnswlib.h" 
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <chrono>
#include <cmath>
#include <algorithm>

const int CANDIDATE_THRESHOLD = 1000;

struct Query {
    hnswlib::labeltype query_id;
    std::array<float, 100> query_d;
    float c_min;
    float c_max;
};

// Function to parse a line into a VectorType
bool parse_line(const std::string& line, VectorType& vector) {
    const char* str = line.c_str();
    char* end;
    // Read id
    vector[0] = std::strtof(str, &end);
    if (str == end) return false; // No conversion
    str = end + 1; // Skip space

    // Read 100 dimensions
    for (int i = 1; i < Dimension; ++i) {
        vector[i] = std::strtof(str, &end);
        if (str == end) return false;
        str = end + 1; // Skip space
    }

    return true;
}

int main(int argc, char* argv[]) {
    using KeyType = float; 

    // Optimize I/O
    std::ios::sync_with_stdio(false);
    std::cin.tie(NULL);

    if (argc != 5) {
        std::cerr << "Usage: " << argv[0] 
                  << " <input.txt> <queries.txt> <output.txt> <k>\n";
        return 1;
    }

    std::string input_file = argv[1];
    std::string queries_file = argv[2];
    std::string output_file = argv[3];
    int k = std::stoi(argv[4]);

    if (k <= 0) {
        std::cerr << "Error: k must be a positive integer.\n";
        return 1;
    }

    BpTree<KeyType, 64> bptree;

    size_t max_elements = 1000000;
    size_t M = 10;
    size_t ef_construction = 100;
    size_t random_seed = 100;
    hnswlib::L2Space space(100);
    hnswlib::HierarchicalNSW<float> hnsw(&space, max_elements, M, ef_construction, random_seed, false);

    std::ifstream infile(input_file);
    if(!infile.is_open()) {
        std::cerr << "Error opening " << input_file << "\n";
        return 1;
    }
    std::string line;
    int line_num = 0;
    auto start_total = std::chrono::steady_clock::now();

    auto start_insert = std::chrono::steady_clock::now();
    auto start_bptree_insert = std::chrono::steady_clock::now();
    auto end_bptree_insert = start_bptree_insert;
    auto start_hnsw_insert = std::chrono::steady_clock::now();
    auto end_hnsw_insert = start_hnsw_insert;

    while(std::getline(infile, line)) {
        if(line.empty()) continue;

        VectorType vector;
        if(!parse_line(line, vector)) {
            std::cerr << "Error parsing line " << line_num + 1 << "\n";
            continue;
        }

        // Extract components
        float id_f = vector[0];
        hnswlib::labeltype id = static_cast<hnswlib::labeltype>(id_f);
        float c_value = vector[101];

        // Insert into B+ tree
        auto bptree_insert_start = std::chrono::steady_clock::now();
        bptree.Insert(c_value, vector);
        auto bptree_insert_end_local = std::chrono::steady_clock::now();

        // Insert into HNSW
        float data_point[100];
        for(int i=1; i <= 100; ++i) { 
            data_point[i-1] = vector[i];
        }

        auto hnsw_insert_start_local = std::chrono::steady_clock::now();
        try {
            hnsw.addPoint(data_point, id, false); // replace_deleted = false
        }
        catch(const std::exception& e) {
            std::cerr << "Error adding point with ID " << id << ": " << e.what() << "\n";
            continue;
        }
        auto hnsw_insert_end_local = std::chrono::steady_clock::now();

        // Update aggregate insertion times
        if(line_num == 0) {
            start_bptree_insert = bptree_insert_start;
            end_bptree_insert = bptree_insert_end_local;
            start_hnsw_insert = hnsw_insert_start_local;
            end_hnsw_insert = hnsw_insert_end_local;
        }
        else {
            end_bptree_insert = bptree_insert_end_local;
            end_hnsw_insert = hnsw_insert_end_local;
        }

        line_num++;
        if(line_num % 100000 == 0) {
            std::cout << "Inserted " << line_num << " vectors.\n";
        }
    }

    infile.close();

    // Calculate insertion durations
    auto end_insert = std::chrono::steady_clock::now();
    auto duration_insert_total = std::chrono::duration_cast<std::chrono::milliseconds>(end_insert - start_insert).count();
    auto duration_bptree_insert = std::chrono::duration_cast<std::chrono::milliseconds>(end_bptree_insert - start_bptree_insert).count();
    auto duration_hnsw_insert = std::chrono::duration_cast<std::chrono::milliseconds>(end_hnsw_insert - start_hnsw_insert).count();

    std::cout << "Finished inserting " << line_num << " vectors.\n";
    std::cout << "Total Insertion Time: " << duration_insert_total / 1000.0 << " seconds.\n";
    std::cout << " - B+ Tree Insertion Time: " << duration_bptree_insert / 1000.0 << " seconds.\n";
    std::cout << " - HNSW Insertion Time: " << duration_hnsw_insert / 1000.0 << " seconds.\n";

    // Read all queries into memory
    std::vector<Query> queries;
    queries.reserve(100000); // Adjust based on expected number of queries

    std::ifstream qfile(queries_file);
    if(!qfile.is_open()) {
        std::cerr << "Error opening " << queries_file << "\n";
        return 1;
    }

    while(std::getline(qfile, line)) {
        if(line.empty()) continue;

        std::istringstream iss(line);
        Query q;
        iss >> q.query_id;
        for(int i=0; i < 100; i++) {
            iss >> q.query_d[i];
        }
        iss >> q.c_min >> q.c_max;
        queries.push_back(q);
    }
    qfile.close();

    std::cout << "Loaded " << queries.size() << " queries.\n";

    // Prepare output buffer
    std::vector<std::vector<hnswlib::labeltype>> outputs;
    outputs.reserve(queries.size());

    // Query Processing Timers
    auto start_queries = std::chrono::steady_clock::now();
    auto start_bptree_probe = std::chrono::steady_clock::now();
    auto end_bptree_probe = start_bptree_probe;
    auto start_hnsw_search = std::chrono::steady_clock::now();
    auto end_hnsw_search = start_hnsw_search;

    // Variables to accumulate probing and searching times
    long long total_bptree_time_ms = 0;
    long long total_hnsw_time_ms = 0;

    // Process each query sequentially
    for(const auto& q : queries) {
        auto bptree_start = std::chrono::steady_clock::now();
        std::vector<hnswlib::labeltype> candidate_ids;
        bptree.FindRangeIds(q.c_min, q.c_max, candidate_ids);
        auto bptree_end = std::chrono::steady_clock::now();
        long long bptree_duration = std::chrono::duration_cast<std::chrono::milliseconds>(bptree_end - bptree_start).count();
        total_bptree_time_ms += bptree_duration;

        int num_candidates = candidate_ids.size();
        std::vector<std::pair<float, hnswlib::labeltype>> topk_candidates;

        if(num_candidates == 0) {
            outputs.emplace_back();
            continue;
        }

        if(num_candidates <= CANDIDATE_THRESHOLD) {
            // Brute-force search among candidates
            topk_candidates.reserve(num_candidates);
            for(auto id : candidate_ids) {
                // Retrieve the vector from HNSW
                std::vector<float> vec;
                try {
                    vec = hnsw.getDataByLabel<float>(id);
                }
                catch(const std::exception& e) {
                    std::cerr << "Error retrieving vector for ID " << id << ": " << e.what() << "\n";
                    continue;
                }
                // Compute Euclidean distance
                float distance = 0.0f;
                for(int i=0; i < 100; ++i) {
                    float diff = q.query_d[i] - vec[i];
                    distance += diff * diff;
                }
                distance = std::sqrt(distance);

                // Store the distance and ID
                topk_candidates.emplace_back(distance, id);
            }

            // Sort the candidates by distance
            std::partial_sort(topk_candidates.begin(), topk_candidates.begin() + std::min(k, (int)topk_candidates.size()), topk_candidates.end(),
                              [](const std::pair<float, hnswlib::labeltype>& a, const std::pair<float, hnswlib::labeltype>& b) {
                                  return a.first < b.first;
                              });

            // Select top-k
            std::vector<hnswlib::labeltype> topk_ids;
            topk_ids.reserve(std::min(k, (int)topk_candidates.size()));
            for(int i=0; i < k && i < (int)topk_candidates.size(); ++i) {
                topk_ids.push_back(topk_candidates[i].second);
            }

            // Handle case where fewer than k candidates are found
            while(topk_ids.size() < (size_t)k) {
                topk_ids.push_back(0); // Sentinel value
            }

            outputs.emplace_back(topk_ids);
        }
        else {
            // Use HNSW's KNN search to find the top-k nearest neighbors
            auto hnsw_start = std::chrono::steady_clock::now();
            auto knn = hnsw.searchKnn(q.query_d.data(), k, nullptr);
            auto hnsw_end = std::chrono::steady_clock::now();
            long long hnsw_duration = std::chrono::duration_cast<std::chrono::milliseconds>(hnsw_end - hnsw_start).count();
            total_hnsw_time_ms += hnsw_duration;

            std::vector<hnswlib::labeltype> topk_ids;
            while(!knn.empty()) {
                topk_ids.push_back(knn.top().second);
                knn.pop();
            }
            std::reverse(topk_ids.begin(), topk_ids.end()); // Optional: smallest distance first

            while(topk_ids.size() < (size_t)k) {
                topk_ids.push_back(0);
            }
            outputs.emplace_back(topk_ids);
        }
    }

    // End query processing timers
    auto end_queries = std::chrono::steady_clock::now();
    auto duration_queries_total = std::chrono::duration_cast<std::chrono::milliseconds>(end_queries - start_queries).count();

    std::cout << "Processed " << queries.size() << " queries.\n";

    // Write results to the output file
    std::ofstream outfile_final(output_file);
    if(!outfile_final.is_open()) {
        std::cerr << "Error opening " << output_file << " for writing.\n";
        return 1;
    }

    for(const auto& topk_ids : outputs) {
        if(topk_ids.empty()) {
            outfile_final << "0";
        }
        else {
            for(size_t i = 0; i < topk_ids.size(); ++i) {
                outfile_final << topk_ids[i];
                if(i < topk_ids.size() - 1) {
                    outfile_final << " ";
                }
            }
        }
        outfile_final << "\n";
    }

    outfile_final.close();

    std::cout << "----- Performance Metrics -----\n";
    std::cout << "Data Ingestion:\n";
    std::cout << " - Total Insertion Time: " << duration_insert_total / 1000.0 << " seconds.\n";
    std::cout << "   - B+ Tree Insertion Time: " << duration_bptree_insert / 1000.0 << " seconds.\n";
    std::cout << "   - HNSW Insertion Time: " << duration_hnsw_insert / 1000.0 << " seconds.\n\n";

    std::cout << "Query Processing:\n";
    std::cout << " - Total Query Processing Time: " << duration_queries_total / 1000.0 << " seconds.\n";
    std::cout << "   - B+ Tree Probing Time: " << total_bptree_time_ms / 1000.0 << " seconds.\n";
    std::cout << "   - HNSW Searching Time: " << total_hnsw_time_ms / 1000.0 << " seconds.\n\n";

    std::cout << "Output written to '" << output_file << "'.\n";
    std::cout << "Program completed successfully.\n";

    return 0;
}
