#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include "BpTree.h" // The header from before

int main() {
    std::cout << "Starting the program..." << std::endl;

    std::ifstream infile("input.txt");
    if (!infile) {
        std::cerr << "Error: Failed to open input.txt" << std::endl;
        return 1;
    }
    std::cout << "input.txt opened successfully." << std::endl;

    BpTree<int> tree;
    std::unordered_map<int, std::vector<uint32_t>> key_ids_map;

    int key;
    uint32_t id;
    std::cout << "Reading data from input.txt..." << std::endl;
    while (infile >> key >> id) {
        std::cout << "Read key: " << key << ", id: " << id << std::endl;
        tree.Insert(key, id);
        key_ids_map[key].push_back(id);
    }
    infile.close();
    std::cout << "Finished reading input.txt." << std::endl;

    std::cout << "Sorting IDs in key_ids_map..." << std::endl;
    for (auto &pair : key_ids_map) {
        std::sort(pair.second.begin(), pair.second.end());
    }
    std::cout << "ID sorting completed." << std::endl;

    std::ofstream outfile("output.txt");
    if (!outfile) {
        std::cerr << "Error: Failed to open output.txt for writing" << std::endl;
        return 1;
    }
    std::cout << "output.txt opened successfully." << std::endl;

    std::vector<int> keys;
    keys.reserve(key_ids_map.size());
    for (const auto &p: key_ids_map) {
        keys.push_back(p.first);
    }
    std::sort(keys.begin(), keys.end());
    std::cout << "Keys sorted: ";
    for (auto k : keys) std::cout << k << " ";
    std::cout << std::endl;

    for (auto k : keys) {
        std::vector<uint32_t> result;
        bool found = tree.Search(k, result);
        if (!found) {
            std::cout << "Key " << k << " not found in tree." << std::endl;
            outfile << k << ":\n";
        } else {
            std::cout << "Key " << k << " found with results: ";
            for (auto r : result) std::cout << r << " ";
            std::cout << std::endl;

            std::sort(result.begin(), result.end());
            outfile << k << ":";
            for (size_t i = 0; i < result.size(); i++) {
                outfile << result[i];
                if (i + 1 < result.size()) outfile << ",";
            }
            outfile << "\n";
        }
    }
    std::cout << "Finished writing to output.txt." << std::endl;
    outfile.close();
    std::cout << "Program completed successfully." << std::endl;

    return 0;
}
