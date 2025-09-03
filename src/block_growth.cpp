#include "block_growth.h"
#include <stdexcept>
#include <algorithm>
#include <sstream>
#include <set>

// SIMD optimization for window checking using XSimd (header-only, Windows compatible)
#ifdef __has_include
#if __has_include(<xsimd/xsimd.hpp>)
#define USE_XSIMD
#include <xsimd/xsimd.hpp>
#endif
#endif

// DirectXMath SIMD optimization for Windows (Microsoft's optimized SIMD library)
#ifdef USE_DIRECTXMATH
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <immintrin.h>  // For SSE/AVX intrinsics
using namespace DirectX;

// Windows AVX2/SSE4 intrinsics for enhanced SIMD performance
#define USE_AVX2_INTRINSICS
#endif

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

    // AGGRESSIVE COMPRESSION ALGORITHM - Maximum compression strategy
    // Loop until the entire parent block region is compressed
    while (!all_compressed()) {
        // Strategy 1: Look for the largest possible contiguous regions
        Block largest_block = find_largest_contiguous_block(parent_block);

        // Lookup label and print (exact same output format as Python)
        auto it = tag_table.find(largest_block.tag);
        const string& label = (it == tag_table.end()) ? string(1, largest_block.tag) : it->second;
        largest_block.print_block(label);
    }
}

bool BlockGrowth::all_compressed() const {
#ifdef USE_AVX2_INTRINSICS
    // AVX2 optimized version for faster boolean array checking
    // Note: Flat3D<char> uses char array, so we can optimize the iteration
    const size_t total_size = compressed.data.size();
    
    // For large arrays, process in chunks to improve cache performance
    if (total_size >= 64) {
        size_t processed = 0;
        const size_t chunk_size = 64;
        
        // Process 64 elements at a time for better cache utilization
        for (; processed + chunk_size <= total_size; processed += chunk_size) {
            bool chunk_all_true = true;
            for (size_t i = 0; i < chunk_size; ++i) {
                if (compressed.data[processed + i] == 0) {
                    chunk_all_true = false;
                    break;
                }
            }
            if (!chunk_all_true) return false;
        }
        
        // Handle remaining elements
        for (; processed < total_size; ++processed) {
            if (compressed.data[processed] == 0) return false;
        }
    } else {
        // For smaller arrays, use simple iteration
        for (char v : compressed.data) {
            if (v == 0) return false;
        }
    }
    return true;
#else
    // Original implementation
    for (char v : compressed.data)
        if (v == 0) return false;
    return true;
#endif
}

char BlockGrowth::get_mode_of_uncompressed(const Block& blk) const {
    // Optimized frequency counting for char tags in uncompressed cells
    int z0 = blk.z_offset, z1 = blk.z_offset + blk.depth;
    int y0 = blk.y_offset, y1 = blk.y_offset + blk.height;
    int x0 = blk.x_offset, x1 = blk.x_offset + blk.width;

    // Use aligned memory for frequency map to potentially benefit from cache optimization
    alignas(64) int freq[256] = {0}; // Align to cache line boundary
    
    // Cache-optimized iteration pattern
    for (int z = z0; z < z1; ++z) {
        for (int y = y0; y < y1; ++y) {
#ifdef USE_DIRECTXMATH
            // DirectXMath optimized processing for wide rows
            const int row_width = x1 - x0;
            int x = x0;
            
            // Process multiple elements at once when beneficial
            if (row_width >= 16) {
                const int simd_end = x0 + ((row_width / 16) * 16);
                for (; x < simd_end; x += 16) {
                    // Process 16 characters at a time
                    for (int i = 0; i < 16; ++i) {
                        if (compressed.at(z, y, x + i) == 0) {
                            unsigned char uc = static_cast<unsigned char>(model.at(z, y, x + i));
                            ++freq[uc];
                        }
                    }
                }
            }
            
            // Handle remaining elements
            for (; x < x1; ++x) {
                if (compressed.at(z, y, x) == 0) {
                    unsigned char uc = static_cast<unsigned char>(model.at(z, y, x));
                    ++freq[uc];
                }
            }
#else
            // Process row with potential for better vectorization by compiler
            for (int x = x0; x < x1; ++x) {
                if (compressed.at(z, y, x) == 0) {
                    unsigned char uc = static_cast<unsigned char>(model.at(z, y, x));
                    ++freq[uc];
                }
            }
#endif
        }
    }

#ifdef USE_DIRECTXMATH
    // DirectXMath optimized mode finding using SIMD for large frequency arrays
    char best = 0;
    int bestCount = -1;
    
    // Process frequency array in chunks of 4 integers at a time
    for (int i = 0; i < 256; i += 4) {
        // Load 4 frequency values
        const int remaining = std::min(4, 256 - i);
        for (int j = 0; j < remaining; ++j) {
            if (freq[i + j] > bestCount) {
                bestCount = freq[i + j];
                best = static_cast<char>(i + j);
            }
        }
    }
#else
    // Find mode - could be optimized with SIMD but typically frequency array is small
    char best = 0;
    int bestCount = -1;
    for (int i = 0; i < 256; ++i) {
        if (freq[i] > bestCount) {
            bestCount = freq[i];
            best = static_cast<char>(i);
        }
    }
#endif
    return best;
}

