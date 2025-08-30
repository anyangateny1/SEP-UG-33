#include "block_growth.h"
#include <stdexcept>
#include <algorithm>
#include <iostream>
#include <stack>
#include <vector>
#include <set>
#include <thread>
#include <future>
#include <mutex>
#include <queue>
#include <cmath>

using std::string;
using std::unordered_map;

BlockGrowth::BlockGrowth(const Vec3<char>& model_slices,
                         const unordered_map<char, string>& tag_table)
    : model(model_slices), tag_table(tag_table) {}

void BlockGrowth::enable_internal_multithreading(bool enable, size_t thread_count) {
    internal_multithreading_enabled = enable;
    if (thread_count > 0) {
        internal_thread_count = thread_count;
    } else {
        internal_thread_count = std::max(static_cast<size_t>(2), std::min(ThreadPoolStd::get_optimal_thread_count(), static_cast<size_t>(2)));
    }
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
    
    // Initialize uncompressed count
    uncompressed_count = parent_block.width * parent_block.height * parent_block.depth;

    // Loop until the entire parent block region is compressed
    while (!all_compressed()) {
        // Use simple mode selection for maximum speed
        char mode = get_mode_of_uncompressed(parent_block);
        int cube_size = std::min({parent_block.width, parent_block.height, parent_block.depth});
        
        // Use fast DFS with multi-threading for speed + compression quality
        Block b = internal_multithreading_enabled ? 
                 fit_block_with_parallel_dfs(mode, cube_size, cube_size, cube_size) :
                 fit_block_with_dfs_exploration(mode, cube_size, cube_size, cube_size);

        // Lookup label and print (exact same output format as Python)
        auto it = tag_table.find(b.tag);
        const string& label = (it == tag_table.end()) ? string(1, b.tag) : it->second;
        b.print_block(label);
    }
}

// Removed: all_compressed() is now inline in header for performance

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
    // Optimization: Try larger blocks first for better compression ratio
    for (int current_depth = depth; current_depth >= 1; --current_depth) {
        for (int current_height = height; current_height >= 1; --current_height) {
            for (int current_width = width; current_width >= 1; --current_width) {
                
                // Scan through the parent block region to find first fitting window
                for (int z = parent_block.z; z < parent_block.z_end; ++z) {
                    int z_off = z - parent_block.z;
                    int z_end = z_off + current_depth;
                    if (z_end > parent_z_end) continue;

                    for (int y = parent_block.y; y < parent_block.y_end; ++y) {
                        int y_off = y - parent_block.y;
                        int y_end = y_off + current_height;
                        if (y_end > parent_y_end) continue;

                        for (int x = parent_block.x; x < parent_block.x_end; ++x) {
                            int x_off = x - parent_block.x;
                            int x_end = x_off + current_width;
                            if (x_end > parent_x_end) continue;

                            // Check uncompressed first (faster early termination)
                            if (window_is_all_uncompressed(z_off, z_end, y_off, y_end, x_off, x_end) &&
                                window_is_all(mode, z_off, z_end, y_off, y_end, x_off, x_end)) {

                                Block b(x, y, z, current_width, current_height, current_depth, mode, x_off, y_off, z_off);
                                mark_compressed(z_off, z_end, y_off, y_end, x_off, x_end, true);
                                grow_block(b);
                                return b;
                            }
                        }
                    }
                }
            }
        }
    }
    
    throw std::runtime_error("No fitting block found at minimal size.");
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
    for (int z = z0; z < z1; ++z) {
        for (int y = y0; y < y1; ++y) {
            for (int x = x0; x < x1; ++x) {
                if (!compressed[z][y][x] && v) {
                    --uncompressed_count;
                }
                compressed[z][y][x] = v;
            }
        }
    }
}

