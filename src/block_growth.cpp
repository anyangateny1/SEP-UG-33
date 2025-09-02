#include "block_growth.h"
#include <stdexcept>
#include <algorithm>

using std::string;
using std::unordered_map;

// Constructor: flattened model, dimensions, tag table
BlockGrowth::BlockGrowth(const string& model_flat, int w, int h, int d,
                         const unordered_map<char, string>& tags)
    : model(model_flat), width(w), height(h), depth(d), tag_table(tags) {
    compressed.assign(model.size(), '0'); // '0' = uncompressed
}

// Flattened index helper
inline int BlockGrowth::idx(int z, int y, int x) const {
    return z * (height * width) + y * width + x;
}

// Run the compression/growth algorithm on the given parent block
void BlockGrowth::run(Block parent) {
    parent_block = parent;
    parent_x_end = parent_block.x + parent_block.width;
    parent_y_end = parent_block.y + parent_block.height;
    parent_z_end = parent_block.z + parent_block.depth;

    while (!all_compressed()) {
        char mode = get_mode_of_uncompressed(parent_block);
        if (mode == '\0') break;

        Block fitted = fit_block(mode, 1, 1, 1);
        Block best = fitted;
        grow_block(fitted, best);

        // Lookup label and print
        auto it = tag_table.find(best.tag);
        if (it != tag_table.end()) {
            best.print_block(it->second);
        }

        mark_compressed(best.z, best.z + best.depth,
                        best.y, best.y + best.height,
                        best.x, best.x + best.width, true);
    }
}

// Checks if entire parent block region is compressed
bool BlockGrowth::all_compressed() const {
    for (char c : compressed)
        if (c == '0') return false;
    return true;
}


// Find mode of uncompressed cells in block slice
char BlockGrowth::get_mode_of_uncompressed(const Block& blk) const {
    int z0 = blk.z_offset, z1 = blk.z_offset + blk.depth;
    int y0 = blk.y_offset, y1 = blk.y_offset + blk.height;
    int x0 = blk.x_offset, x1 = blk.x_offset + blk.width;

    // Frequency map over char tags
    int freq[256] = {0};
    for (int z = z0; z < z1; ++z)
        for (int y = y0; y < y1; ++y)
            for (int x = x0; x < x1; ++x)
                if (compressed[idx(z,y,x)] == '0') {
                    unsigned char uc = static_cast<unsigned char>(model[idx(z,y,x)]);
                    ++freq[uc];
                }

    char best = 0;
    int bestCount = -1;
    for (int i = 0; i < 256; ++i) {
        if (freq[i] > bestCount) {
            bestCount = freq[i];
            best = static_cast<char>(i);
        }
    }
    return best;
}

// Scan through parent block region to find the first fitting window
Block BlockGrowth::fit_block(char mode, int width, int height, int depth) {
    for (int z = parent_block.z; z < parent_block.z_end; ++z) {
        int z_off = z - parent_block.z;
        int z_end = z_off + depth;
        if (z_end > parent_z_end) break;

        for (int y = parent_block.y; y < parent_block.y_end; ++y) {
            int y_off = y - parent_block.y;
            int y_end = y_off + height;
            if (y_end > parent_y_end) break;

            for (int x = parent_block.x; x < parent_block.x_end; ++x) {
                int x_off = x - parent_block.x;
                int x_end = x_off + width;
                if (x_end > parent_x_end) break;

                if (window_is_all(mode, z_off, z_end, y_off, y_end, x_off, x_end) &&
                    window_is_all_uncompressed(z_off, z_end, y_off, y_end, x_off, x_end)) {

                    Block b(x, y, z, width, height, depth, mode, x_off, y_off, z_off);
                    grow_block(b, b);
                    mark_compressed(z_off, z_off+b.depth, y_off, y_off+b.height, x_off, x_off+b.width, true);
                    return b;
                }
            }
        }
    }

    // Recursively shrink to try smaller cubes (replicates Python behavior)
    if (width <= 1 || height <= 1 || depth <= 1) {
        throw std::runtime_error("No fitting block found at minimal size.");
    }
    return fit_block(mode, width - 1, height - 1, depth - 1);
}

// Check if all cells in a window match the given tag
bool BlockGrowth::window_is_all(char val,
                                int z0, int z1, int y0, int y1, int x0, int x1) const {
    for (int z = z0; z < z1; ++z)
        for (int y = y0; y < y1; ++y)
            for (int x = x0; x < x1; ++x)
                if (model[idx(z,y,x)] != val) return false;
    return true;
}

// Check if all cells in a window are uncompressed
bool BlockGrowth::window_is_all_uncompressed(int z0, int z1, int y0, int y1, int x0, int x1) const {
    for (int z = z0; z < z1; ++z)
        for (int y = y0; y < y1; ++y)
            for (int x = x0; x < x1; ++x)
                if (compressed[idx(z,y,x)] == '1') return false;
    return true;
}

// Mark a window as compressed/uncompressed
void BlockGrowth::mark_compressed(int z0, int z1, int y0, int y1, int x0, int x1, bool v) {
    char mark = v ? '1' : '0';
    for (int z = z0; z < z1; ++z)
        for (int y = y0; y < y1; ++y)
            for (int x = x0; x < x1; ++x)
                compressed[idx(z,y,x)] = mark;
}

// Recursive growth in +X, +Y, +Z directions
void BlockGrowth::grow_block(Block& current, Block& best_block) {
    Block b = current;
    int x = b.x_offset, y = b.y_offset, z = b.z_offset;
    int x_end = x + b.width, y_end = y + b.height, z_end = z + b.depth;

    // Try +X growth
    if (x_end < parent_x_end) {
        bool ok = true;
        for (int zz = z; zz < z_end && ok; ++zz)
            for (int yy = y; yy < y_end && ok; ++yy) {
                if (model[idx(zz,yy,x_end)] != b.tag) ok = false;
                if (compressed[idx(zz,yy,x_end)] == '1') ok = false;
            }
        if (ok) {
            b.set_width(b.width + 1);
            current = b;
            grow_block(current, best_block);
            if (current.volume > best_block.volume) best_block = current;
            b.set_width(b.width - 1);
        }
    }

    // Try +Y growth
    if (y_end < parent_y_end) {
        bool ok = true;
        for (int zz = z; zz < z_end && ok; ++zz)
            for (int xx = x; xx < x_end && ok; ++xx) {
                if (model[idx(zz,y_end,xx)] != b.tag) ok = false;
                if (compressed[idx(zz,y_end,xx)] == '1') ok = false;
            }
        if (ok) {
            b.set_height(b.height + 1);
            current = b;
            grow_block(current, best_block);
            if (current.volume > best_block.volume) best_block = current;
            b.set_height(b.height - 1);
        }
    }

    // Try +Z growth
    if (z_end < parent_z_end) {
        bool ok = true;
        for (int yy = y; yy < y_end && ok; ++yy)
            for (int xx = x; xx < x_end && ok; ++xx) {
                if (model[idx(z_end,yy,xx)] != b.tag) ok = false;
                if (compressed[idx(z_end,yy,xx)] == '1') ok = false;
            }
        if (ok) {
            b.set_depth(b.depth + 1);
            current = b;
            grow_block(current, best_block);
            if (current.volume > best_block.volume) best_block = current;
            b.set_depth(b.depth - 1);
        }
    }

    if (b.volume > best_block.volume) best_block = b;
}