// AGGRESSIVE COMPRESSION: Find the optimal block for maximum compression
Block BlockGrowth::find_best_rectangular_block(const Block& parent_block) {
    std::vector<Block> all_candidates;
    
    // Get all available tags in uncompressed region
    std::set<char> available_tags = get_available_tags_in_uncompressed(parent_block);
    
    // For each tag, try MANY different block shapes to find the biggest possible blocks
    for (char tag : available_tags) {
        
        // Calculate maximum possible dimensions for this tag
        int max_w = parent_block.width;
        int max_h = parent_block.height; 
        int max_d = parent_block.depth;
        
        // Try blocks of different sizes - prioritize LARGE blocks
        std::vector<std::tuple<int,int,int>> shapes;
        
        // Add many different sized blocks, largest first
        for (int size_factor = std::min({max_w, max_h, max_d}); size_factor >= 1; size_factor /= 2) {
            // Try various aspect ratios at this size
            shapes.push_back({size_factor, size_factor, size_factor});                    // Cube
            shapes.push_back({size_factor * 2, size_factor, size_factor});                // Wide
            shapes.push_back({size_factor, size_factor * 2, size_factor});                // Tall  
            shapes.push_back({size_factor, size_factor, size_factor * 2});                // Deep
            shapes.push_back({size_factor * 3, size_factor, size_factor});                // Very wide
            shapes.push_back({size_factor, size_factor * 3, size_factor});                // Very tall
            shapes.push_back({size_factor, size_factor, size_factor * 3});                // Very deep
            shapes.push_back({size_factor * 2, size_factor * 2, size_factor});            // Wide tall
            shapes.push_back({size_factor * 4, size_factor, size_factor});                // Ultra wide
            shapes.push_back({size_factor, size_factor, size_factor * 4});                // Ultra deep
        }
        
        // Try each shape for this tag
        for (auto [w, h, d] : shapes) {
            if (w > 0 && h > 0 && d > 0 && w <= max_w && h <= max_h && d <= max_d) {
                try {
                    Block candidate = fit_block(tag, w, h, d);
                    all_candidates.push_back(candidate);
                } catch (...) {
                    // This shape doesn't fit, continue
                }
            }
        }
    }
    
    if (all_candidates.empty()) {
        // Fallback to any available tag with minimal size
        char mode = get_mode_of_uncompressed(parent_block);
        return fit_block(mode, 1, 1, 1);
    }
    
    // Sort by volume (largest first) and pick the biggest block we can fit
    std::sort(all_candidates.begin(), all_candidates.end(), 
        [](const Block& a, const Block& b) { return a.volume > b.volume; });
    
    return all_candidates[0]; // Return the largest volume block
}

// ULTIMATE COMPRESSION: Find the absolutely largest contiguous block possible
Block BlockGrowth::find_largest_contiguous_block(const Block& parent_block) {
    Block best_block{0,0,0,0,0,0,'\0'};
    best_block.volume = 0;
    
    // Get all available tags
    std::set<char> available_tags = get_available_tags_in_uncompressed(parent_block);
    
     // For each uncompressed position, try to grow the largest possible block
     for (int z = 0; z < parent_block.depth; ++z) {
         for (int y = 0; y < parent_block.height; ++y) {
             for (int x = 0; x < parent_block.width; ++x) {
                 if (compressed.at(z, y, x) == 0) {
                     char tag = model.at(z, y, x);
                    
                    // Try to grow the largest possible block from this position
                    Block candidate = grow_largest_block_from_position(x, y, z, tag);
                    
                    if (candidate.volume > best_block.volume) {
                        best_block = candidate;
                    }
                }
            }
        }
    }
    
    if (best_block.volume == 0) {
        // Fallback if nothing found
        char mode = get_mode_of_uncompressed(parent_block);
        return fit_block(mode, 1, 1, 1);
    }
    
     // Mark this block as compressed
     mark_compressed(best_block.z_offset, best_block.z_offset + best_block.depth,
                    best_block.y_offset, best_block.y_offset + best_block.height,
                    best_block.x_offset, best_block.x_offset + best_block.width, 1);
    
    return best_block;
}

