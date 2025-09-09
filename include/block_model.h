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

    // Ring buffer for slices: [parent_z][y_count][x_count]
    Flat3D<char> model;
    std::vector<char> model_storage; // backing storage for 'model'

    // Scratch buffer for per-parent sub-volume slices (max size parent_z*parent_y*parent_x)
    Flat3D<char> slice_view;
    std::vector<char> slice_storage; // backing storage for 'slice_view'

    // Single-char tag -> label
    std::unordered_map<char, std::string> tag_table;

    // Helper functions
    static bool is_empty_line(const std::string& s);
    static void getline_strict(std::string& out);
    // CSV parse without dynamic allocations (expects exactly 6 ints)
    static void parse_spec_line_six_ints(const std::string& line,
                                         int& a0, int& a1, int& a2,
                                         int& a3, int& a4, int& a5);

    static void slice_model(const Flat3D<char>& src,
                            Flat3D<char>& dst,
                            int depth, int y0, int y1, int x0, int x1);

    void compress_slices(int top_slice, int n_slices);
};

#endif // BLOCK_MODEL_H
