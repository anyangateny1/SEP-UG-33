#ifndef BLOCK_GROWTH_H
#define BLOCK_GROWTH_H

#include <vector>
#include <string>
#include <unordered_map>
#include "block.h"

// Flattened 3D container: [depth][height][width]
template<typename T>
class Flat3D {
public:
    int depth, height, width;
    T* data;

    Flat3D() : depth(0), height(0), width(0), data(nullptr) {}

    // Non-owning view over external buffer
    Flat3D(int d, int h, int w, T* ptr)
        : depth(d), height(h), width(w), data(ptr) {}

    inline void reset(int d, int h, int w, T* ptr) {
        depth = d; height = h; width = w; data = ptr;
    }

    inline T& at(int z, int y, int x) {
        return data[(z * height + y) * width + x];
    }

    inline const T& at(int z, int y, int x) const {
        return data[(z * height + y) * width + x];
    }
};

// BlockGrowth encapsulates the "fit & grow" compression logic for a parent block
// over a sub-volume (model_slices). The tag_table maps single-char tags to labels.
class BlockGrowth {
public:
    BlockGrowth(const Flat3D<char>& model_slices,
                const std::unordered_map<char, std::string>& tag_table);

    void run(Block parent_block);

private:
    const Flat3D<char>& model; 
    const std::unordered_map<char, std::string>& tag_table;

    Block parent_block{0,0,0,0,0,0,'\0'};
    int parent_x_end = 0, parent_y_end = 0, parent_z_end = 0;

    // Tracks which cells in 'model' have been compressed (0 = false, 1 = true)
    Flat3D<char> compressed;
    std::vector<char> compressed_storage; // pre-allocated buffer backing 'compressed'

    bool all_compressed() const;
    char get_mode_of_uncompressed(const Block& blk) const;

    Block fit_block(char mode, int width, int height, int depth);
    void grow_block(Block& current, Block& best_block);

    bool window_is_all(char val,
                       int z0, int z1, int y0, int y1, int x0, int x1) const;
    bool window_is_all_uncompressed(int z0, int z1, int y0, int y1, int x0, int x1) const;
    void mark_compressed(int z0, int z1, int y0, int y1, int x0, int x1, char v);
};

#endif // BLOCK_GROWTH_H