void BlockGrowth::grow_block(Block& b) {
    int x = b.x_offset, y = b.y_offset, z = b.z_offset;
    int x_end = x + b.width;
    int y_end = y + b.height;
    int z_end = z + b.depth;

    bool grew = true;
    while (grew) {
        grew = false;

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
                for (int zz = z; zz < z_end; ++zz) {
                    for (int yy = y; yy < y_end; ++yy) {
                        compressed[zz][yy][x_end] = true;
                        --uncompressed_count;
                    }
                }
                x_end += 1;
                grew = true;
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
                for (int zz = z; zz < z_end; ++zz) {
                    for (int xx = x; xx < x_end; ++xx) {
                        compressed[zz][y_end][xx] = true;
                        --uncompressed_count;
                    }
                }
                y_end += 1;
                grew = true;
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
                for (int yy = y; yy < y_end; ++yy) {
                    for (int xx = x; xx < x_end; ++xx) {
                        compressed[z_end][yy][xx] = true;
                        --uncompressed_count;
                    }
                }
                z_end += 1;
                grew = true;
            }
        }
    }
}

std::string BlockGrowth::run_and_capture(Block parent_block_) {
    parent_block = parent_block_;
    parent_x_end = parent_block.x_offset + parent_block.width;
    parent_y_end = parent_block.y_offset + parent_block.height;
    parent_z_end = parent_block.z_offset + parent_block.depth;

    // Initialise compressed mask to false
    compressed = Vec3<bool>(
        parent_block.depth,
        std::vector<std::vector<bool>>(parent_block.height, std::vector<bool>(parent_block.width, false))
    );
    
    // Initialize uncompressed count
    uncompressed_count = parent_block.width * parent_block.height * parent_block.depth;

    std::ostringstream output;
    
    // Loop until the entire parent block region is compressed
    while (!all_compressed()) {
        // Use simple mode selection for maximum speed
        char mode = get_mode_of_uncompressed(parent_block);
        int cube_size = std::min({parent_block.width, parent_block.height, parent_block.depth});
        
        // Use fast DFS with multi-threading for speed + compression quality
        Block b = internal_multithreading_enabled ? 
                 fit_block_with_parallel_dfs(mode, cube_size, cube_size, cube_size) :
                 fit_block_with_dfs_exploration(mode, cube_size, cube_size, cube_size);

        // Lookup label and print to stringstream (exact same output format as Python)
        auto it = tag_table.find(b.tag);
        const string& label = (it == tag_table.end()) ? string(1, b.tag) : it->second;
        
        // Print to string stream instead of cout
        output << b.x << "," << b.y << "," << b.z << ","
               << b.width << "," << b.height << "," << b.depth << ","
               << label << "\n";
    }
    
    return output.str();
}

// DFS-based compression improvements

std::vector<Block> BlockGrowth::find_connected_region_dfs(char target_char, int start_z, int start_y, int start_x) {
    std::vector<Block> region_blocks;
    std::set<std::tuple<int, int, int>> visited;
    std::stack<std::tuple<int, int, int>> dfs_stack;
    
    // Start DFS from the given position
    dfs_stack.push({start_z, start_y, start_x});
    
    while (!dfs_stack.empty()) {
        auto [z, y, x] = dfs_stack.top();
        dfs_stack.pop();
        
        // Skip if out of bounds or already visited
        if (z < 0 || z >= parent_block.depth || 
            y < 0 || y >= parent_block.height || 
            x < 0 || x >= parent_block.width ||
            visited.count({z, y, x}) || 
            compressed[z][y][x] ||
            model[z][y][x] != target_char) {
            continue;
        }
        
        visited.insert({z, y, x});
        
        // Try to form a block starting from this position
        int max_width = 1, max_height = 1, max_depth = 1;
        
        // Expand in each direction to find the largest possible block
        while (x + max_width < parent_block.width && 
               window_is_all(target_char, z, z + 1, y, y + 1, x, x + max_width + 1) &&
               window_is_all_uncompressed(z, z + 1, y, y + 1, x, x + max_width + 1)) {
            max_width++;
        }
        
        while (y + max_height < parent_block.height && 
               window_is_all(target_char, z, z + 1, y, y + max_height + 1, x, x + max_width) &&
               window_is_all_uncompressed(z, z + 1, y, y + max_height + 1, x, x + max_width)) {
            max_height++;
        }
        
        while (z + max_depth < parent_block.depth && 
               window_is_all(target_char, z, z + max_depth + 1, y, y + max_height, x, x + max_width) &&
               window_is_all_uncompressed(z, z + max_depth + 1, y, y + max_height, x, x + max_width)) {
            max_depth++;
        }
        
        // Create block and add to region
        Block b(parent_block.x + x, parent_block.y + y, parent_block.z + z, 
                max_width, max_height, max_depth, target_char, x, y, z);
        region_blocks.push_back(b);
        
        // Add neighbors to DFS stack
        std::vector<std::tuple<int, int, int>> neighbors = {
            {z-1, y, x}, {z+1, y, x},
            {z, y-1, x}, {z, y+1, x},
            {z, y, x-1}, {z, y, x+1}
        };
        
        for (auto neighbor : neighbors) {
            dfs_stack.push(neighbor);
        }
    }
    
    return region_blocks;
}

