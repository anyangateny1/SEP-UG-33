#ifndef BLOCK_GROWTH_H
#define BLOCK_GROWTH_H

#include <vector>
#include <string>
#include <unordered_map>
#include <cstdint>
#include "lru_cache.h"
#include "block.h"

// 3D alias: [depth][height][width]
template<typename T>
using Vec3 = std::vector<std::vector<std::vector<T>>>;

// BlockGrowth encapsulates the "fit & grow" compression logic for a parent block
// over a sub-volume (model_slices). The tag_table maps single-char tags to labels.
class BlockGrowth {
public:
    BlockGrowth(const Vec3<char>& model_slices,
                const std::unordered_map<char, std::string>& tag_table);

    // Run the compression/growth algorithm on the given parent block.
    // Prints each fitted/grown block using Block::print_block(label).
    void run(Block parent_block);

private:
    // Inputs
    const Vec3<char>& model; // sub-volume: depth x height x width
    const std::unordered_map<char, std::string>& tag_table;

    // State for current run()
    Block parent_block{0,0,0,0,0,0,'\0'};
    int parent_x_end = 0, parent_y_end = 0, parent_z_end = 0;

    // Tracks which cells in 'model' have been compressed (claimed)
    Vec3<bool> compressed; // same shape as 'model'

    // Caching
    mutable uint64_t generation = 0; // bump when compressed mask changes

    // Keys
    struct WindowKey { char val; int z0,z1,y0,y1,x0,x1; uint64_t gen; };
    struct WindowUncKey { int z0,z1,y0,y1,x0,x1; uint64_t gen; };
    struct ModeKey { int x0,x1,y0,y1,z0,z1; uint64_t gen; };
    struct FitKey { char mode; int w,h,d; uint64_t gen; };
    struct GrowKey { int x,y,z,w,h,d; char tag; uint64_t gen; };

    struct WindowKeyHash { std::size_t operator()(const WindowKey& k) const; };
    struct WindowUncKeyHash { std::size_t operator()(const WindowUncKey& k) const; };
    struct ModeKeyHash { std::size_t operator()(const ModeKey& k) const; };
    struct FitKeyHash { std::size_t operator()(const FitKey& k) const; };
    struct GrowKeyHash { std::size_t operator()(const GrowKey& k) const; };

    struct WindowKeyEq { bool operator()(const WindowKey& a, const WindowKey& b) const; };
    struct WindowUncKeyEq { bool operator()(const WindowUncKey& a, const WindowUncKey& b) const; };
    struct ModeKeyEq { bool operator()(const ModeKey& a, const ModeKey& b) const; };
    struct FitKeyEq { bool operator()(const FitKey& a, const FitKey& b) const; };
    struct GrowKeyEq { bool operator()(const GrowKey& a, const GrowKey& b) const; };

    mutable LruCache<WindowKey, bool, WindowKeyHash, WindowKeyEq> cache_window_all{32768};
    mutable LruCache<WindowUncKey, bool, WindowUncKeyHash, WindowUncKeyEq> cache_window_unc{32768};
    mutable LruCache<ModeKey, char, ModeKeyHash, ModeKeyEq> cache_mode{8192};
    mutable LruCache<FitKey, Block, FitKeyHash, FitKeyEq> cache_fit{4096};
    mutable LruCache<GrowKey, Block, GrowKeyHash, GrowKeyEq> cache_grow{16384};

    

    // Helper functions
    bool all_compressed() const;
    char get_mode_of_uncompressed(const Block& blk) const;

    Block fit_block(char mode, int width, int height, int depth);
    void grow_block(Block& current, Block& best_block);

    bool window_is_all(char val,
                       int z0, int z1, int y0, int y1, int x0, int x1) const;
    bool window_is_all_uncompressed(int z0, int z1, int y0, int y1, int x0, int x1) const;
    void mark_compressed(int z0, int z1, int y0, int y1, int x0, int x1, bool v);
};

#endif // BLOCK_GROWTH_H
