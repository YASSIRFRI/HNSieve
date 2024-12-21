#include "io.h" // Ensure this header contains the ReadBin function
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <array>
#include <cmath>
#include <algorithm>
#include <chrono>
#include <unordered_set>
#include <unordered_map>
#include "hnswlib/hnswlib.h"

const int DATA_DIMENSION = 102; // Total elements in data vector
const int VECTOR_DIMENSION = 100; // Dimension used for queries and data vectors
const int QUERY_DIMENSION = 104; // Total elements in query vector
const int k = 100;

struct Query {
    int query_id; // Assuming query_id is an integer
    float category; // From query data, not used in brute force
    float c_min;
    float c_max;
    std::array<float, VECTOR_DIMENSION> query_vector;
};

// Function to compute Euclidean distance between two vectors
float compute_distance(const std::array<float, VECTOR_DIMENSION>& a, const std::vector<float>& b) {
    float distance = 0.0f;
    for(int i = 0; i < VECTOR_DIMENSION; ++i) {
        float diff = a[i] - b[i];
        distance += diff * diff;
    }
    return std::sqrt(distance);
}

int main(int argc, char* argv[]) {
    std::ios::sync_with_stdio(false);
    std::cin.tie(NULL);

    // File paths
    std::string input_data_file = "dummy-data.bin";
    std::string queries_file = "dummy-queries.bin";
    std::string program_output_file = "output.txt";
    std::string ground_truth_file = "ground_truth.txt";

    // Read data vectors
    std::vector<std::vector<float>> raw_data;
    bool data_read_success = ReadBin(input_data_file, DATA_DIMENSION, raw_data);
    if (!data_read_success) {
        std::cerr << "Error: Failed to read input data file. Exiting.\n";
        return 1;
    }
    uint32_t N = raw_data.size();
    std::cout << "Read " << N << " data vectors from " << input_data_file << std::endl;

    // Read queries
    std::vector<std::vector<float>> raw_queries;
    bool queries_read_success = ReadBin(queries_file, QUERY_DIMENSION, raw_queries);
    if (!queries_read_success) {
        std::cerr << "Error: Failed to read queries file. Exiting.\n";
        return 1;
    }
    uint32_t Q = raw_queries.size();
    std::cout << "Loaded " << Q << " queries from " << queries_file << std::endl;

    // Filter type 2 queries
    std::vector<Query> type2_queries;
    type2_queries.reserve(Q); // Reserve maximum possible to avoid reallocations
    for (const auto& q_raw : raw_queries) {
        Query q;
        q.query_id = static_cast<int>(q_raw[0]);
        if(q.query_id == 2) { // Assuming query_id == 2 indicates type 2
            q.category = q_raw[1];
            q.c_min = q_raw[2];
            q.c_max = q_raw[3];
            for (int j = 4; j < QUERY_DIMENSION; ++j) {
                q.query_vector[j-4] = q_raw[j];
            }
            type2_queries.push_back(q);
        }
    }
    uint32_t num_type2 = type2_queries.size();
    std::cout << "Number of type 2 queries: " << num_type2 << std::endl;

    // Read program output
    std::ifstream infile_output(program_output_file);
    if(!infile_output.is_open()) {
        std::cerr << "Error opening " << program_output_file << " for reading.\n";
        return 1;
    }

    std::vector<std::vector<int>> program_outputs;
    std::string line;
    while(std::getline(infile_output, line)) {
        std::vector<int> ids;
        std::istringstream iss(line);
        int id;
        while(iss >> id) {
            ids.push_back(id);
        }
        program_outputs.push_back(ids);
    }
    infile_output.close();

    if(program_outputs.size() != type2_queries.size()) {
        std::cerr << "Error: Number of program output lines (" << program_outputs.size() 
                  << ") does not match number of type 2 queries (" << type2_queries.size() << ").\n";
        return 1;
    }
    std::cout << "Read " << program_outputs.size() << " program output lines from " << program_output_file << std::endl;

    // Prepare to write ground truth
    std::ofstream outfile_ground_truth(ground_truth_file);
    if(!outfile_ground_truth.is_open()) {
        std::cerr << "Error opening " << ground_truth_file << " for writing.\n";
        return 1;
    }

    // Preprocess data vectors: separate timestamp and vector components
    std::vector<float> data_timestamps(N);
    std::vector<std::vector<float>> data_vectors(N, std::vector<float>(VECTOR_DIMENSION, 0.0f));
    for(uint32_t i = 0; i < N; ++i) {
        data_timestamps[i] = raw_data[i][1]; // Timestamp is at index 1
        for(int j = 0; j < VECTOR_DIMENSION; ++j) {
            data_vectors[i][j] = raw_data[i][j + 2]; // Vector components start at index 2
        }
    }

    // Perform brute force search and compute recall
    double total_recall = 0.0;
    uint32_t processed_queries = 0;

    // Start timing
    auto start_bf = std::chrono::steady_clock::now();

    for(uint32_t i = 0; i < type2_queries.size(); ++i) {
        const Query& q = type2_queries[i];
        const std::array<float, VECTOR_DIMENSION>& query_vector = q.query_vector;

        std::vector<int> valid_ids;
        valid_ids.reserve(N); 
        for(int id = 0; id < static_cast<int>(N); ++id) {
            float timestamp = data_timestamps[id];
            if(timestamp >= q.c_min && timestamp <= q.c_max) {
                valid_ids.push_back(id);
            }
        }

        if(valid_ids.empty()) {
            outfile_ground_truth << "\n";
            continue;
        }

        std::vector<std::pair<float, int>> distances;
        distances.reserve(valid_ids.size());
        for(const auto& id : valid_ids) {
            float dist = compute_distance(query_vector, data_vectors[id]);
            distances.emplace_back(dist, id);
        }

        if(static_cast<int>(distances.size()) > k) {
            std::nth_element(distances.begin(), distances.begin() + k, distances.end(),
                             [](const std::pair<float, int>& a, const std::pair<float, int>& b) {
                                 return a.first < b.first;
                             });
            distances.resize(k);
        }
        // Sort the top-k for consistency (optional)
        std::sort(distances.begin(), distances.end(),
                  [](const std::pair<float, int>& a, const std::pair<float, int>& b) {
                      return a.first < b.first;
                  });

        // Extract ground truth IDs
        std::vector<int> ground_truth_ids;
        ground_truth_ids.reserve(distances.size());
        for(const auto& pair : distances) {
            ground_truth_ids.push_back(pair.second);
        }

        // Write ground truth to file
        if(!ground_truth_ids.empty()) {
            for(size_t j = 0; j < ground_truth_ids.size(); ++j) {
                outfile_ground_truth << ground_truth_ids[j];
                if(j < ground_truth_ids.size() - 1) {
                    outfile_ground_truth << " ";
                }
            }
        }
        outfile_ground_truth << "\n";

        // Retrieve program output for this query
        const std::vector<int>& program_ids = program_outputs[i];

        // Compute intersection
        std::unordered_set<int> gt_set(ground_truth_ids.begin(), ground_truth_ids.end());
        size_t intersection_size = 0;
        for(const auto& pid : program_ids) {
            if(gt_set.find(pid) != gt_set.end()) {
                intersection_size++;
            }
        }

        double recall = (static_cast<double>(intersection_size) / ground_truth_ids.size());
        total_recall += recall;
        processed_queries++;
        if((i+1) % 100 == 0 || i == type2_queries.size()-1) {
            std::cout << "Processed " << (i+1) << " / " << type2_queries.size() 
                      << " queries. Current average recall: " 
                      << (total_recall / processed_queries) << std::endl;
        }
    }

    auto end_bf = std::chrono::steady_clock::now();
    auto duration_bf = std::chrono::duration_cast<std::chrono::milliseconds>(end_bf - start_bf).count();
    outfile_ground_truth.close();

    double average_recall = (processed_queries > 0) ? (total_recall / processed_queries) : 0.0;
    std::cout << "----- Recall Metrics -----\n";
    std::cout << "Total Queries Processed: " << processed_queries << std::endl;
    std::cout << "Total Recall: " << total_recall << std::endl;
    std::cout << "Average Recall: " << average_recall << std::endl;
    std::cout << "Brute Force Search Time: " << duration_bf / 1000.0 << " seconds.\n";
    std::cout << "Ground truth written to '" << ground_truth_file << "'.\n";

    return 0;
}
