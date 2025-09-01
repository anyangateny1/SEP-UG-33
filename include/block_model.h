#ifndef BLOCK_MODEL_H
#define BLOCK_MODEL_H

#include <string>
#include <unordered_map>
#include "block.h"
#include "block_growth.h"

// BlockModel reads the spec, tag table, and 3D model from stdin,
// batches slices by parent block thickness, and invokes BlockGrowth.
class BlockModel {
public:
    void read_specification(); // reads: x_count, y_count, z_count, parent_x, parent_y, parent_z
    void read_tag_table(); // reads "tag, label" lines until an empty line
    void read_model(); // reads z_count slices, each: y_count rows of x_count chars (then blank line)

private:
    int x_count = 0, y_count = 0, z_count = 0;
    int parent_x = 0, parent_y = 0, parent_z = 0;

    // Ring buffer for slices: [parent_z][y_count][x_count]
    Vec3<char> model;

    // Single-char tag -> label
    std::unordered_map<char, std::string> tag_table;

    // Helper functions
    static bool is_empty_line(const std::string& s);
    static void getline_strict(std::string& out);
    static std::vector<int> split_csv_ints(const std::string& line);

    static Vec3<char> slice_model(const Vec3<char>& src,
                                  int depth, int y0, int y1, int x0, int x1);

    void compress_slices(int top_slice, int n_slices);
};

#endif // BLOCK_MODEL_H