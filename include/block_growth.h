#ifndef BLOCK_GROWTH_H
#define BLOCK_GROWTH_H

#include <vector>
#include <string>
#include <unordered_map>
#include <sstream>
#include <mutex>
#include <future>
#include "block.h"
#include "thread_pool_std.h"

// 3D alias: [depth][height][width]
template<typename T>
using Vec3 = std::vector<std::vector<std::vector<T>>>;

// BlockGrowth encapsulates the "fit & grow" compression logic for a parent block
// over a sub-volume (model_slices). The tag_table maps single-char tags to labels.
class BlockGrowth {
public:
    BlockGrowth(const Vec3<char>& model_slices,
                const std::unordered_map<char, std::string>& tag_table);
    
    // Enable/disable internal multi-threading for DFS operations
    void enable_internal_multithreading(bool enable = true, size_t thread_count = 0);

    // Run the compression/growth algorithm on the given parent block.
    // Prints each fitted/grown block using Block::print_block(label).
    void run(Block parent_block);
    
    // Run and capture output instead of printing directly
    std::string run_and_capture(Block parent_block);

private:
    // Inputs
    const Vec3<char>& model; // sub-volume: depth x height x width
    const std::unordered_map<char, std::string>& tag_table;

    // State for current run()
    Block parent_block{0,0,0,0,0,0,'\0'};
    int parent_x_end = 0, parent_y_end = 0, parent_z_end = 0;

    // Tracks which cells in 'model' have been compressed (claimed)
    Vec3<bool> compressed; // same shape as 'model'
    
    // Optimization: Track remaining uncompressed count
    int uncompressed_count = 0;
    
    // Internal multi-threading support
    bool internal_multithreading_enabled = false;
    size_t internal_thread_count = 2;
    mutable std::mutex best_block_mutex;

    // Helper functions
    bool all_compressed() const { return uncompressed_count == 0; }
    char get_mode_of_uncompressed(const Block& blk) const;

    Block fit_block(char mode, int width, int height, int depth);
    void  grow_block(Block& b);

    // DFS-based compression improvements
    std::vector<Block> find_connected_region_dfs(char target_char, int start_z, int start_y, int start_x);
    Block fit_block_with_dfs_exploration(char mode, int max_width, int max_height, int max_depth);
    Block fit_block_with_parallel_dfs(char mode, int max_width, int max_height, int max_depth);
    char get_mode_of_uncompressed_parallel(const Block& blk) const;
    int calculate_compression_score(const std::vector<Block>& blocks) const;
    
    // Parallel helper functions
    void parallel_block_search_worker(char mode, int z_start, int z_end, int y_start, int y_end, 
                                     int max_width, int max_height, int max_depth,
                                     Block& best_block, int& best_volume) const;
    
    // Advanced compression algorithms for better ratios
    Block fit_block_with_global_optimization(char mode, int max_width, int max_height, int max_depth);
    char get_optimal_mode_with_clustering(const Block& blk) const;
    Block find_largest_connected_region(char mode) const;
    std::vector<Block> get_candidate_blocks(char mode, int max_width, int max_height, int max_depth) const;
    int calculate_fragmentation_score(const Block& candidate) const;
    bool would_create_unfillable_gaps(const Block& candidate) const;
    Block optimize_block_shape(const Block& initial_block) const;

    bool window_is_all(char val,
                       int z0, int z1, int y0, int y1, int x0, int x1) const;
    bool window_is_all_uncompressed(int z0, int z1, int y0, int y1, int x0, int x1) const;
    void mark_compressed(int z0, int z1, int y0, int y1, int x0, int x1, bool v);
};

#endif // BLOCK_GROWTH_H
