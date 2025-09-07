#ifndef BLOCK_GROWTH_H
#define BLOCK_GROWTH_H

#include <vector>
#include <string>
#include <unordered_map>
#include "block.h"
#include <optional>

// 3D alias: [depth][height][width]
template<typename T>
using Vec3 = std::vector<std::vector<std::vector<T>>>;

// BlockGrowth encapsulates the "fit & grow" compression logic for a parent block
// over a sub-volume (model_sl  ices). The tag_table maps single-char tags to labels.
class BlockGrowth {
public:
    BlockGrowth(const Vec3<char>& model_slices,
                const std::unordered_map<char, std::string>& tag_table);

    // Run the compression/growth algorithm on the given parent block.
    // Prints each fitted/grown block using Block::print_block(label).
    void run(Block parent_block);

private:
    // Inputs
    const Vec3<char>& model; // sub-volume: depth x height x width
    const std::unordered_map<char, std::string>& tag_table;

    //Prefix sum tables
    std::vector<int> prefix_tag;
    std::vector<int> prefix_compressed;
    int D, H, W;
    bool prefix_enabled = false; //control hybrid method. Prefix sums only for large blocks
    int candidate_count = 0;

    // State for current run()
    Block parent_block{0,0,0,0,0,0,'\0'};
    int parent_x_end = 0, parent_y_end = 0, parent_z_end = 0;

    // Tracks which cells in 'model' have been compressed (claimed)
    Vec3<bool> compressed; // same shape as 'model'

    // Helper functions
    bool all_compressed() const;
    char get_mode_of_uncompressed(const Block& blk) const;

    Block fit_block(char mode, int width, int height, int depth);
    void grow_block(Block& current, Block& best_block);


    //for the prefix sum tables
    inline int idx(int z, int y, int x) const {
        return (z * (H+1) * (W+1)) + (y * (W+1)) + x;
    }
    void build_prefix_tables(char tag);
    int query_prefix(const std::vector<int>& psum,
                     int z0, int y0, int x0,
                     int z1, int y1, int x1) const;
    void maybe_build_prefix(char tag, int num_queries);

    bool window_is_all(char val,
                       int z0, int z1, int y0, int y1, int x0, int x1) const;
    bool window_is_all_uncompressed(int z0, int z1, int y0, int y1, int x0, int x1) const;
    void mark_compressed(int z0, int z1, int y0, int y1, int x0, int x1, bool v);
};

#endif // BLOCK_GROWTH_H
