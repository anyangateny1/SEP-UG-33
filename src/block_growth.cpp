#include "block_growth.h"
#include <stdexcept>
#include <algorithm>

using std::string;
using std::unordered_map;

BlockGrowth::BlockGrowth(const Flat3D<char>& model_slices,
                         const unordered_map<char, string>& tag_table)
    : model(model_slices), tag_table(tag_table) {}

void BlockGrowth::run(Block parent_block_) {
    parent_block = parent_block_;
    parent_x_end = parent_block.x_offset + parent_block.width;
    parent_y_end = parent_block.y_offset + parent_block.height;
    parent_z_end = parent_block.z_offset + parent_block.depth;

    // Initialise compressed mask to 0 (false)
    compressed = Flat3D<char>(parent_block.depth,
                              parent_block.height,
                              parent_block.width,
                              0);

    while (!all_compressed()) {
        char mode = get_mode_of_uncompressed(parent_block);
        int cube_size = std::min({parent_block.width, parent_block.height, parent_block.depth});
        Block b = fit_block(mode, cube_size, cube_size, cube_size);

        auto it = tag_table.find(b.tag);
        const string& label = (it == tag_table.end()) ? string(1, b.tag) : it->second;
        b.print_block(label);
    }
}

bool BlockGrowth::all_compressed() const {
    for (char v : compressed.data)
        if (v == 0) return false;
    return true;
}

char BlockGrowth::get_mode_of_uncompressed(const Block& blk) const {
    int z0 = blk.z_offset, z1 = blk.z_offset + blk.depth;
    int y0 = blk.y_offset, y1 = blk.y_offset + blk.height;
    int x0 = blk.x_offset, x1 = blk.x_offset + blk.width;

    int freq[256] = {0};
    for (int z = z0; z < z1; ++z)
        for (int y = y0; y < y1; ++y)
            for (int x = x0; x < x1; ++x)
                if (compressed.at(z, y, x) == 0) {
                    unsigned char uc = static_cast<unsigned char>(model.at(z, y, x));
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
                    grow_block(b);
                    mark_compressed(z_off, z_off+b.depth, y_off, y_off+b.height, x_off, x_off+b.width, 1);
                    return b;
                }
            }
        }
    }

    if (width <= 1 || height <= 1 || depth <= 1) {
        throw std::runtime_error("No fitting block found at minimal size.");
    }
    return fit_block(mode, width - 1, height - 1, depth - 1);
}

bool BlockGrowth::window_is_all(char val,
                                int z0, int z1,
                                int y0, int y1,
                                int x0, int x1) const {
    int volume = (z1-z0) * (y1-y0) * (x1-x0);

    // fallback for small blocks (cheap to brute-force)
    if (volume <= 32) {
        for (int z = z0; z < z1; ++z)
            for (int y = y0; y < y1; ++y)
                for (int x = x0; x < x1; ++x)
                    if (model[z][y][x] != val) return false;
        return true;
    }

    int count = query_prefix(prefix_tag, z0, y0, x0, z1, y1, x1);
    return (count == volume);
}

bool BlockGrowth::window_is_all_uncompressed(int z0, int z1,
                                             int y0, int y1,
                                             int x0, int x1) const {
    int volume = (z1-z0) * (y1-y0) * (x1-x0);

    if (volume <= 32) {
        for (int z = z0; z < z1; ++z)
            for (int y = y0; y < y1; ++y)
                for (int x = x0; x < x1; ++x)
                    if (compressed[z][y][x]) return false;
        return true;
    }

    int count = query_prefix(prefix_compressed, z0, y0, x0, z1, y1, x1);
    return (count == 0);
}


void BlockGrowth::mark_compressed(int z0, int z1, int y0, int y1, int x0, int x1, bool v) {
    for (int z = z0; z < z1; ++z)
        for (int y = y0; y < y1; ++y)
            for (int x = x0; x < x1; ++x)
                compressed.at(z, y, x) = v;
}

