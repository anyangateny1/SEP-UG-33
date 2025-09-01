#ifndef BLOCK_GROWTH_H
#define BLOCK_GROWTH_H

#include <string>
#include <unordered_map>
#include "block.h"

// BlockGrowth encapsulates the "fit & grow" compression logic for a parent block
// over a sub-volume (flattened model_slices). The tag_table maps single-char tags to labels.
class BlockGrowth {
public:
    // New constructor: flattened model, dimensions, tag table
    BlockGrowth(const std::string& model_flat, int width, int height, int depth,
                const std::unordered_map<char, std::string>& tag_table);

    // Run the compression/growth algorithm on the given parent block.
    // Prints each fitted/grown block using Block::print_block(label).
    void run(Block parent_block);

private:
    // Inputs
    const std::string& model; // flattened: depth*height*width
    int width, height, depth;  // dimensions of this sub-volume
    const std::unordered_map<char, std::string>& tag_table;

    // State for current run()
    Block parent_block{0,0,0,0,0,0,'\0'};
    int parent_x_end = 0, parent_y_end = 0, parent_z_end = 0;

    // Tracks which cells in 'model' have been compressed (claimed)
    std::string compressed; // same size as model, '0' = uncompressed, '1' = compressed

    // Helper functions
    bool all_compressed() const;
    char get_mode_of_uncompressed(const Block& blk) const;

    Block fit_block(char mode, int width, int height, int depth);
    void grow_block(Block& current, Block& best_block);

    bool window_is_all(char val,
                       int z0, int z1, int y0, int y1, int x0, int x1) const;
    bool window_is_all_uncompressed(int z0, int z1, int y0, int y1, int x0, int x1) const;
    void mark_compressed(int z0, int z1, int y0, int y1, int x0, int x1, bool v);

    // Flattened index helper
    inline int idx(int z, int y, int x) const { return z * height * width + y * width + x; }
};

#endif // BLOCK_GROWTH_H
