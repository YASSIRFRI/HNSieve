#include "BpTree.h"
#include <fstream>
#include <sstream>
#include <string>
#include <iomanip>
#include <iostream>
#include <vector>
#include <memory>
#include <chrono>
#include <thread> 

bool read_input(const std::string& filename, std::vector<std::pair<uint64_t, VectorType>>& data) {
    std::ifstream infile(filename);
    if (!infile.is_open()) {
        std::cerr << "Failed to open " << filename << " for reading.\n";
        return false;
    }

    std::string line;
    int line_num = 0;
    while (std::getline(infile, line)) {
        line_num++;
        if (line.empty()) continue;
        std::istringstream iss(line);
        uint64_t key;
        iss >> key;
        VectorType vec;
        for (int i = 0; i < Dimension; ++i) {
            if (!(iss >> vec[i])) {
                std::cerr << "Insufficient vector dimensions for key " << key << " at line " << line_num << ".\n";
                return false;
            }
        }
        data.emplace_back(key, vec);
    }
    infile.close();
    return true;
}

void write_tree(const std::shared_ptr<BpTree<uint64_t, 64>::Node>& node, std::ofstream& outfile) {
    if (node->is_leaf_) {
        auto leaf = std::static_pointer_cast<BpTree<uint64_t, 64>::LeafNode>(node);
        for (int i = 0; i < leaf->count_; ++i) {
            outfile << leaf->keys_[i] << " ";
            for (int j = 0; j < Dimension; ++j) {
                outfile << std::fixed << std::setprecision(6) << leaf->values_[i][j];
                if (j != Dimension - 1) outfile << " ";
            }
            outfile << "\n";
        }
    } else {
        auto internal = std::static_pointer_cast<BpTree<uint64_t, 64>::InternalNode>(node);
        for (int i = 0; i <= internal->count_; ++i) {
            write_tree(internal->children_[i], outfile);
        }
    }
}

int main() {
    typedef uint64_t KeyType;

    // Create a B+ tree instance
    BpTree<KeyType, 64> tree;

    // Read input data
    std::vector<std::pair<uint64_t, VectorType>> data;
    if (!read_input("input.txt", data)) {
        return 1;
    }

    // Insert data into the B+ tree
    for (const auto& entry : data) {
        const uint64_t& key = entry.first;
        const VectorType& vec = entry.second;
        tree.Insert(key, vec);
    }

    // Write tree contents to output.txt for verification
    {
        std::ofstream outfile("output.txt");
        if (!outfile.is_open()) {
            std::cerr << "Failed to open output.txt for writing.\n";
            return 1;
        }
        write_tree(tree.get_root(), outfile);
    }

    std::cout << "Insertion complete. Tree contents written to output.txt.\n";

    // Suppose you have a batch of queries to search
    // (For demonstration, let's say we query the same keys we inserted)
    std::vector<KeyType> queries;
    for (const auto& entry : data) {
        queries.push_back(entry.first);
    }

    // Prepare a vector to store the results of the searches
    std::vector<VectorType> results(queries.size());
    std::vector<bool> found(queries.size(), false);

    // Timing the single-threaded search phase
    auto st_start = std::chrono::high_resolution_clock::now();

    // Perform searches sequentially with detailed logging
    for (size_t i = 0; i < queries.size(); ++i) {
        // Log the start of each search
        //std::cout << "Searching for key " << queries[i] << "...\n";

        // Simulate some workload (optional)
        // std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Optional

        VectorType out_vector;
        bool res = tree.Search(queries[i], out_vector);
        if (res) {
            results[i] = out_vector;
            found[i] = true;
            //std::cout << "Key " << queries[i] << " found.\n";
        } else {
            found[i] = false;
            std::cout << "Key " << queries[i] << " not found.\n";
        }
    }

    auto st_end = std::chrono::high_resolution_clock::now();
    auto st_duration = std::chrono::duration_cast<std::chrono::milliseconds>(st_end - st_start).count();
    std::cout << "Single-threaded search completed in " << st_duration << " ms.\n";

    // Display search results (optional)
    /*
    for (size_t i = 0; i < queries.size(); ++i) {
        if (found[i]) {
            std::cout << "Key: " << queries[i] << " found. First element of vector: " << results[i][0] << "\n";
        } else {
            std::cout << "Key: " << queries[i] << " not found.\n";
        }
    }
    */

    return 0;
}
