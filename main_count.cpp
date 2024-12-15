#include "BpTree.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <array>
#include <string>
#include <chrono>
#include <cassert>
#include <thread>
#include <mutex>


int main(int argc, char* argv[]) {
    using KeyType = float; 
    using VectorType = std::array<float, Dimension>; // [0]: id, [1-100]: d1-d100, [101]: c_value

    std::ios::sync_with_stdio(false);
    std::cin.tie(NULL);

    if (argc < 4) {
        std::cerr << "Usage: " << argv[0] 
                  << " <input.txt> <queries.txt> <output.txt>\n";
        return 1;
    }

    std::string input_file = argv[1];
    std::string queries_file = argv[2];
    std::string output_file = argv[3];

    BpTree<KeyType, 64> bptree;
    std::cout << "Initialized B+ tree.\n";

    std::cout << "Reading input vectors from '" << input_file << "'...\n";
    std::ifstream infile(input_file);
    if (!infile.is_open()) {
        std::cerr << "Error: Unable to open '" << input_file << "' for reading.\n";
        return 1;
    }

    std::string line;
    int line_num = 0;
    std::vector<VectorType> all_vectors;
    all_vectors.reserve(1000000); // Reserve space based on expected number of vectors

    auto read_start = std::chrono::high_resolution_clock::now();

    while (std::getline(infile, line)) {
        if (line.empty()) continue; 

        std::istringstream iss(line);
        VectorType vector;
        int id;
        iss >> id;
        if (iss.fail()) {
            std::cerr << "Error: Unable to read id at line " << line_num + 1 << " in '" << input_file << "'.\n";
            return 1;
        }
        vector[0] = static_cast<float>(id);
        for (int i = 1; i < Dimension; ++i) {
            float val;
            if (!(iss >> val)) {
                std::cerr << "Error: Incomplete data at line " << line_num + 1 << " in '" << input_file << "'.\n";
                return 1;
            }
            vector[i] = val;
        }
        all_vectors.emplace_back(vector);
        line_num++;
        if (line_num % 100000 == 0) {
            std::cout << "Read " << line_num << " vectors.\n";
        }
    }

    infile.close();
    auto read_end = std::chrono::high_resolution_clock::now();
    auto read_duration = std::chrono::duration_cast<std::chrono::seconds>(read_end - read_start).count();
    std::cout << "Read " << line_num << " vectors in " << read_duration << " seconds.\n";

    // Step 2: Insert all vectors into the B+ tree
    std::cout << "Inserting vectors into the B+ tree...\n";
    auto insert_start = std::chrono::high_resolution_clock::now();

    for (const auto& vector : all_vectors) {
        float c_value = vector[Dimension - 1];
        bptree.Insert(c_value, vector);
    }

    auto insert_end = std::chrono::high_resolution_clock::now();
    auto insert_duration = std::chrono::duration_cast<std::chrono::seconds>(insert_end - insert_start).count();
    std::cout << "Inserted " << line_num << " vectors in " << insert_duration << " seconds.\n";
    all_vectors.clear();

    std::cout << "Reading queries from '" << queries_file << "'...\n";
    std::ifstream qfile(queries_file);
    if (!qfile.is_open()) {
        std::cerr << "Error: Unable to open '" << queries_file << "' for reading.\n";
        return 1;
    }

    std::vector<std::pair<float, float>> queries;
    queries.reserve(1000000);

    while (std::getline(qfile, line)) {
        if (line.empty()) continue; // Skip empty lines
        std::istringstream iss(line);
        int q_id;
        iss >> q_id;
        if (iss.fail()) {
            std::cerr << "Error: Unable to read query id at line " << queries.size() + 1 << " in '" << queries_file << "'.\n";
            return 1;
        }
        for (int i = 0; i < 100; ++i) {
            float q_val;
            if (!(iss >> q_val)) {
                std::cerr << "Error: Incomplete query dimensions at line " << queries.size() + 1 << " in '" << queries_file << "'.\n";
                return 1;
            }
        }
        // Read c_min and c_max
        float c_min, c_max;
        if (!(iss >> c_min >> c_max)) {
            std::cerr << "Error: Missing c_min or c_max at line " << queries.size() + 1 << " in '" << queries_file << "'.\n";
            return 1;
        }
        queries.emplace_back(c_min, c_max);
    }
    qfile.close();
    std::cout << "Read " << queries.size() << " queries.\n";
    std::cout << "Performing range queries...\n";
    std::vector<int> results(queries.size(), 0);
    //std::mutex results_mutex;
    unsigned int num_threads = std::thread::hardware_concurrency();
    if (num_threads == 0) num_threads = 4; 
    std::cout << "Using " << num_threads << " threads for processing queries.\n";
    auto process_queries = [&](int start_idx, int end_idx) {
        for (int i = start_idx; i < end_idx; ++i) {
            const auto& [c_min, c_max] = queries[i];
            int count = bptree.FindRange(c_min, c_max);
            results[i] = count;
        }
    };

    std::vector<std::thread> threads;
    int queries_per_thread = queries.size() / num_threads;
    int remaining_queries = queries.size() % num_threads;

    int start_idx = 0;
    for (unsigned int i = 0; i < num_threads; ++i) {
        int end_idx = start_idx + queries_per_thread;
        if (i == num_threads - 1) {
            end_idx += remaining_queries; // Add remaining queries to the last thread
        }
        threads.emplace_back(process_queries, start_idx, end_idx);
        start_idx = end_idx;
    }

    for (auto& th : threads) {
        th.join();
    }

    std::cout << "Writing results to '" << output_file << "'...\n";
    std::ofstream outfile(output_file);
    if (!outfile.is_open()) {
        std::cerr << "Error: Unable to open '" << output_file << "' for writing.\n";
        return 1;
    }

    for (const auto& count : results) {
        outfile << count << "\n";
    }

    outfile.close();
    std::cout << "Processed " << queries.size() << " queries successfully.\n";
    std::cout << "Output written to '" << output_file << "'.\n";
    std::cout << "Program completed successfully.\n";

    return 0;
}
