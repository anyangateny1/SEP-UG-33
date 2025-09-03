#ifndef BLOCK_MODEL_H
#define BLOCK_MODEL_H

#include <string>
#include <unordered_map>
#include <thread>
#include <vector>
#include <future>
#include <mutex>
#include "block.h"
#include "block_growth.h"

// BlockModel reads the spec, tag table, and 3D model from stdin,
// batches slices by parent block thickness, and invokes BlockGrowth.
class BlockModel {
public:
    BlockModel(); // Constructor to initialize threading
    void read_specification(); // reads: x_count, y_count, z_count, parent_x, parent_y, parent_z
    void read_tag_table();     // reads "tag, label" lines until an empty line
    void read_model();         // reads z_count slices, each: y_count rows of x_count chars (then blank line)
    void set_num_threads(unsigned int threads); // Set number of threads to use

private:
    int x_count = 0, y_count = 0, z_count = 0;
    int parent_x = 0, parent_y = 0, parent_z = 0;

    // Ring buffer for slices: [parent_z][y_count][x_count]
    Flat3D<char> model;

    // Single-char tag -> label
    std::unordered_map<char, std::string> tag_table;

    // Threading support
    mutable std::mutex output_mutex;
    unsigned int num_threads;

    // Helper functions
    static bool is_empty_line(const std::string& s);
    static void getline_strict(std::string& out);
    static std::vector<int> split_csv_ints(const std::string& line);

    static Flat3D<char> slice_model(const Flat3D<char>& src,
                                    int depth, int y0, int y1, int x0, int x1);

    void compress_slices(int top_slice, int n_slices);
    void process_parent_block(int x, int y, int z, int width, int height, int depth, char tag, int n_slices);
    std::string process_parent_block_to_string(int x, int y, int z, int width, int height, int depth, char tag, int n_slices);
    std::string process_parent_block_to_string_safe(const Flat3D<char>& model_slices, const Block& parentBlock);
};

#endif // BLOCK_MODEL_H