#include "block_growth.h"
#include <stdexcept>
#include <algorithm>
#include <functional>

using std::string;
using std::unordered_map;

BlockGrowth::BlockGrowth(const Vec3<char>& model_slices,
                         const unordered_map<char, string>& tag_table)
    : model(model_slices), tag_table(tag_table) {}

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
    generation++;

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

// Hash helpers for keys
namespace {
    static inline std::size_t hash_combine(std::size_t seed, std::size_t v) {
        seed ^= v + 0x9e3779b9 + (seed<<6) + (seed>>2);
        return seed;
    }
}

std::size_t BlockGrowth::WindowKeyHash::operator()(const WindowKey& k) const {
    std::size_t h = std::hash<int>{}(k.val);
    h = hash_combine(h, std::hash<int>{}(k.z0));
    h = hash_combine(h, std::hash<int>{}(k.z1));
    h = hash_combine(h, std::hash<int>{}(k.y0));
    h = hash_combine(h, std::hash<int>{}(k.y1));
    h = hash_combine(h, std::hash<int>{}(k.x0));
    h = hash_combine(h, std::hash<int>{}(k.x1));
    h = hash_combine(h, std::hash<uint64_t>{}(k.gen));
    return h;
}

std::size_t BlockGrowth::WindowUncKeyHash::operator()(const WindowUncKey& k) const {
    std::size_t h = 0;
    h = hash_combine(h, std::hash<int>{}(k.z0));
    h = hash_combine(h, std::hash<int>{}(k.z1));
    h = hash_combine(h, std::hash<int>{}(k.y0));
    h = hash_combine(h, std::hash<int>{}(k.y1));
    h = hash_combine(h, std::hash<int>{}(k.x0));
    h = hash_combine(h, std::hash<int>{}(k.x1));
    h = hash_combine(h, std::hash<uint64_t>{}(k.gen));
    return h;
}

std::size_t BlockGrowth::ModeKeyHash::operator()(const ModeKey& k) const {
    std::size_t h = 0;
    h = hash_combine(h, std::hash<int>{}(k.x0));
    h = hash_combine(h, std::hash<int>{}(k.x1));
    h = hash_combine(h, std::hash<int>{}(k.y0));
    h = hash_combine(h, std::hash<int>{}(k.y1));
    h = hash_combine(h, std::hash<int>{}(k.z0));
    h = hash_combine(h, std::hash<int>{}(k.z1));
    h = hash_combine(h, std::hash<uint64_t>{}(k.gen));
    return h;
}

std::size_t BlockGrowth::FitKeyHash::operator()(const FitKey& k) const {
    std::size_t h = std::hash<int>{}(k.mode);
    h = hash_combine(h, std::hash<int>{}(k.w));
    h = hash_combine(h, std::hash<int>{}(k.h));
    h = hash_combine(h, std::hash<int>{}(k.d));
    h = hash_combine(h, std::hash<uint64_t>{}(k.gen));
    return h;
}

std::size_t BlockGrowth::GrowKeyHash::operator()(const GrowKey& k) const {
    std::size_t h = 0;
    h = hash_combine(h, std::hash<int>{}(k.x));
    h = hash_combine(h, std::hash<int>{}(k.y));
    h = hash_combine(h, std::hash<int>{}(k.z));
    h = hash_combine(h, std::hash<int>{}(k.w));
    h = hash_combine(h, std::hash<int>{}(k.h));
    h = hash_combine(h, std::hash<int>{}(k.d));
    h = hash_combine(h, std::hash<int>{}(k.tag));
    h = hash_combine(h, std::hash<uint64_t>{}(k.gen));
    return h;
}

bool BlockGrowth::WindowKeyEq::operator()(const WindowKey& a, const WindowKey& b) const {
    return a.val==b.val && a.z0==b.z0 && a.z1==b.z1 && a.y0==b.y0 && a.y1==b.y1 && a.x0==b.x0 && a.x1==b.x1 && a.gen==b.gen;
}
bool BlockGrowth::WindowUncKeyEq::operator()(const WindowUncKey& a, const WindowUncKey& b) const {
    return a.z0==b.z0 && a.z1==b.z1 && a.y0==b.y0 && a.y1==b.y1 && a.x0==b.x0 && a.x1==b.x1 && a.gen==b.gen;
}
bool BlockGrowth::ModeKeyEq::operator()(const ModeKey& a, const ModeKey& b) const {
    return a.x0==b.x0 && a.x1==b.x1 && a.y0==b.y0 && a.y1==b.y1 && a.z0==b.z0 && a.z1==b.z1 && a.gen==b.gen;
}
bool BlockGrowth::FitKeyEq::operator()(const FitKey& a, const FitKey& b) const {
    return a.mode==b.mode && a.w==b.w && a.h==b.h && a.d==b.d && a.gen==b.gen;
}
bool BlockGrowth::GrowKeyEq::operator()(const GrowKey& a, const GrowKey& b) const {
    return a.x==b.x && a.y==b.y && a.z==b.z && a.w==b.w && a.h==b.h && a.d==b.d && a.tag==b.tag && a.gen==b.gen;
}

