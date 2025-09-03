#ifndef BLOCK_MODEL_H
#define BLOCK_MODEL_H

#include <string>
#include <unordered_map>
#include <vector>
#include "block.h"
#include "block_growth.h"

// BlockModel reads the spec, tag table, and 3D model from stdin,
// batches slices by parent block thickness, and invokes BlockGrowth.
class BlockModel {
public:
    void read_specification(); // reads: x_count, y_count, z_count, parent_x, parent_y, parent_z
    void read_tag_table();     // reads "tag, label" lines until an empty line
    void read_model();         // reads z_count slices, each: y_count rows of x_count chars (then blank line)

private:
    int x_count = 0, y_count = 0, z_count = 0;
    int parent_x = 0, parent_y = 0, parent_z = 0;

    // Flattened ring buffer: size = parent_z * y_count * x_count
    std::string model;

    // Single-char tag -> label
    std::unordered_map<char, std::string> tag_table;

    // Helper functions
    static bool is_empty_line(const std::string& s);
    static void getline_strict(std::string& out);
    static std::vector<int> split_csv_ints(const std::string& line);

    // Extracts a sub-volume slice and flattens it
    static std::string slice_model(const std::string& src,
                                   int width, int height, int depth,
                                   int z0, int z1, int y0, int y1, int x0, int x1);

    void compress_slices(int top_slice, int n_slices);
};

#endif // BLOCK_MODEL_H