void BlockGrowth::grow_block(Block& b) {
    bool grown = true;

    while (grown) {
        grown = false;

        // Attempt +X growth
        if (b.x_offset + b.width < parent_x_end) {
            bool ok = true;
            int x_end = b.x_offset + b.width;
            for (int z = b.z_offset; z < b.z_offset + b.depth && ok; ++z)
                for (int y = b.y_offset; y < b.y_offset + b.height && ok; ++y) {
                    if (model.at(z, y, x_end) != b.tag || compressed.at(z, y, x_end) != 0)
                        ok = false;
                }
            if (ok) { b.width++; grown = true; continue; }
        }

        // Attempt +Y growth
        if (b.y_offset + b.height < parent_y_end) {
            bool ok = true;
            int y_end = b.y_offset + b.height;
            for (int z = b.z_offset; z < b.z_offset + b.depth && ok; ++z)
                for (int x = b.x_offset; x < b.x_offset + b.width && ok; ++x) {
                    if (model.at(z, y_end, x) != b.tag || compressed.at(z, y_end, x) != 0)
                        ok = false;
                }
            if (ok) { b.height++; grown = true; continue; }
        }

        // Attempt +Z growth
        if (b.z_offset + b.depth < parent_z_end) {
            bool ok = true;
            int z_end = b.z_offset + b.depth;
            for (int y = b.y_offset; y < b.y_offset + b.height && ok; ++y)
                for (int x = b.x_offset; x < b.x_offset + b.width && ok; ++x) {
                    if (model.at(z_end, y, x) != b.tag || compressed.at(z_end, y, x) != 0)
                        ok = false;
                }
            if (ok) { b.depth++; grown = true; continue; }
        }
    }
}



//All the 3D prefix table stuff
void BlockGrowth::build_prefix_tables(char tag) {
    D = parent_block.depth;
    H = parent_block.height;
    W = parent_block.width;

    int size = (D+1) * (H+1) * (W+1);
    prefix_tag.assign(size, 0);
    prefix_compressed.assign(size, 0);

    // Build prefix sums
    for (int z = 1; z <= D; ++z) {
        for (int y = 1; y <= H; ++y) {
            for (int x = 1; x <= W; ++x) {
                int is_tag = (model[z-1][y-1][x-1] == tag);
                int is_compressed = (compressed[z-1][y-1][x-1] ? 1 : 0);

                prefix_tag[idx(z,y,x)] =
                      is_tag
                    + prefix_tag[idx(z-1,y,x)]
                    + prefix_tag[idx(z,y-1,x)]
                    + prefix_tag[idx(z,y,x-1)]
                    - prefix_tag[idx(z-1,y-1,x)]
                    - prefix_tag[idx(z-1,y,x-1)]
                    - prefix_tag[idx(z,y-1,x-1)]
                    + prefix_tag[idx(z-1,y-1,x-1)];

                prefix_compressed[idx(z,y,x)] =
                      is_compressed
                    + prefix_compressed[idx(z-1,y,x)]
                    + prefix_compressed[idx(z,y-1,x)]
                    + prefix_compressed[idx(z,y,x-1)]
                    - prefix_compressed[idx(z-1,y-1,x)]
                    - prefix_compressed[idx(z-1,y,x-1)]
                    - prefix_compressed[idx(z,y-1,x-1)]
                    + prefix_compressed[idx(z-1,y-1,x-1)];
            }
        }
    }
}

int BlockGrowth::query_prefix(const std::vector<int>& psum,
                              int z0, int y0, int x0,
                              int z1, int y1, int x1) const {
    // inclusive-exclusive, so indices are already offset
    return psum[idx(z1,y1,x1)]
         - psum[idx(z0,y1,x1)]
         - psum[idx(z1,y0,x1)]
         - psum[idx(z1,y1,x0)]
         + psum[idx(z0,y0,x1)]
         + psum[idx(z0,y1,x0)]
         + psum[idx(z1,y0,x0)]
         - psum[idx(z0,y0,x0)];
}