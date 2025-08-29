#include "block_model.h"
#include <algorithm>
#include <cctype>
#include <iostream>
#include <stdexcept>
#include <vector>

using std::string;
using std::unordered_map;
using std::vector;

void BlockModel::read_specification() {
    // Line: x_count, y_count, z_count, parent_x, parent_y, parent_z
    string line;
    getline_strict(line);
    vector<int> vals = split_csv_ints(line);
    if (vals.size() != 6) throw std::runtime_error("Invalid specification line (need 6 ints).");
    x_count  = vals[0];
    y_count  = vals[1];
    z_count  = vals[2];
    parent_x = vals[3];
    parent_y = vals[4];
    parent_z = vals[5];
}

void BlockModel::read_tag_table() {
    tag_table.clear();
    string line;
    while (true) {
        if (!std::getline(std::cin, line)) { line.clear(); } // EOF -> treat as empty
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (is_empty_line(line)) break;

        // Expect format: "T, Label" (comma + space)
        auto pos = line.find(", ");
        if (pos == string::npos || pos == 0 || pos + 2 > line.size())
            throw std::runtime_error("Invalid tag table line: " + line);

        char tag = line[0]; // first character is the tag (Python used dtype 'U1')
        string label = line.substr(pos + 2);
        tag_table[tag] = label;
    }
}

void BlockModel::read_model() {
    // Allocate ring buffer depth = parent_z
    model.assign(parent_z,
        vector<vector<char>>(y_count, vector<char>(x_count, '\0')));

    int top_slice = 0;
    int n_slices = parent_z;

    string line;
    for (int z = 0; z < z_count; ++z) {
        for (int y = 0; y < y_count; ++y) {
            getline_strict(line);
            if ((int)line.size() < x_count)
                throw std::runtime_error("Model row shorter than x_count.");
            for (int x = 0; x < x_count; ++x) {
                model[z % parent_z][y][x] = line[x];
            }
        }

        // After each full parent_z-thickness batch, compress it
        if ((z + 1) % parent_z == 0) {
            compress_slices(top_slice, n_slices);
            top_slice = z + 1;
        }

        // Skip blank separator between slices (Python called input() unconditionally)
        if (z < z_count - 1) {
            string sep;
            getline_strict(sep);
            // We don't enforce emptiness; just consume one line to replicate Python behavior.
        }
    }

    // Handle trailing partial depth
    if (z_count % parent_z != 0) {
        n_slices = z_count % parent_z;
        compress_slices(top_slice, n_slices);
    }
}

// --------- helpers ---------
bool BlockModel::is_empty_line(const string& s) {
    if (s.empty()) return true;
    if (s.size() == 1 && (s[0] == '\r' || s[0] == '\n')) return true;
    return false;
}

void BlockModel::getline_strict(string& out) {
    if (!std::getline(std::cin, out)) out.clear(); // On EOF, act like Python input(): returns ""
    if (!out.empty() && out.back() == '\r') out.pop_back(); // Normalize CRLF
}

vector<int> BlockModel::split_csv_ints(const string& line) {
    vector<int> vals;
    string cur;
    for (char c : line) {
        if (c == ',') {
            if (!cur.empty()) { vals.push_back(std::stoi(cur)); cur.clear(); }
            else { vals.push_back(0); } // tolerate empty segment (unlikely here)
        } else if (!std::isspace(static_cast<unsigned char>(c))) {
            cur.push_back(c);
        }
    }
    if (!cur.empty()) vals.push_back(std::stoi(cur));
    return vals;
}

Vec3<char> BlockModel::slice_model(const Vec3<char>& src,
                                   int depth, int y0, int y1, int x0, int x1) {
    Vec3<char> out(depth, vector<vector<char>>(y1 - y0, vector<char>(x1 - x0, '\0')));
    for (int z = 0; z < depth; ++z)
        for (int y = y0; y < y1; ++y)
            for (int x = x0; x < x1; ++x)
                out[z][y - y0][x - x0] = src[z][y][x];
    return out;
}

void BlockModel::compress_slices(int top_slice, int n_slices) {
    for (int y = 0; y < y_count; y += parent_y) {
        for (int x = 0; x < x_count; x += parent_x) {
            int z = top_slice;
            int width  = std::min(parent_x, x_count - x);
            int height = std::min(parent_y, y_count - y);
            int depth  = n_slices;
            char tag = model[z % parent_z][y][x];

            Block parentBlock(x, y, z, width, height, depth, tag);
            // model_slices = model[:depth, y:parentBlock.y_end, x:parentBlock.x_end]
            Vec3<char> model_slices = slice_model(model, depth, y, parentBlock.y_end, x, parentBlock.x_end);

            BlockGrowth growth(model_slices, tag_table);
            growth.run(parentBlock);
        }
    }
}