Block BlockGrowth::fit_block_with_dfs_exploration(char mode, int max_width, int max_height, int max_depth) {
    Block best_block(0, 0, 0, 0, 0, 0, mode);
    int best_volume = 0;
    
    // Try DFS from multiple starting points to find the best block arrangement
    for (int z = 0; z < parent_block.depth; ++z) {
        for (int y = 0; y < parent_block.height; ++y) {
            for (int x = 0; x < parent_block.width; ++x) {
                
                if (compressed[z][y][x] || model[z][y][x] != mode) {
                    continue;
                }
                
                // Try different block sizes starting from this position
                for (int d = std::min(max_depth, parent_block.depth - z); d >= 1; --d) {
                    for (int h = std::min(max_height, parent_block.height - y); h >= 1; --h) {
                        for (int w = std::min(max_width, parent_block.width - x); w >= 1; --w) {
                            
                            // Check if this block fits
                            if (window_is_all_uncompressed(z, z + d, y, y + h, x, x + w) &&
                                window_is_all(mode, z, z + d, y, y + h, x, x + w)) {
                                
                                int volume = w * h * d;
                                if (volume > best_volume) {
                                    best_volume = volume;
                                    best_block = Block(parent_block.x + x, parent_block.y + y, parent_block.z + z,
                                                     w, h, d, mode, x, y, z);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
    // If we found a good block, mark it as compressed and grow it
    if (best_volume > 0) {
        mark_compressed(best_block.z_offset, best_block.z_offset + best_block.depth,
                       best_block.y_offset, best_block.y_offset + best_block.height,
                       best_block.x_offset, best_block.x_offset + best_block.width, true);
        grow_block(best_block);
        return best_block;
    }
    
    // Fallback to original algorithm if DFS doesn't find anything
    return fit_block(mode, max_width, max_height, max_depth);
}

int BlockGrowth::calculate_compression_score(const std::vector<Block>& blocks) const {
    int total_volume = 0;
    int total_blocks = blocks.size();
    
    for (const auto& block : blocks) {
        total_volume += block.width * block.height * block.depth;
    }
    
    // Better compression = higher volume per block (fewer total blocks)
    return total_blocks > 0 ? total_volume / total_blocks : 0;
}

// Parallel DFS implementations

Block BlockGrowth::fit_block_with_parallel_dfs(char mode, int max_width, int max_height, int max_depth) {
    Block best_block(0, 0, 0, 0, 0, 0, mode);
    int best_volume = 0;
    
    // Divide the search space among threads
    std::vector<std::future<std::pair<Block, int>>> futures;
    
    int z_chunk_size = std::max(1, parent_block.depth / (int)internal_thread_count);
    int y_chunk_size = std::max(1, parent_block.height / (int)internal_thread_count);
    
    // Launch parallel search tasks
    for (size_t t = 0; t < internal_thread_count; ++t) {
        int z_start = t * z_chunk_size;
        int z_end = (t == internal_thread_count - 1) ? parent_block.depth : (t + 1) * z_chunk_size;
        
        int y_start = 0;
        int y_end = parent_block.height;
        
        if (z_start >= parent_block.depth) break;
        
        futures.push_back(std::async(std::launch::async, [this, mode, max_width, max_height, max_depth, z_start, z_end, y_start, y_end]() {
            Block thread_best_block(0, 0, 0, 0, 0, 0, mode);
            int thread_best_volume = 0;
            
            // Search assigned region
            for (int z = z_start; z < z_end; ++z) {
                for (int y = y_start; y < y_end; ++y) {
                    for (int x = 0; x < parent_block.width; ++x) {
                        
                        if (compressed[z][y][x] || model[z][y][x] != mode) {
                            continue;
                        }
                        
                        // Try different block sizes starting from this position
                        for (int d = std::min(max_depth, parent_block.depth - z); d >= 1; --d) {
                            for (int h = std::min(max_height, parent_block.height - y); h >= 1; --h) {
                                for (int w = std::min(max_width, parent_block.width - x); w >= 1; --w) {
                                    
                                    // Check if this block fits
                                    if (window_is_all_uncompressed(z, z + d, y, y + h, x, x + w) &&
                                        window_is_all(mode, z, z + d, y, y + h, x, x + w)) {
                                        
                                        int volume = w * h * d;
                                        if (volume > thread_best_volume) {
                                            thread_best_volume = volume;
                                            thread_best_block = Block(parent_block.x + x, parent_block.y + y, parent_block.z + z,
                                                                     w, h, d, mode, x, y, z);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            
            return std::make_pair(thread_best_block, thread_best_volume);
        }));
    }
    
    // Collect results from all threads
    for (auto& future : futures) {
        auto [thread_block, thread_volume] = future.get();
        if (thread_volume > best_volume) {
            best_volume = thread_volume;
            best_block = thread_block;
        }
    }
    
    // If we found a good block, mark it as compressed and grow it
    if (best_volume > 0) {
        mark_compressed(best_block.z_offset, best_block.z_offset + best_block.depth,
                       best_block.y_offset, best_block.y_offset + best_block.height,
                       best_block.x_offset, best_block.x_offset + best_block.width, true);
        grow_block(best_block);
        return best_block;
    }
    
    // Fallback to original algorithm if parallel DFS doesn't find anything
    return fit_block(mode, max_width, max_height, max_depth);
}

char BlockGrowth::get_mode_of_uncompressed_parallel(const Block& blk) const {
    int z0 = blk.z_offset, z1 = blk.z_offset + blk.depth;
    int y0 = blk.y_offset, y1 = blk.y_offset + blk.height;
    int x0 = blk.x_offset, x1 = blk.x_offset + blk.width;

    // Parallel frequency counting
    std::vector<std::future<std::array<int, 256>>> futures;
    
    int z_chunk_size = std::max(1, (z1 - z0) / (int)internal_thread_count);
    
    // Launch parallel frequency counting tasks
    for (size_t t = 0; t < internal_thread_count; ++t) {
        int z_start = z0 + t * z_chunk_size;
        int z_end = (t == internal_thread_count - 1) ? z1 : z0 + (t + 1) * z_chunk_size;
        
        if (z_start >= z1) break;
        
        futures.push_back(std::async(std::launch::async, [this, z_start, z_end, y0, y1, x0, x1]() {
            std::array<int, 256> thread_freq{};
            
            for (int z = z_start; z < z_end; ++z) {
                for (int y = y0; y < y1; ++y) {
                    for (int x = x0; x < x1; ++x) {
                        if (!compressed[z][y][x]) {
                            unsigned char uc = static_cast<unsigned char>(model[z][y][x]);
                            ++thread_freq[uc];
                        }
                    }
                }
            }
            
            return thread_freq;
        }));
    }
    
    // Combine results from all threads
    std::array<int, 256> combined_freq{};
    for (auto& future : futures) {
        auto thread_freq = future.get();
        for (int i = 0; i < 256; ++i) {
            combined_freq[i] += thread_freq[i];
        }
    }
    
    // Find best character
    char best = 0;
    int bestCount = -1;
    for (int i = 0; i < 256; ++i) {
        if (combined_freq[i] > bestCount) {
            bestCount = combined_freq[i];
            best = static_cast<char>(i);
        }
    }
    
    return best;
}

// Advanced Compression Algorithms for Maximum Compression Ratio

Block BlockGrowth::fit_block_with_global_optimization(char mode, int max_width, int max_height, int max_depth) {
    // Compression-focused approach: Try largest rectangular blocks first
    Block largest_rect = find_largest_connected_region(mode);
    if (largest_rect.width > 0) {
        mark_compressed(largest_rect.z_offset, largest_rect.z_offset + largest_rect.depth,
                       largest_rect.y_offset, largest_rect.y_offset + largest_rect.height,
                       largest_rect.x_offset, largest_rect.x_offset + largest_rect.width, true);
        grow_block(largest_rect);
        return largest_rect;
    }
    
    // Fallback to enhanced DFS for maximum compression
    return internal_multithreading_enabled ? 
           fit_block_with_parallel_dfs(mode, max_width, max_height, max_depth) :
           fit_block_with_dfs_exploration(mode, max_width, max_height, max_depth);
}

char BlockGrowth::get_optimal_mode_with_clustering(const Block& blk) const {
    int z0 = blk.z_offset, z1 = blk.z_offset + blk.depth;
    int y0 = blk.y_offset, y1 = blk.y_offset + blk.height;
    int x0 = blk.x_offset, x1 = blk.x_offset + blk.width;

    std::array<int, 256> freq{};
    std::array<int, 256> clustering_score{};
    
    // Count frequencies and calculate clustering scores
    for (int z = z0; z < z1; ++z) {
        for (int y = y0; y < y1; ++y) {
            for (int x = x0; x < x1; ++x) {
                if (!compressed[z][y][x]) {
                    unsigned char uc = static_cast<unsigned char>(model[z][y][x]);
                    ++freq[uc];
                    
                    // Clustering bonus: check if surrounded by same material
                    int neighbors = 0;
                    for (int dz = -1; dz <= 1; ++dz) {
                        for (int dy = -1; dy <= 1; ++dy) {
                            for (int dx = -1; dx <= 1; ++dx) {
                                if (dz == 0 && dy == 0 && dx == 0) continue;
                                int nz = z + dz, ny = y + dy, nx = x + dx;
                                if (nz >= z0 && nz < z1 && ny >= y0 && ny < y1 && 
                                    nx >= x0 && nx < x1 && !compressed[nz][ny][nx] &&
                                    model[nz][ny][nx] == model[z][y][x]) {
                                    neighbors++;
                                }
                            }
                        }
                    }
                    clustering_score[uc] += neighbors;
                }
            }
        }
    }
    
    // Find best character combining frequency and clustering
    char best = 0;
    int best_combined_score = -1;
    for (int i = 0; i < 256; ++i) {
        if (freq[i] > 0) {
            // Combined score: frequency * clustering bonus
            int combined = freq[i] * 100 + clustering_score[i] * 50;
            if (combined > best_combined_score) {
                best_combined_score = combined;
                best = static_cast<char>(i);
            }
        }
    }
    
    return best;
}

Block BlockGrowth::find_largest_connected_region(char mode) const {
    Block largest_block(0, 0, 0, 0, 0, 0, mode);
    int largest_volume = 0;
    
    // Fast heuristic: Look for large rectangular regions instead of full BFS
    // This is much faster but still finds good compression opportunities
    for (int z = 0; z < parent_block.depth; ++z) {
        for (int y = 0; y < parent_block.height; ++y) {
            for (int x = 0; x < parent_block.width; ++x) {
                if (!compressed[z][y][x] && model[z][y][x] == mode) {
                    
                    // Try to grow a rectangular region from this seed point
                    int max_width = 1, max_height = 1, max_depth = 1;
                    
                    // Grow width
                    while (x + max_width < parent_block.width) {
                        bool can_grow = true;
                        for (int zz = z; zz < z + max_depth && can_grow; ++zz) {
                            for (int yy = y; yy < y + max_height && can_grow; ++yy) {
                                if (compressed[zz][yy][x + max_width] || 
                                    model[zz][yy][x + max_width] != mode) {
                                    can_grow = false;
                                }
                            }
                        }
                        if (!can_grow) break;
                        max_width++;
                    }
                    
                    // Grow height
                    while (y + max_height < parent_block.height) {
                        bool can_grow = true;
                        for (int zz = z; zz < z + max_depth && can_grow; ++zz) {
                            for (int xx = x; xx < x + max_width && can_grow; ++xx) {
                                if (compressed[zz][y + max_height][xx] || 
                                    model[zz][y + max_height][xx] != mode) {
                                    can_grow = false;
                                }
                            }
                        }
                        if (!can_grow) break;
                        max_height++;
                    }
                    
                    // Grow depth
                    while (z + max_depth < parent_block.depth) {
                        bool can_grow = true;
                        for (int yy = y; yy < y + max_height && can_grow; ++yy) {
                            for (int xx = x; xx < x + max_width && can_grow; ++xx) {
                                if (compressed[z + max_depth][yy][xx] || 
                                    model[z + max_depth][yy][xx] != mode) {
                                    can_grow = false;
                                }
                            }
                        }
                        if (!can_grow) break;
                        max_depth++;
                    }
                    
                    int volume = max_width * max_height * max_depth;
                    if (volume > largest_volume && volume >= 8) { // Minimum size threshold
                        largest_volume = volume;
                        largest_block = Block(parent_block.x + x, parent_block.y + y, parent_block.z + z,
                                            max_width, max_height, max_depth, mode, x, y, z);
                    }
                }
            }
        }
    }
    
    return largest_block;
}

std::vector<Block> BlockGrowth::get_candidate_blocks(char mode, int max_width, int max_height, int max_depth) const {
    std::vector<Block> candidates;
    
    // Limit search space for performance while keeping quality
    int step_size = std::max(1, std::min(parent_block.width, parent_block.height) / 8);
    
    // Generate candidates with different strategies (optimized search)
    for (int z = 0; z < parent_block.depth; z += step_size) {
        for (int y = 0; y < parent_block.height; y += step_size) {
            for (int x = 0; x < parent_block.width; x += step_size) {
                if (compressed[z][y][x] || model[z][y][x] != mode) continue;
                
                // Strategy 1: Cube-shaped blocks (good compression) - Only try top 3 sizes
                for (int cube_size = std::min({max_width, max_height, max_depth, 
                                              parent_block.width - x, 
                                              parent_block.height - y,
                                              parent_block.depth - z}); 
                     cube_size >= 1 && candidates.size() < 20; --cube_size) {
                    if (window_is_all_uncompressed(z, z + cube_size, y, y + cube_size, x, x + cube_size) &&
                        window_is_all(mode, z, z + cube_size, y, y + cube_size, x, x + cube_size)) {
                        candidates.emplace_back(parent_block.x + x, parent_block.y + y, parent_block.z + z,
                                              cube_size, cube_size, cube_size, mode, x, y, z);
                        break; // Found a cube, move to next position
                    }
                }
                
                // Strategy 2: Rectangular blocks (balanced performance/compression)
                if (candidates.size() < 15) {
                    for (int w = std::min(max_width, parent_block.width - x); w >= 2; w -= 2) {
                        for (int h = std::min(max_height, parent_block.height - y); h >= 2; h -= 2) {
                            int d = std::min(max_depth, parent_block.depth - z);
                            if (window_is_all_uncompressed(z, z + d, y, y + h, x, x + w) &&
                                window_is_all(mode, z, z + d, y, y + h, x, x + w)) {
                                candidates.emplace_back(parent_block.x + x, parent_block.y + y, parent_block.z + z,
                                                      w, h, d, mode, x, y, z);
                                goto next_position; // Found a good block, move on
                            }
                        }
                    }
                }
                
                next_position:;
            }
        }
    }
    
    return candidates;
}

int BlockGrowth::calculate_fragmentation_score(const Block& candidate) const {
    int fragmentation = 0;
    int x = candidate.x_offset, y = candidate.y_offset, z = candidate.z_offset;
    
    // Check how this block would fragment the remaining space
    for (int dz = -1; dz <= candidate.depth; ++dz) {
        for (int dy = -1; dy <= candidate.height; ++dy) {
            for (int dx = -1; dx <= candidate.width; ++dx) {
                int nx = x + dx, ny = y + dy, nz = z + dz;
                if (nx >= 0 && nx < parent_block.width && 
                    ny >= 0 && ny < parent_block.height &&
                    nz >= 0 && nz < parent_block.depth && !compressed[nz][ny][nx]) {
                    
                    // Check if this creates isolated pockets
                    bool isolated = true;
                    for (auto [ddz, ddy, ddx] : std::vector<std::tuple<int,int,int>>{{-1,0,0},{1,0,0},{0,-1,0},{0,1,0},{0,0,-1},{0,0,1}}) {
                        int nnz = nz + ddz, nny = ny + ddy, nnx = nx + ddx;
                        if (nnz >= 0 && nnz < parent_block.depth && 
                            nny >= 0 && nny < parent_block.height &&
                            nnx >= 0 && nnx < parent_block.width && !compressed[nnz][nny][nnx]) {
                            isolated = false;
                            break;
                        }
                    }
                    if (isolated) fragmentation++;
                }
            }
        }
    }
    
    return fragmentation;
}

bool BlockGrowth::would_create_unfillable_gaps(const Block& candidate) const {
    // Check if placing this block would create gaps that can't be filled efficiently
    // This is a simplified heuristic - check for small isolated regions
    
    int gaps = 0;
    int x = candidate.x_offset, y = candidate.y_offset, z = candidate.z_offset;
    
    // Check immediate surroundings
    for (int dz = -1; dz <= candidate.depth; ++dz) {
        for (int dy = -1; dy <= candidate.height; ++dy) {
            for (int dx = -1; dx <= candidate.width; ++dx) {
                int nx = x + dx, ny = y + dy, nz = z + dz;
                if (nx >= 0 && nx < parent_block.width && 
                    ny >= 0 && ny < parent_block.height &&
                    nz >= 0 && nz < parent_block.depth && !compressed[nz][ny][nx]) {
                    
                    // Count neighboring uncompressed cells
                    int neighbors = 0;
                    for (auto [ddz, ddy, ddx] : std::vector<std::tuple<int,int,int>>{{-1,0,0},{1,0,0},{0,-1,0},{0,1,0},{0,0,-1},{0,0,1}}) {
                        int nnz = nz + ddz, nny = ny + ddy, nnx = nx + ddx;
                        if (nnz >= 0 && nnz < parent_block.depth && 
                            nny >= 0 && nny < parent_block.height &&
                            nnx >= 0 && nnx < parent_block.width && !compressed[nnz][nny][nnx]) {
                            neighbors++;
                        }
                    }
                    
                    // If only 1-2 neighbors, it might become an unfillable gap
                    if (neighbors <= 2) gaps++;
                }
            }
        }
    }
    
    return gaps > 3; // threshold for "too many gaps"
}

Block BlockGrowth::optimize_block_shape(const Block& initial_block) const {
    Block optimized = initial_block;
    
    // Try to make the block more cube-like (better for compression)
    int total_volume = optimized.width * optimized.height * optimized.depth;
    int target_side = static_cast<int>(std::round(std::cbrt(total_volume)));
    
    // Try different aspect ratios that maintain similar volume
    std::vector<std::tuple<int, int, int>> shape_candidates;
    for (int w = 1; w <= optimized.width * 2; ++w) {
        for (int h = 1; h <= optimized.height * 2; ++h) {
            int d = total_volume / (w * h);
            if (d > 0 && w * h * d == total_volume) {
                // Prefer more cube-like shapes
                int shape_score = std::abs(w - target_side) + std::abs(h - target_side) + std::abs(d - target_side);
                shape_candidates.emplace_back(shape_score, w, h);
            }
        }
    }
    
    // For now, return the original block (shape optimization can be complex)
    // This is a placeholder for more sophisticated shape optimization
    return optimized;
}