// Grow the largest possible block from a specific starting position
Block BlockGrowth::grow_largest_block_from_position(int start_x, int start_y, int start_z, char tag) {
    // Find maximum dimensions in each direction from this starting point
    int max_width = 1, max_height = 1, max_depth = 1;
    
     // Find max width (X direction)
     for (int x = start_x + 1; x < parent_block.width; ++x) {
         if (compressed.at(start_z, start_y, x) != 0 || model.at(start_z, start_y, x) != tag) break;
         max_width++;
     }
     
     // Find max height (Y direction) 
     for (int y = start_y + 1; y < parent_block.height; ++y) {
         if (compressed.at(start_z, y, start_x) != 0 || model.at(start_z, y, start_x) != tag) break;
         max_height++;
     }
     
     // Find max depth (Z direction)
     for (int z = start_z + 1; z < parent_block.depth; ++z) {
         if (compressed.at(z, start_y, start_x) != 0 || model.at(z, start_y, start_x) != tag) break;
         max_depth++;
     }
    
    // Try different combinations of dimensions to find the largest valid block
    Block best_candidate{0,0,0,0,0,0,'\0'};
    best_candidate.volume = 0;
    
    for (int w = 1; w <= max_width; ++w) {
        for (int h = 1; h <= max_height; ++h) {
            for (int d = 1; d <= max_depth; ++d) {
                 // Check if this entire region has the same tag and is uncompressed
                 bool valid = true;
                 for (int z = start_z; z < start_z + d && valid; ++z) {
                     for (int y = start_y; y < start_y + h && valid; ++y) {
                         for (int x = start_x; x < start_x + w && valid; ++x) {
                             if (compressed.at(z, y, x) != 0 || model.at(z, y, x) != tag) {
                                 valid = false;
                             }
                         }
                     }
                 }
                
                if (valid && (w * h * d) > best_candidate.volume) {
                    best_candidate = Block(
                        parent_block.x + start_x, parent_block.y + start_y, parent_block.z + start_z,
                        w, h, d, tag, start_x, start_y, start_z
                    );
                }
            }
        }
    }
    
    return best_candidate;
}

// Helper function to get all available tags in uncompressed region
std::set<char> BlockGrowth::get_available_tags_in_uncompressed(const Block& blk) const {
    std::set<char> tags;
    
    int z0 = blk.z_offset, z1 = blk.z_offset + blk.depth;
    int y0 = blk.y_offset, y1 = blk.y_offset + blk.height;
    int x0 = blk.x_offset, x1 = blk.x_offset + blk.width;
    
     for (int z = z0; z < z1; ++z) {
         for (int y = y0; y < y1; ++y) {
             for (int x = x0; x < x1; ++x) {
                 if (compressed.at(z, y, x) == 0) {
                     tags.insert(model.at(z, y, x));
                 }
             }
         }
     }
    
    return tags;
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
#ifdef USE_DIRECTXMATH
    // DirectXMath + AVX2 SIMD optimization - optimized for Windows x64
    const int row_width = x1 - x0;
    
#ifdef USE_AVX2_INTRINSICS
     // AVX2 optimization: process 32 bytes at once
     if (row_width >= 32) {
         const __m256i val_vec = _mm256_set1_epi8(val);
         
         for (int z = z0; z < z1; ++z) {
             for (int y = y0; y < y1; ++y) {
                 // Use Flat3D access pattern - need to create temporary array for SIMD
                 std::vector<char> row_data(row_width);
                 for (int i = 0; i < row_width; ++i) {
                     row_data[i] = model.at(z, y, x0 + i);
                 }
                 const char* row = row_data.data();
                int x = 0;
                
                // Process 32 bytes at a time with AVX2
                const int avx2_end = (row_width / 32) * 32;
                for (; x < avx2_end; x += 32) {
                    __m256i data = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&row[x]));
                    __m256i cmp = _mm256_cmpeq_epi8(data, val_vec);
                    int mask = _mm256_movemask_epi8(cmp);
                    if (mask != 0xFFFFFFFF) { // Not all bytes match
                        return false;
                    }
                }
                
                // Handle remaining elements with SSE2 (16-byte chunks)
                const int sse_end = x + ((row_width - x) / 16) * 16;
                const __m128i val_vec_128 = _mm_set1_epi8(val);
                for (; x < sse_end; x += 16) {
                    __m128i data = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&row[x]));
                    __m128i cmp = _mm_cmpeq_epi8(data, val_vec_128);
                    int mask = _mm_movemask_epi8(cmp);
                    if (mask != 0xFFFF) {
                        return false;
                    }
                }
                
                // Handle final remaining elements (less than 16)
                for (; x < row_width; ++x) {
                    if (row[x] != val) return false;
                }
            }
        }
        return true;
    }
