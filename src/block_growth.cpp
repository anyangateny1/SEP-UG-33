#include "block_growth.h"
#include <stdexcept>
#include <algorithm>

using std::string;
using std::unordered_map;

BlockGrowth::BlockGrowth(const std::string& model_flat, int w, int h, int d,
                         const std::unordered_map<char, std::string>& tags)
    : model(model_flat), width(w), height(h), depth(d), tag_table(tags) {
    compressed.assign(model.size(), '0');
}

void BlockGrowth::run(Block parent_block_) {
    parent_block = parent_block_;
    parent_x_end = parent_block.x_offset + parent_block.width;
    parent_y_end = parent_block.y_offset + parent_block.height;
    parent_z_end = parent_block.z_offset + parent_block.depth;

    // Initialise compressed mask to false
    compressed = Vec3<bool>(
        parent_block.depth,
        std::vector<std::vector<bool>>(parent_block.height, std::vector<bool>(parent_block.width, false))
    );

    // Loop until the entire parent block region is compressed
    while (!all_compressed()) {
        char mode = get_mode_of_uncompressed(parent_block);
        int cube_size = std::min({parent_block.width, parent_block.height, parent_block.depth});
        Block b = fit_block(mode, cube_size, cube_size, cube_size);

        // Lookup label and print (exact same output format as Python)
        auto it = tag_table.find(b.tag);
        const string& label = (it == tag_table.end()) ? string(1, b.tag) : it->second;
        b.print_block(label);
    }
}

bool BlockGrowth::all_compressed() const {
    for (const auto& plane : compressed)
        for (const auto& row : plane)
            for (bool v : row)
                if (!v) return false;
    return true;
}

char BlockGrowth::get_mode_of_uncompressed(const Block& blk) const {
    // Equivalent to numpy unique+argmax over uncompressed cells in the block slice
    int z0 = blk.z_offset, z1 = blk.z_offset + blk.depth;
    int y0 = blk.y_offset, y1 = blk.y_offset + blk.height;
    int x0 = blk.x_offset, x1 = blk.x_offset + blk.width;

    // Frequency map over char tags
    int freq[256] = {0}; // tags are char; assuming unsigned char range
    for (int z = z0; z < z1; ++z)
        for (int y = y0; y < y1; ++y)
            for (int x = x0; x < x1; ++x)
                if (!compressed[z][y][x]) {
                    unsigned char uc = static_cast<unsigned char>(model[z][y][x]);
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

Block BlockGrowth::fit_block(char mode, int width, int height, int depth) {
    // Scan through the parent block region to find the first fitting window
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

bool BlockGrowth::window_is_all(char val,
                                int z0, int z1, int y0, int y1, int x0, int x1) const {
    for (int z = z0; z < z1; ++z)
        for (int y = y0; y < y1; ++y)
            for (int x = x0; x < x1; ++x)
                if (model[z][y][x] != val) return false;
    return true;
}

bool BlockGrowth::window_is_all_uncompressed(int z0, int z1, int y0, int y1, int x0, int x1) const {
    for (int z = z0; z < z1; ++z)
        for (int y = y0; y < y1; ++y)
            for (int x = x0; x < x1; ++x)
                if (compressed[z][y][x]) return false;
    return true;
}

void BlockGrowth::mark_compressed(int z0, int z1, int y0, int y1, int x0, int x1, bool v) {
    for (int z = z0; z < z1; ++z)
        for (int y = y0; y < y1; ++y)
            for (int x = x0; x < x1; ++x)
                compressed[z][y][x] = v;
}

void BlockGrowth::grow_block(Block& current, Block& best_block) {
    Block b = current;

    int x = b.x_offset, y = b.y_offset, z = b.z_offset;
    int x_end = x + b.width;
    int y_end = y + b.height;
    int z_end = z + b.depth;

    // Try +X growth
    if (x_end < parent_x_end) {
        bool ok = true;
        for (int zz = z; zz < z_end && ok; ++zz)
            for (int yy = y; yy < y_end && ok; ++yy) {
                if (model[zz][yy][x_end] != b.tag) ok = false;
                if (compressed[zz][yy][x_end]) ok = false;
            }
        if (ok) {
            b.set_width(b.width + 1);
            current = b;
            grow_block(current, best_block);
            if (current.volume > best_block.volume)
                best_block = current;
            b.set_width(b.width - 1);
        }
    }

    // Try +Y growth
    if (y_end < parent_y_end) {
        bool ok = true;
        for (int zz = z; zz < z_end && ok; ++zz)
            for (int xx = x; xx < x_end && ok; ++xx) {
                if (model[zz][y_end][xx] != b.tag) ok = false;
                if (compressed[zz][y_end][xx]) ok = false;
            }
        if (ok) {
            b.set_height(b.height + 1);
            current = b;
            grow_block(current, best_block);
            if (current.volume > best_block.volume)
                best_block = current;
            b.set_height(b.height - 1);
        }
    }

    // Try +Z growth
    if (z_end < parent_z_end) {
        bool ok = true;
        for (int yy = y; yy < y_end && ok; ++yy)
            for (int xx = x; xx < x_end && ok; ++xx) {
                if (model[z_end][yy][xx] != b.tag) ok = false;
                if (compressed[z_end][yy][xx]) ok = false;
            }
        if (ok) {
            b.set_depth(b.depth + 1);
            current = b;
            grow_block(current, best_block);
            if (current.volume > best_block.volume)
                best_block = current;
            b.set_depth(b.depth - 1);
        }
    }

    if (b.volume > best_block.volume)
        best_block = b;
}