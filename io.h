// io.h
#ifndef IO_H
#define IO_H

#include <fstream>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>
#include <cassert>
#include <cstdint>

// Using declarations
using std::cout;
using std::endl;
using std::string;
using std::vector;

/// @brief Save knng in binary format (uint32_t) with name "output.bin"
/// @param knn a (N * 100) shape 2-D vector
/// @param path target save path, the output knng should be named as
/// "output.bin" for evaluation
void SaveKNN(const std::vector<std::vector<uint32_t>> &knns,
             const std::string &path = "output.bin")
{
    std::ofstream ofs(path, std::ios::out | std::ios::binary);
    if (!ofs.is_open()) {
        std::cerr << "Error: Unable to open file " << path << " for writing.\n";
        return;
    }

    const int K = 100;
    const uint32_t N = knns.size();
    if (N == 0) {
        std::cerr << "Warning: No KNN data to save.\n";
        return;
    }
    assert(knns.front().size() == K);
    for (unsigned i = 0; i < N; ++i)
    {
        auto const &knn = knns[i];
        ofs.write(reinterpret_cast<char const *>(&knn[0]), K * sizeof(uint32_t));
        if (!ofs) {
            std::cerr << "Error: Failed to write KNN data for point " << i << ".\n";
            return;
        }
    }
    ofs.close();
    std::cout << "Successfully saved KNN results to " << path << std::endl;
}

/// @brief Reading binary data vectors. Raw data stored as a (N x dim)
/// @param file_path file path of binary data
/// @param num_dimensions number of dimensions per data point
/// @param data returned 2D data vectors
bool ReadBin(const std::string &file_path,
             const int num_dimensions,
             std::vector<std::vector<float>> &data)
{
    std::cout << "Reading Data: " << file_path << std::endl;
    std::ifstream ifs(file_path, std::ios::binary);
    if (!ifs.is_open()) {
        std::cerr << "Error: Unable to open file " << file_path << " for reading.\n";
        return false;
    }

    uint32_t N; // Number of points
    ifs.read(reinterpret_cast<char*>(&N), sizeof(uint32_t));
    if (ifs.gcount() != sizeof(uint32_t)) {
        std::cerr << "Error: Failed to read the number of points from " << file_path << ".\n";
        return false;
    }
    std::cout << "# of points: " << N << std::endl;
    data.resize(N);
    std::vector<float> buff(num_dimensions);
    for (uint32_t counter = 0; counter < N; ++counter) {
        ifs.read(reinterpret_cast<char*>(buff.data()), num_dimensions * sizeof(float));
        if (ifs.gcount() != num_dimensions * sizeof(float)) {
            std::cerr << "Error: Failed to read data point " << counter << " from " << file_path << ".\n";
            data.resize(counter); // Resize to the number of successfully read points
            return false;
        }
        data[counter].assign(buff.begin(), buff.end());
    }

    ifs.close();
    std::cout << "Finish Reading Data" << std::endl;
    return true;
}

#endif // IO_H
