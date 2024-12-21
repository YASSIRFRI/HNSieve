#include "BpTree.h"
#include "hnswlib/hnswlib.h" 
#include <iostream>
#include "io.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <array>

const int CANDIDATE_THRESHOLD = 10;
const int QUERY_DIMENSION = 104; 
const int VECTOR_DIMENSION = 100; // Dimension used in HNSW (excluding categorical attributes)

struct Query {
    hnswlib::labeltype query_id;
    std::array<float, VECTOR_DIMENSION> query_d;
    float c_min;
    float c_max;
    float category;
};

using namespace std;

int compute_prefetch_size(int k, uint32_t N, uint32_t C) {
    if (C == 0) return 0;
    double x = ceil(static_cast<double>(k) * N / C);
    return static_cast<int>(x);
}

// Function to filter vectors based on categorical constraints
bool is_valid(float c_value, float c_min, float c_max, float category) {
    return (c_value >= c_min) && (c_value <= c_max) && (category == category);
}

int main(int argc, char* argv[]) {
    using KeyType = float; 

    std::ios::sync_with_stdio(false);
    std::cin.tie(NULL);

    std::vector<std::vector<float>> raw_data;
    std::vector<std::vector<float>> raw_queries;
    std::string input_data_file = "dummy-data.bin";
    std::string queries_file = "dummy-queries.bin";

    bool queries_read_success = ReadBin(queries_file, QUERY_DIMENSION, raw_queries);
    if (!queries_read_success) {
        std::cerr << "Error: Failed to read queries file. Exiting.\n";
        return 1;
    }
    uint32_t Q = raw_queries.size();
    std::cout << "Loaded " << Q << " queries.\n";
    for(int i = 0; i < 104; ++i) {
        std::cout << raw_queries[0][i] << " ";
    }
    std::cout << std::endl;

    bool data_read_success = ReadBin(input_data_file, VECTOR_DIMENSION + 2, raw_data); // Assuming first two dimensions are categorical
    uint32_t N = raw_data.size();
    if (!data_read_success) {
        std::cerr << "Error: Failed to read input data file. Exiting.\n";
        return 1;
    }
    std::cout << "Read " << raw_data.size() << " vectors from " << input_data_file << std::endl;
    int k = 100;

    BpTree<KeyType, 64> bptree;
    size_t max_elements = 1000000;
    size_t M = 16;
    size_t ef_construction = 100;
    size_t random_seed = 100;
    hnswlib::L2Space space(VECTOR_DIMENSION);
    hnswlib::HierarchicalNSW<float> hnsw(&space, max_elements, M, ef_construction, random_seed, false);

    std::string line;
    int line_num = 0;
    auto start_total = std::chrono::steady_clock::now();

    auto start_insert = std::chrono::steady_clock::now();
    float id_f = 0;
    unordered_map<hnswlib::labeltype,float> mp_id_c_value;
    for(int i = 0; i < N; ++i) {
        VectorType vector;
        for(int j=0;j<Dimension;j++){
                vector[j]= raw_data[i][j];
        }
        vector[0] = id_f;
        float c_value = vector[1];
        hnswlib::labeltype id = static_cast<hnswlib::labeltype>(id_f);
        mp_id_c_value[id]=c_value;

        bptree.Insert(c_value, vector);

        // Prepare data point for HNSW (exclude first two categorical dimensions)
        float data_point[VECTOR_DIMENSION];
        for(int ii = 0; ii < VECTOR_DIMENSION; ++ii) { 
            data_point[ii] = vector[ii + 2];
        }

        // Insert into HNSW
        try {
            hnsw.addPoint(data_point, id, false);
        }
        catch(const std::exception& e) {
            std::cerr << "Error adding point with ID " << id << ": " << e.what() << "\n";
            continue;
        }

        id_f++;
        line_num++;
        if(line_num % 100000 == 0) {
            std::cout << "Inserted " << line_num << " vectors.\n";
        }
    }

    auto end_insert = std::chrono::steady_clock::now();
    auto duration_insert_total = std::chrono::duration_cast<std::chrono::milliseconds>(end_insert - start_insert).count();

    std::cout << "Finished inserting " << line_num << " vectors.\n";
    std::cout << "Total Insertion Time: " << duration_insert_total / 1000.0 << " seconds.\n";

    // Prepare Queries
    std::vector<Query> queries;
    queries.reserve(Q);
    int queries_type2 = 0;

    for (int i = 0; i < Q; ++i) {
        Query q;
        q.query_id = static_cast<hnswlib::labeltype>(raw_queries[i][0]);
        if(q.query_id == 2) { // Assuming query_id == 2 signifies type 2 queries
            queries_type2++;
            q.category = raw_queries[i][1];
            q.c_min = raw_queries[i][2];
            q.c_max = raw_queries[i][3];
            for (int j = 4; j < QUERY_DIMENSION; ++j) {
                q.query_d[j-4] = raw_queries[i][j];
            }
            queries.push_back(q);
        }
    }
    cout << "Number of type 2 queries: " << queries_type2 << endl;
    if(!queries.empty()) {
        std::cout << "Sample Query Details:\n";
        std::cout << "Query ID: " << queries[0].query_id << "\n";
        std::cout << "C_min: " << queries[0].c_min << ", C_max: " << queries[0].c_max << ", Category: " << queries[0].category << "\n";
        std::cout << "Query Vector: ";
        for(int i = 0; i < VECTOR_DIMENSION; ++i) {
            std::cout << queries[0].query_d[i] << " ";
        }
        std::cout << std::endl;
    }

    // Prepare for Query Processing
    std::vector<std::vector<hnswlib::labeltype>> outputs;
    outputs.reserve(queries.size());

    // Performance Metrics
    long long total_bptree_time_ms = 0;
    long long total_hnsw_time_ms = 0;

    // Start Query Processing
    auto start_queries = std::chrono::steady_clock::now();

    for(const auto& q : queries) {
        // Start B+ Tree Range Query
        auto bptree_start = std::chrono::steady_clock::now();
        std::vector<hnswlib::labeltype> candidate_ids;
        bptree.FindRangeIds(q.c_min, q.c_max, candidate_ids);
        auto bptree_end = std::chrono::steady_clock::now();
        long long bptree_duration = std::chrono::duration_cast<std::chrono::milliseconds>(bptree_end - bptree_start).count();
        total_bptree_time_ms += bptree_duration;

        uint32_t C = candidate_ids.size();
        if(C == 0){
            outputs.emplace_back(); 
            continue;
        }
        int prefetch_size = compute_prefetch_size(k, N, C);
        if(prefetch_size > max_elements) {
            prefetch_size = max_elements;
        }
        std::cout<<"Prefetching : "<<prefetch_size<<"\n";
        std::cout<<"Number of candidate : "<<C<<"\n";

        auto hnsw_start = std::chrono::steady_clock::now();
        auto knn = hnsw.searchKnn(q.query_d.data(), prefetch_size, nullptr);
        auto hnsw_end = std::chrono::steady_clock::now();
        long long hnsw_duration = std::chrono::duration_cast<std::chrono::milliseconds>(hnsw_end - hnsw_start).count();
        total_hnsw_time_ms += hnsw_duration;

        std::vector<std::pair<float, hnswlib::labeltype>> hnsw_results;
        while(!knn.empty()) {
            hnsw_results.emplace_back(knn.top());
            knn.pop();
        }
        std::sort(hnsw_results.begin(), hnsw_results.end(),
                  [](const std::pair<float, hnswlib::labeltype>& a, const std::pair<float, hnswlib::labeltype>& b) {
                      return a.first < b.first;
                  });

        std::vector<hnswlib::labeltype> topk_ids;
        topk_ids.reserve(k);
        for(const auto& [distance, id] : hnsw_results) {
            bool valid = (mp_id_c_value[id]<=q.c_max)&&(mp_id_c_value[id]>=q.c_min);
            if(valid) {
                topk_ids.push_back(id);
                if(topk_ids.size() == static_cast<size_t>(k)) break;
            }
        }

        while(topk_ids.size() < static_cast<size_t>(k)) {
            topk_ids.push_back(0);
        }

        outputs.emplace_back(topk_ids);
    }
    auto end_queries = std::chrono::steady_clock::now();
    auto duration_queries_total = std::chrono::duration_cast<std::chrono::milliseconds>(end_queries - start_queries).count();

    string output_file = "output.txt";
    std::ofstream outfile_final(output_file);
    if(!outfile_final.is_open()) {
        std::cerr << "Error opening " << output_file << " for writing.\n";
        return 1;
    }

    for(const auto& topk_ids : outputs) {
        if(topk_ids.empty()) {
            outfile_final << "0\n";
        }
        else {
            for(size_t i = 0; i < topk_ids.size(); ++i) {
                outfile_final << topk_ids[i];
                if(i < topk_ids.size() - 1) {
                    outfile_final << " ";
                }
            }
            outfile_final << "\n";
        }
    }
    outfile_final.close();

    std::cout << "----- Performance Metrics -----\n";
    std::cout << "Data Ingestion:\n";
    std::cout << " - Total Insertion Time: " << duration_insert_total / 1000.0 << " seconds.\n\n";

    std::cout << "Query Processing:\n";
    std::cout << " - Total Query Processing Time: " << duration_queries_total / 1000.0 << " seconds.\n";
    std::cout << "   - B+ Tree Probing Time: " << total_bptree_time_ms / 1000.0 << " seconds.\n";
    std::cout << "   - HNSW Searching Time: " << total_hnsw_time_ms / 1000.0 << " seconds.\n\n";

    std::cout << "Output written to '" << output_file << "'.\n";
    std::cout << "Program completed successfully.\n";

    return 0;
}