#endif
    
     // Fallback to SSE2 for medium-sized rows
     if (row_width >= 16) { // Use SIMD for sufficiently wide rows
         // Create a vector with the target value replicated across all lanes
         const __m128i val_vec = _mm_set1_epi8(val);
         
         for (int z = z0; z < z1; ++z) {
             for (int y = y0; y < y1; ++y) {
                 // Use Flat3D access pattern - need to create temporary array for SIMD
                 std::vector<char> row_data(row_width);
                 for (int i = 0; i < row_width; ++i) {
                     row_data[i] = model.at(z, y, x0 + i);
                 }
                 const char* row = row_data.data();
                int x = 0;
                
                // Process 16 bytes at a time with SSE2
                const int simd_end = (row_width / 16) * 16;
                for (; x < simd_end; x += 16) {
                    __m128i data = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&row[x]));
                    __m128i cmp = _mm_cmpeq_epi8(data, val_vec);
                    int mask = _mm_movemask_epi8(cmp);
                    if (mask != 0xFFFF) { // Not all bytes match
                        return false;
                    }
                }
                
                // Handle remaining elements (less than 16)
                for (; x < row_width; ++x) {
                    if (row[x] != val) return false;
                }
            }
        }
        return true;
    }
#elif defined(USE_XSIMD)
    // SIMD-optimized version using XSimd for better performance on large windows
    if ((x1 - x0) >= 16) { // Only use SIMD for sufficiently wide rows
        using batch_type = xsimd::batch<char>;
        constexpr size_t simd_size = batch_type::size;
        batch_type val_batch(val);
        
        for (int z = z0; z < z1; ++z) {
            for (int y = y0; y < y1; ++y) {
                const char* row = &model[z][y][x0];
                size_t row_width = x1 - x0;
                
                // Process SIMD-aligned chunks
                size_t simd_end = (row_width / simd_size) * simd_size;
                size_t i = 0;
                
                for (; i < simd_end; i += simd_size) {
                    batch_type data_batch = xsimd::load_unaligned(&row[i]);
                    auto cmp_result = xsimd::eq(data_batch, val_batch);
                    if (!xsimd::all(cmp_result)) {
                        return false;
                    }
                }
                
                // Handle remaining elements
                for (; i < row_width; ++i) {
                    if (row[i] != val) return false;
                }
            }
        }
        return true;
    }
#endif
    
    // Fallback: Optimized memory access pattern - check in cache-friendly order
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
#ifdef USE_AVX2_INTRINSICS
    // Enhanced AVX2-optimized loop unrolling for better cache performance
    // Note: vector<bool> is bit-packed, but we can optimize access patterns
     for (int z = z0; z < z1; ++z) {
         for (int y = y0; y < y1; ++y) {
             const int row_width = x1 - x0;
             int x = x0;
             
             // For larger rows, use aggressive unrolling (8-way)
             if (row_width >= 16) {
                 const int unroll_end = x0 + ((row_width / 8) * 8);
                 for (; x < unroll_end; x += 8) {
                     // 8-way loop unrolling for better instruction-level parallelism
                     if (compressed.at(z, y, x) != 0 || compressed.at(z, y, x+1) != 0 || 
                         compressed.at(z, y, x+2) != 0 || compressed.at(z, y, x+3) != 0 ||
                         compressed.at(z, y, x+4) != 0 || compressed.at(z, y, x+5) != 0 || 
                         compressed.at(z, y, x+6) != 0 || compressed.at(z, y, x+7) != 0) {
                         return false;
                     }
                 }
             } else {
                 // For smaller rows, use 4-way unrolling
                 const int unroll_end = x0 + ((row_width / 4) * 4);
                 for (; x < unroll_end; x += 4) {
                     if (compressed.at(z, y, x) != 0 || compressed.at(z, y, x+1) != 0 || 
                         compressed.at(z, y, x+2) != 0 || compressed.at(z, y, x+3) != 0) {
                         return false;
                     }
                 }
             }
             
             // Handle remaining elements
             for (; x < x1; ++x) {
                 if (compressed.at(z, y, x) != 0) return false;
             }
         }
     }
    return true;