char BlockGrowth::get_mode_of_uncompressed(const Block& blk) const {
    // Equivalent to numpy unique+argmax over uncompressed cells in the block slice
    int z0 = blk.z_offset, z1 = blk.z_offset + blk.depth;
    int y0 = blk.y_offset, y1 = blk.y_offset + blk.height;
    int x0 = blk.x_offset, x1 = blk.x_offset + blk.width;

    ModeKey key{x0,x1,y0,y1,z0,z1,generation};
    char cached;
    if (cache_mode.get(key, cached)) return cached;

    int freq[256] = {0};
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
    cache_mode.put(key, best);
    return best;
}

Block BlockGrowth::fit_block(char mode, int width, int height, int depth) {
    // Check memoized fit result for current generation
    FitKey fk{mode,width,height,depth,generation};
    Block cachedBlock(0,0,0,0,0,0,'\0');
    if (cache_fit.get(fk, cachedBlock)) {
        // Re-validate window and uncompressed mask for safety (cheap with caches)
        if (window_is_all(mode, cachedBlock.z_offset, cachedBlock.z_offset+cachedBlock.depth,
                          cachedBlock.y_offset, cachedBlock.y_offset+cachedBlock.height,
                          cachedBlock.x_offset, cachedBlock.x_offset+cachedBlock.width) &&
            window_is_all_uncompressed(cachedBlock.z_offset, cachedBlock.z_offset+cachedBlock.depth,
                                       cachedBlock.y_offset, cachedBlock.y_offset+cachedBlock.height,
                                       cachedBlock.x_offset, cachedBlock.x_offset+cachedBlock.width)) {
            mark_compressed(cachedBlock.z_offset, cachedBlock.z_offset+cachedBlock.depth,
                            cachedBlock.y_offset, cachedBlock.y_offset+cachedBlock.height,
                            cachedBlock.x_offset, cachedBlock.x_offset+cachedBlock.width, true);
            return cachedBlock;
        }
    }
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
                    cache_fit.put(fk, b);
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
    WindowKey key{val,z0,z1,y0,y1,x0,x1,generation};
    bool cached;
    if (cache_window_all.get(key, cached)) return cached;
    for (int z = z0; z < z1; ++z)
        for (int y = y0; y < y1; ++y)
            for (int x = x0; x < x1; ++x)
                if (model[z][y][x] != val) { cache_window_all.put(key, false); return false; }
    cache_window_all.put(key, true);
    return true;
}

bool BlockGrowth::window_is_all_uncompressed(int z0, int z1, int y0, int y1, int x0, int x1) const {
    WindowUncKey key{z0,z1,y0,y1,x0,x1,generation};
    bool cached;
    if (cache_window_unc.get(key, cached)) return cached;
    for (int z = z0; z < z1; ++z)
        for (int y = y0; y < y1; ++y)
            for (int x = x0; x < x1; ++x)
                if (compressed[z][y][x]) { cache_window_unc.put(key, false); return false; }
    cache_window_unc.put(key, true);
    return true;
}

void BlockGrowth::mark_compressed(int z0, int z1, int y0, int y1, int x0, int x1, bool v) {
    for (int z = z0; z < z1; ++z)
        for (int y = y0; y < y1; ++y)
            for (int x = x0; x < x1; ++x)
                compressed[z][y][x] = v;
    generation++;
}

void BlockGrowth::grow_block(Block& current, Block& best_block) {
    GrowKey gk{current.x_offset, current.y_offset, current.z_offset,
               current.width, current.height, current.depth, current.tag, generation};
    Block memo(0,0,0,0,0,0,'\0');
    if (cache_grow.get(gk, memo)) {
        best_block = memo;
        return;
    }

    Block b = current;

    // Removed greedy pre-pass; rely on original recursive exploration

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
    cache_grow.put(gk, best_block);
}