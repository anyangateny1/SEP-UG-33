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
                    grow_block(b, b);
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
                                int z0, int z1, int y0, int y1, int x0, int x1) const {
    // Optimized memory access pattern - check in cache-friendly order
    for (int z = z0; z < z1; ++z) {
        for (int y = y0; y < y1; ++y) {
            // Check consecutive memory locations for better cache performance
            for (int x = x0; x < x1; ++x) {
                if (model.at(z, y, x) != val) return false;
            }
        }
    }
    return true;
}

bool BlockGrowth::window_is_all_uncompressed(int z0, int z1, int y0, int y1, int x0, int x1) const {
    // Optimized loop with better cache access pattern
    for (int z = z0; z < z1; ++z) {
        for (int y = y0; y < y1; ++y) {
            // Direct access for better cache performance
            for (int x = x0; x < x1; ++x) {
                if (compressed.at(z, y, x) != 0) return false;
            }
        }
    }
    return true;
}

void BlockGrowth::mark_compressed(int z0, int z1, int y0, int y1, int x0, int x1, char v) {
    // Optimized loop structure for better cache locality
    for (int z = z0; z < z1; ++z) {
        for (int y = y0; y < y1; ++y) {
            // Direct assignment for better performance
            for (int x = x0; x < x1; ++x) {
                compressed.at(z, y, x) = v;
            }
        }
    }
}

void BlockGrowth::grow_block(Block& current, Block& best_block) {
    Block b = current;

    int x = b.x_offset, y = b.y_offset, z = b.z_offset;
    int x_end = x + b.width;
    int y_end = y + b.height;
    int z_end = z + b.depth;
    
    // Early termination: if current block can't possibly beat best_block
    int max_possible_width = parent_x_end - x;
    int max_possible_height = parent_y_end - y;
    int max_possible_depth = parent_z_end - z;
    int max_possible_volume = max_possible_width * max_possible_height * max_possible_depth;
    
    if (max_possible_volume <= best_block.volume) {
        return; // No point in continuing this branch
    }

    // Try growth in order of potential volume gain (greedy approach)
    // Calculate potential gains for each direction
    int x_gain = (x_end < parent_x_end) ? (b.height * b.depth) : 0;
    int y_gain = (y_end < parent_y_end) ? (b.width * b.depth) : 0;
    int z_gain = (z_end < parent_z_end) ? (b.width * b.height) : 0;
    
    // Sort growth directions by potential gain (largest first)
    struct GrowthOption {
        int gain;
        int direction; // 0=x, 1=y, 2=z
    };
    
    GrowthOption options[3] = {{x_gain, 0}, {y_gain, 1}, {z_gain, 2}};
    std::sort(options, options + 3, [](const GrowthOption& a, const GrowthOption& b) {
        return a.gain > b.gain;
    });
    
    for (int i = 0; i < 3; ++i) {
        if (options[i].gain == 0) break; // No more valid directions
        
        bool ok = true;
        int direction = options[i].direction;
        
        if (direction == 0) { // +X growth
            for (int zz = z; zz < z_end && ok; ++zz)
                for (int yy = y; yy < y_end && ok; ++yy) {
                    if (model.at(zz, yy, x_end) != b.tag) ok = false;
                    if (compressed.at(zz, yy, x_end) != 0) ok = false;
                }
            if (ok) {
                b.set_width(b.width + 1);
                current = b;
                grow_block(current, best_block);
                if (current.volume > best_block.volume)
                    best_block = current;
                b.set_width(b.width - 1);
            }
        } else if (direction == 1) { // +Y growth
            for (int zz = z; zz < z_end && ok; ++zz)
                for (int xx = x; xx < x_end && ok; ++xx) {
                    if (model.at(zz, y_end, xx) != b.tag) ok = false;
                    if (compressed.at(zz, y_end, xx) != 0) ok = false;
                }
            if (ok) {
                b.set_height(b.height + 1);
                current = b;
                grow_block(current, best_block);
                if (current.volume > best_block.volume)
                    best_block = current;
                b.set_height(b.height - 1);
            }
        } else if (direction == 2) { // +Z growth
            for (int yy = y; yy < y_end && ok; ++yy)
                for (int xx = x; xx < x_end && ok; ++xx) {
                    if (model.at(z_end, yy, xx) != b.tag) ok = false;
                    if (compressed.at(z_end, yy, xx) != 0) ok = false;
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
    }

    if (b.volume > best_block.volume)
        best_block = b;
}