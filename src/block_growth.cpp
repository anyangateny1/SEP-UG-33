#include "block_growth.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <queue>
#include <stdexcept>
#include <algorithm>

using namespace std;

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

    priority_queue<Block> uniform;
    search_uniform_blocks(uniform);

    while (!uniform.empty()) {
        char mode = get_mode_of_uncompressed(parent_block);
        Block b = uniform.top();
        if (!compressed.at(b.z_offset, b.y_offset, b.x_offset)) {
            if (window_is_all_uncompressed(b.z_offset, b.z_offset+b.depth, b.y_offset, b.y_offset+b.height, b.x_offset, b.x_offset+b.width)) {
                grow_block(b, b);
                mark_compressed(b.z_offset, b.z_offset+b.depth, b.y_offset, b.y_offset+b.height, b.x_offset, b.x_offset+b.width, 1);

                auto it = tag_table.find(b.tag);
                const string& label = (it == tag_table.end()) ? string(1, b.tag) : it->second;
                b.print_block(label);

                uniform.pop();
            } else {
                b.set_width(b.width - 1);
                b.set_height(b.height - 1);
                b.set_depth(b.depth - 1);
                uniform.pop();
                uniform.push(b);
            }
        } else {
            uniform.pop();
        }
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

void BlockGrowth::search_uniform_blocks(priority_queue<Block>& uniform) {
    int max_cube = min({parent_block.width, parent_block.height, parent_block.depth});
    for (int z = parent_block.z; z < parent_block.z_end; ++z) {
        int z_off = z - parent_block.z;
        for (int y = parent_block.y; y < parent_block.y_end; ++y) {
            int y_off = y - parent_block.y;
            for (int x = parent_block.x; x < parent_block.x_end; ++x) {
                int x_off = x - parent_block.x;

                for (int cube_size = max_cube; cube_size >= 1; cube_size--) {
                    if ((z + cube_size > parent_block.z_end) ||
                        (y + cube_size > parent_block.y_end) ||
                        (x + cube_size > parent_block.x_end))
                            continue;

                    char tag = model.at(z_off, y_off, x_off);
                    if (window_is_all(tag, z_off, z_off+cube_size, y_off, y_off+cube_size, x_off, x_off+cube_size)) {
                        uniform.push(Block(x, y, z, cube_size, cube_size, cube_size, tag, x_off, y_off, z_off));
                        break;
                    }
                }
            }
        }
    }
}

bool BlockGrowth::window_is_all(char val, int z0, int z1, int y0, int y1, int x0, int x1) const {
    for (int z = z0; z < z1; ++z)
        for (int y = y0; y < y1; ++y)
            for (int x = x0; x < x1; ++x)
                if (model.at(z, y, x) != val) return false;
    return true;
}

bool BlockGrowth::window_is_all_uncompressed(int z0, int z1, int y0, int y1, int x0, int x1) const {
    for (int z = z0; z < z1; ++z)
        for (int y = y0; y < y1; ++y)
            for (int x = x0; x < x1; ++x)
                if (compressed.at(z, y, x) != 0) return false;
    return true;
}

void BlockGrowth::mark_compressed(int z0, int z1, int y0, int y1, int x0, int x1, char v) {
    for (int z = z0; z < z1; ++z)
        for (int y = y0; y < y1; ++y)
            for (int x = x0; x < x1; ++x)
                compressed.at(z, y, x) = v;
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
        for (int zz = z; zz < z_end && ok; ++zz) {
            for (int yy = y; yy < y_end && ok; ++yy) {
                if (model.at(zz, yy, x_end) != b.tag || compressed.at(zz, yy, x_end) != 0)
                    ok = false;
            }
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
        for (int zz = z; zz < z_end && ok; ++zz) {
            for (int xx = x; xx < x_end && ok; ++xx) {
                if (model.at(zz, y_end, xx) != b.tag || compressed.at(zz, y_end, xx) != 0)
                    ok = false;
            }
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
        for (int yy = y; yy < y_end && ok; ++yy) {
            for (int xx = x; xx < x_end && ok; ++xx) {
                if (model.at(z_end, yy, xx) != b.tag || compressed.at(z_end, yy, xx) != 0)
                    ok = false;
            }
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
