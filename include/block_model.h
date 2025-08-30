#ifndef BLOCK_MODEL_H
#define BLOCK_MODEL_H

#include <string>
#include <unordered_map>
#include "block.h"
#include "block_growth.h"
#include "thread_pool_std.h"
#include "output_manager_std.h"

// BlockModel reads the spec, tag table, and 3D model from stdin,
// batches slices by parent block thickness, and invokes BlockGrowth.
class BlockModel {
public:
    BlockModel();
    
    void read_specification(); // reads: x_count, y_count, z_count, parent_x, parent_y, parent_z
    void read_tag_table(); // reads "tag, label" lines until an empty line
    void read_model(); // reads z_count slices, each: y_count rows of x_count chars (then blank line)
    
    // Multi-threading controls
    void enable_multithreading(bool enable = true);
    void set_thread_count(size_t thread_count = 0); // 0 = auto-detect

private:
    int x_count = 0, y_count = 0, z_count = 0;
    int parent_x = 0, parent_y = 0, parent_z = 0;

    // Ring buffer for slices: [parent_z][y_count][x_count]
    Vec3<char> model;

    // Single-char tag -> label
    std::unordered_map<char, std::string> tag_table;
    
    // Threading support using standard library
    bool multithreading_enabled;
    size_t thread_count;
    OutputManagerStd output_manager;

    // Helper functions
    static bool is_empty_line(const std::string& s);
    static void getline_strict(std::string& out);
    static std::vector<int> split_csv_ints(const std::string& line);

    static Vec3<char> slice_model(const Vec3<char>& src,
                                  int depth, int y0, int y1, int x0, int x1);

    void compress_slices(int top_slice, int n_slices);
    void compress_slices_parallel(int top_slice, int n_slices);
    void compress_single_parent_block(int x, int y, int z, int width, int height, int depth, 
                                     size_t order_index, const Vec3<char>& model_snapshot);
};

#endif // BLOCK_MODEL_H
