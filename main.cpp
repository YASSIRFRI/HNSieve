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

const int CANDIDATE_THRESHOLD = 100000;
const int QUERY_DIMENSION = 104; 

struct Query {
    hnswlib::labeltype query_id;
    std::array<float, 100> query_d;
    float c_min;
    float c_max;
    float category;
};

using namespace std;

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

    bool data_read_success = ReadBin(input_data_file, Dimension, raw_data);
    uint32_t N = raw_data.size();
    if (!data_read_success) {
        std::cerr << "Error: Failed to read input data file. Exiting.\n";
        return 1;
    }
    std::cout << "Read " << raw_data.size() << " vectors from " << input_data_file << std::endl;
    int k = 100;

    BpTree<KeyType, 64> bptree;

    size_t max_elements = 1000000;
    size_t M = 10;
    size_t ef_construction = 100;
    size_t random_seed = 100;
    hnswlib::L2Space space(100);
    hnswlib::HierarchicalNSW<float> hnsw(&space, max_elements, M, ef_construction, random_seed, false);

    std::string line;
    int line_num = 0;
    auto start_total = std::chrono::steady_clock::now();

    auto start_insert = std::chrono::steady_clock::now();
    auto start_bptree_insert = std::chrono::steady_clock::now();
    auto end_bptree_insert = start_bptree_insert;
    auto start_hnsw_insert = std::chrono::steady_clock::now();
    auto end_hnsw_insert = start_hnsw_insert;
    float id_f=0;
    for(int i = 0; i < N; ++i) {
        VectorType vector;
        for(int j=0;j<Dimension;j++) {
            vector[j] = raw_data[i][j];
        }
        vector[0]=id_f;
        float c_value = vector[1];
        hnswlib::labeltype id = static_cast<hnswlib::labeltype>(id_f);
        auto bptree_insert_start = std::chrono::steady_clock::now();
        bptree.Insert(c_value, vector);
        auto bptree_insert_end_local = std::chrono::steady_clock::now();

        float data_point[Dimension-2];
        for(int ii=2; ii < Dimension; ++ii) { 
            data_point[ii-2] = vector[ii];
        }

        auto hnsw_insert_start_local = std::chrono::steady_clock::now();
        try {
            hnsw.addPoint(data_point, id, false);
        }
        catch(const std::exception& e) {
            std::cerr << "Error adding point with ID " << id << ": " << e.what() << "\n";
            continue;
        }
        auto hnsw_insert_end_local = std::chrono::steady_clock::now();

        id_f++;
        line_num++;
        if(line_num % 100000 == 0) {
            std::cout << "Inserted " << line_num << " vectors.\n";
        }
    }

    auto end_insert = std::chrono::steady_clock::now();
    auto duration_insert_total = std::chrono::duration_cast<std::chrono::milliseconds>(end_insert - start_insert).count();
    auto duration_bptree_insert = std::chrono::duration_cast<std::chrono::milliseconds>(end_bptree_insert - start_bptree_insert).count();
    auto duration_hnsw_insert = std::chrono::duration_cast<std::chrono::milliseconds>(end_hnsw_insert - start_hnsw_insert).count();

    std::cout << "Finished inserting " << line_num << " vectors.\n";
    std::cout << "Total Insertion Time: " << duration_insert_total / 1000.0 << " seconds.\n";
    std::cout << " - B+ Tree Insertion Time: " << duration_bptree_insert / 1000.0 << " seconds.\n";
    std::cout << " - HNSW Insertion Time: " << duration_hnsw_insert / 1000.0 << " seconds.\n";

    std::vector<Query> queries;
    queries.reserve(100000);
    int queries_type2 = 0;

    for (int i = 0; i < Q; ++i) {
        Query q;
        q.query_id = raw_queries[i][0];
        if(q.query_id == 2) {
            queries_type2++;
            q.category = raw_queries[i][1];
            q.c_min = raw_queries[i][2];
            q.c_max = raw_queries[i][3];
            for (int j = 4; j < 104; ++j) {
                q.query_d[j-4] = raw_queries[i][j];
            }
            queries.push_back(q);
        }
    }
    cout<<"Number of type 2 queries: "<<queries_type2<<endl;
    std::cout << queries[0].query_id << " ";
    std::cout << queries[0].c_min << " ";
    std::cout << queries[0].c_max << " ";
    std::cout << queries[0].category << " ";
    for(int i = 0; i < 100; ++i) {
        std::cout << queries[0].query_d[i] << " ";
    }


    std::vector<std::vector<hnswlib::labeltype>> outputs;

    auto start_queries = std::chrono::steady_clock::now();
    auto start_bptree_probe = std::chrono::steady_clock::now();
    auto end_bptree_probe = start_bptree_probe;
    auto start_hnsw_search = std::chrono::steady_clock::now();
    auto end_hnsw_search = start_hnsw_search;

    long long total_bptree_time_ms = 0;
    long long total_hnsw_time_ms = 0;

    for(const auto& q : queries) {
        auto bptree_start = std::chrono::steady_clock::now();
        std::vector<hnswlib::labeltype> candidate_ids;
        bptree.FindRangeIds(q.c_min, q.c_max, candidate_ids);
        /*
        TO DO:
            For some reason extreamly slow! (locks?)
            bptree.FindIdRangeIterative(q.c_min,q.c_max,candidate_ids);
        */



        //cout<<"Candidate IDs: ";
        //for(auto id : candidate_ids) {
            //std::cout << id << " ";
        //}
        std::cout << endl;
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
            topk_candidates.reserve(num_candidates);
            for(auto id : candidate_ids) {
                std::vector<float> vec;
                try {
                    vec = hnsw.getDataByLabel<float>(id);
                }
                catch(const std::exception& e) {
                    std::cerr << "Error retrieving vector for ID " << id << ": " << e.what() << "\n";
                    continue;
                }
                float distance = 0.0f;
                for(int i=0; i < 100; ++i) {
                    float diff = q.query_d[i] - vec[i];
                    distance += diff * diff;
                }
                distance = std::sqrt(distance);
                topk_candidates.emplace_back(distance, id);
            }
            std::partial_sort(topk_candidates.begin(), topk_candidates.begin() + std::min(k, (int)topk_candidates.size()), topk_candidates.end(),
                              [](const std::pair<float, hnswlib::labeltype>& a, const std::pair<float, hnswlib::labeltype>& b) {
                                  return a.first < b.first;
                              });

            std::vector<hnswlib::labeltype> topk_ids;
            topk_ids.reserve(std::min(k, (int)topk_candidates.size()));
            for(int i=0; i < k && i < (int)topk_candidates.size(); ++i) {
                topk_ids.push_back(topk_candidates[i].second);
            }

            while(topk_ids.size() < (size_t)k) {
                topk_ids.push_back(0);
            }

            outputs.emplace_back(topk_ids);
        }
        else {
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
            std::reverse(topk_ids.begin(), topk_ids.end()); 
            while(topk_ids.size() < (size_t)k) {
                topk_ids.push_back(0);
            }
            outputs.emplace_back(topk_ids);
        }
    }

    auto end_queries = std::chrono::steady_clock::now();
    auto duration_queries_total = std::chrono::duration_cast<std::chrono::milliseconds>(end_queries - start_queries).count();

    std::cout << "Processed " << queries.size() << " queries.\n";
    string output_file = "output.txt";
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
