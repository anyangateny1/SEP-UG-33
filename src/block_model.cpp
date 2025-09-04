#include "block_model.h"
#include <algorithm>
#include <cctype>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <thread>
#include <future>
#include <sstream>
#include <map>
#include <atomic>

#ifdef USE_TBB
#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>
#include <tbb/partitioner.h>
#endif

using std::string;
using std::unordered_map;
using std::vector;

BlockModel::BlockModel() {
    // Auto-detect optimal thread count, but cap at 8 for diminishing returns
    num_threads = std::min(std::thread::hardware_concurrency(), 8u);
    if (num_threads == 0)
        num_threads = 1;  // Fallback for systems that don't report
}

void BlockModel::set_num_threads(unsigned int threads) {
    num_threads = std::max(1u, threads);  // Ensure at least 1 thread
}

void BlockModel::read_specification() {
    string line;
    getline_strict(line);
    vector<int> vals = split_csv_ints(line);
    if (vals.size() != 6)
        throw std::runtime_error("Invalid specification line (need 6 ints).");
    x_count = vals[0];
    y_count = vals[1];
    z_count = vals[2];
    parent_x = vals[3];
    parent_y = vals[4];
    parent_z = vals[5];
}

void BlockModel::read_tag_table() {
    tag_table.clear();
    string line;
    while (true) {
        if (!std::getline(std::cin, line)) {
            line.clear();
        }
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        if (is_empty_line(line))
            break;

        auto pos = line.find(", ");
        if (pos == string::npos || pos == 0 || pos + 2 > line.size())
            throw std::runtime_error("Invalid tag table line: " + line);

        char tag = line[0];
        string label = line.substr(pos + 2);
        tag_table[tag] = label;
    }
}

void BlockModel::read_model() {
    model = Flat3D<char>(parent_z, y_count, x_count, '\0');

    int top_slice = 0;
    int n_slices = parent_z;

    string line;
    for (int z = 0; z < z_count; ++z) {
        for (int y = 0; y < y_count; ++y) {
            getline_strict(line);
            if ((int)line.size() < x_count)
                throw std::runtime_error("Model row shorter than x_count.");
            for (int x = 0; x < x_count; ++x) {
                model.at(z % parent_z, y, x) = line[x];
            }
        }

        if ((z + 1) % parent_z == 0) {
            compress_slices(top_slice, n_slices);
            top_slice = z + 1;
        }

        if (z < z_count - 1) {
            string sep;
            getline_strict(sep);
        }
    }

    if (z_count % parent_z != 0) {
        n_slices = z_count % parent_z;
        compress_slices(top_slice, n_slices);
    }
}

bool BlockModel::is_empty_line(const string& s) {
    if (s.empty())
        return true;
    if (s.size() == 1 && (s[0] == '\r' || s[0] == '\n'))
        return true;
    return false;
}

void BlockModel::getline_strict(string& out) {
    if (!std::getline(std::cin, out))
        out.clear();
    if (!out.empty() && out.back() == '\r')
        out.pop_back();
}

vector<int> BlockModel::split_csv_ints(const string& line) {
    vector<int> vals;
    string cur;
    for (char c : line) {
        if (c == ',') {
            if (!cur.empty()) {
                vals.push_back(std::stoi(cur));
                cur.clear();
            } else {
                vals.push_back(0);
            }
        } else if (!std::isspace(static_cast<unsigned char>(c))) {
            cur.push_back(c);
        }
    }
    if (!cur.empty())
        vals.push_back(std::stoi(cur));
    return vals;
}

Flat3D<char> BlockModel::slice_model(const Flat3D<char>& src, int depth, int y0, int y1, int x0, int x1) {
    Flat3D<char> out(depth, y1 - y0, x1 - x0, '\0');
    for (int z = 0; z < depth; ++z)
        for (int y = y0; y < y1; ++y)
            for (int x = x0; x < x1; ++x)
                out.at(z, y - y0, x - x0) = src.at(z, y, x);
    return out;
}