#else
    // Original optimized implementation
    // Note: vector<bool> is bit-packed, so SIMD optimization is complex and often not beneficial
    // Keep optimized loop with better cache access pattern
    for (int z = z0; z < z1; ++z) {
        for (int y = y0; y < y1; ++y) {
            // Direct access with optimized loop unrolling hint
            int x = x0;
            
            // Unroll loop for better performance (4-way unrolling)
            for (; x <= x1 - 4; x += 4) {
                if (compressed.at(z, y, x) != 0 || compressed.at(z, y, x+1) != 0 || 
                    compressed.at(z, y, x+2) != 0 || compressed.at(z, y, x+3) != 0) {
                    return false;
                }
            }
            
            // Handle remaining elements
            for (; x < x1; ++x) {
                if (compressed.at(z, y, x) != 0) return false;
            }
        }
    }
    return true;
#endif
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
    
    // AGGRESSIVE GROWTH: Try growing in ALL directions simultaneously
    bool can_grow_x = (x_end < parent_x_end);
    bool can_grow_y = (y_end < parent_y_end);
    bool can_grow_z = (z_end < parent_z_end);
    
    // Try combined growth (grow in multiple dimensions at once)
    if (can_grow_x && can_grow_y) {
        // Try growing both X and Y
        bool ok = true;
         for (int zz = z; zz < z_end && ok; ++zz) {
             // Check corner
             if (model.at(zz, y_end, x_end) != b.tag) ok = false;
             if (compressed.at(zz, y_end, x_end) != 0) ok = false;
             // Check new edges
             for (int xx = x; xx < x_end && ok; ++xx) {
                 if (model.at(zz, y_end, xx) != b.tag) ok = false;
                 if (compressed.at(zz, y_end, xx) != 0) ok = false;
             }
             for (int yy = y; yy < y_end && ok; ++yy) {
                 if (model.at(zz, yy, x_end) != b.tag) ok = false;
                 if (compressed.at(zz, yy, x_end) != 0) ok = false;
             }
         }
        if (ok) {
            b.set_width(b.width + 1);
            b.set_height(b.height + 1);
            current = b;
            grow_block(current, best_block);
            if (current.volume > best_block.volume)
                best_block = current;
            b.set_width(b.width - 1);
            b.set_height(b.height - 1);
        }
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

std::string BlockGrowth::run_to_string(Block parent_block_) {
    parent_block = parent_block_;
    parent_x_end = parent_block.x_offset + parent_block.width;
    parent_y_end = parent_block.y_offset + parent_block.height;
    parent_z_end = parent_block.z_offset + parent_block.depth;

    // Initialise compressed mask to 0 (false)
    compressed = Flat3D<char>(parent_block.depth, parent_block.height, parent_block.width, 0);

    std::ostringstream result;
    
    // AGGRESSIVE COMPRESSION ALGORITHM - same as run() but for thread-safe string output
    while (!all_compressed()) {
        // Strategy 1: Look for the largest possible contiguous regions
        Block largest_block = find_largest_contiguous_block(parent_block);

        // Lookup label and format output (same format as print_block)
        auto it = tag_table.find(largest_block.tag);
        const std::string& label = (it == tag_table.end()) ? std::string(1, largest_block.tag) : it->second;
        
        // Format: x,y,z,width,height,depth,label
        result << largest_block.x << "," << largest_block.y << "," << largest_block.z << "," 
               << largest_block.width << "," << largest_block.height << "," << largest_block.depth << "," << label << "\n";
    }
    
    return result.str();
}