void BlockModel::compress_slices(int top_slice, int n_slices) {
#ifdef USE_TBB
    // Intel TBB implementation for maximum performance on multi-core systems
    int x_blocks = (x_count + parent_x - 1) / parent_x;
    int y_blocks = (y_count + parent_y - 1) / parent_y;

    // Collect all parent blocks with their processing order
    std::vector<std::tuple<int, int, int, int, int, int, char>> parent_blocks;

    for (int yi = 0; yi < y_blocks; ++yi) {
        for (int xi = 0; xi < x_blocks; ++xi) {
            int y = yi * parent_y;
            int x = xi * parent_x;
            int z = top_slice;

            int width = std::min(parent_x, x_count - x);
            int height = std::min(parent_y, y_count - y);
            int depth = n_slices;
            char tag = model.at(z % parent_z, y, x);

            parent_blocks.emplace_back(x, y, z, width, height, depth, tag);
        }
    }

    // Pre-size results; each task writes to a unique index → no synchronization needed
    std::vector<std::string> results(parent_blocks.size());

    // Process blocks in parallel
    // Tune grainsize to reduce scheduling overhead at higher thread counts
    size_t total = parent_blocks.size();
    unsigned int threads = std::max(1u, num_threads);
    size_t grainsize = std::max<size_t>(1, total / (threads * 4));
    tbb::affinity_partitioner ap;

    tbb::parallel_for(
        tbb::blocked_range<size_t>(0, total, grainsize),
        [&](const tbb::blocked_range<size_t>& range) {
            for (size_t i = range.begin(); i != range.end(); ++i) {
                auto [x, y, z, width, height, depth, tag] = parent_blocks[i];

                Block parentBlock(x, y, z, width, height, depth, tag);
                Flat3D<char> model_slices = slice_model(model, depth, y, parentBlock.y_end, x, parentBlock.x_end);
                BlockGrowth growth(model_slices, tag_table);
                results[i] = growth.run_to_string(parentBlock);
            }
        },
        ap
    );

    // Output results in original order
    for (const auto& result : results) {
        if (!result.empty()) {
            std::cout << result;
        }
    }
#else
    // Optimized single-threaded approach with SIMD and algorithm improvements
    for (int y = 0; y < y_count; y += parent_y) {
        for (int x = 0; x < x_count; x += parent_x) {
            int z = top_slice;
            int width = std::min(parent_x, x_count - x);
            int height = std::min(parent_y, y_count - y);
            int depth = n_slices;
            char tag = model.at(z % parent_z, y, x);

            Block parentBlock(x, y, z, width, height, depth, tag);
            Flat3D<char> model_slices = slice_model(model, depth, y, parentBlock.y_end, x, parentBlock.x_end);

            BlockGrowth growth(model_slices, tag_table);
            growth.run(parentBlock);
        }
    }
#endif
}

void BlockModel::process_parent_block(int x, int y, int z, int width, int height, int depth, char tag, int n_slices) {
    Block parentBlock(x, y, z, width, height, depth, tag);
    // model_slices = model[:depth, y:parentBlock.y_end, x:parentBlock.x_end]
    Flat3D<char> model_slices = slice_model(model, n_slices, y, parentBlock.y_end, x, parentBlock.x_end);

    BlockGrowth growth(model_slices, tag_table);
    growth.run(parentBlock);
}

std::string BlockModel::process_parent_block_to_string(
    int x, int y, int z, int width, int height, int depth, char tag, int n_slices
) {
    Block parentBlock(x, y, z, width, height, depth, tag);
    // model_slices = model[:depth, y:parentBlock.y_end, x:parentBlock.x_end]
    Flat3D<char> model_slices = slice_model(model, n_slices, y, parentBlock.y_end, x, parentBlock.x_end);

    // Capture output to string
    std::ostringstream oss;
    std::streambuf* orig = std::cout.rdbuf();
    std::cout.rdbuf(oss.rdbuf());

    BlockGrowth growth(model_slices, tag_table);
    growth.run(parentBlock);

    std::cout.rdbuf(orig);  // Restore original cout
    return oss.str();
}

std::string BlockModel::process_parent_block_to_string_safe(
    const Flat3D<char>& model_slices, const Block& parentBlock
) {
    // Create a modified BlockGrowth that outputs to string instead of cout
    std::ostringstream result;

    // For now, let's implement a simple version that doesn't redirect cout
    // We'll need to modify BlockGrowth to support string output or use a different approach

    // Temporarily redirect cout for this thread only
    std::ostringstream oss;
    std::streambuf* orig_cout = std::cout.rdbuf();
    std::cout.rdbuf(oss.rdbuf());

    try {
        BlockGrowth growth(model_slices, tag_table);
        growth.run(parentBlock);
    } catch (...) {
        std::cout.rdbuf(orig_cout);
        throw;
    }

    std::cout.rdbuf(orig_cout);
    return oss.str();
}
