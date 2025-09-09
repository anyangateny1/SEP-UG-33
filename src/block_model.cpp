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
    string line;
    getline_strict(line);
    parse_spec_line_six_ints(line, x_count, y_count, z_count, parent_x, parent_y, parent_z);
}

void BlockModel::read_tag_table() {
    tag_table.clear();
    string line;
    while (true) {
        if (!std::getline(std::cin, line)) { line.clear(); }
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (is_empty_line(line)) break;

        auto pos = line.find(", ");
        if (pos == string::npos || pos == 0 || pos + 2 > line.size())
            throw std::runtime_error("Invalid tag table line: " + line);

        char tag = line[0];
        string label = line.substr(pos + 2);
        tag_table[tag] = label;
    }
}

void BlockModel::read_model() {
    // allocate ring buffer storage once
    model_storage.assign(static_cast<size_t>(parent_z) * y_count * x_count, '\0');
    model.reset(parent_z, y_count, x_count, model_storage.data());

    int top_slice = 0;
    int n_slices = parent_z;

    string line;
    for (int z = 0; z < z_count; ++z) {
        for (int y = 0; y < y_count; ++y) {
            getline_strict(line);
            if ((int)line.size() < x_count)
                throw std::runtime_error("Model row shorter than x_count.");
            for (int x = 0; x < x_count; ++x) model.at(z % parent_z, y, x) = line[x];
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
    if (s.empty()) return true;
    if (s.size() == 1 && (s[0] == '\r' || s[0] == '\n')) return true;
    return false;
}

void BlockModel::getline_strict(string& out) {
    if (!std::getline(std::cin, out)) out.clear();
    if (!out.empty() && out.back() == '\r') out.pop_back();
}

void BlockModel::parse_spec_line_six_ints(const string& line,
                                          int& a0, int& a1, int& a2,
                                          int& a3, int& a4, int& a5) {
    long vals[6] = {0,0,0,0,0,0};
    int vi = 0;
    long sign = 1;
    long cur = 0;
    bool in_num = false;
    for (char ch : line) {
        if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n') continue;
        if (ch == ',') {
            if (vi >= 6) throw std::runtime_error("Too many fields in spec line");
            vals[vi++] = sign * cur;
            sign = 1; cur = 0; in_num = false;
            continue;
        }
        if (ch == '-') { sign = -1; in_num = true; continue; }
        if (ch < '0' || ch > '9') throw std::runtime_error("Invalid char in spec line");
        cur = cur * 10 + (ch - '0');
        in_num = true;
    }
    if (in_num) {
        if (vi >= 6) throw std::runtime_error("Too many fields in spec line");
        vals[vi++] = sign * cur;
    }
    if (vi != 6) throw std::runtime_error("Invalid specification line (need 6 ints).");
    a0 = static_cast<int>(vals[0]);
    a1 = static_cast<int>(vals[1]);
    a2 = static_cast<int>(vals[2]);
    a3 = static_cast<int>(vals[3]);
    a4 = static_cast<int>(vals[4]);
    a5 = static_cast<int>(vals[5]);
}

void BlockModel::slice_model(const Flat3D<char>& src,
                             Flat3D<char>& dst,
                             int depth, int y0, int y1, int x0, int x1) {
    for (int z = 0; z < depth; ++z)
        for (int y = y0; y < y1; ++y)
            for (int x = x0; x < x1; ++x)
                dst.at(z, y - y0, x - x0) = src.at(z, y, x);
}

void BlockModel::compress_slices(int top_slice, int n_slices) {
    for (int y = 0; y < y_count; y += parent_y) {
        for (int x = 0; x < x_count; x += parent_x) {
            int z = top_slice;
            int width  = std::min(parent_x, x_count - x);
            int height = std::min(parent_y, y_count - y);
            int depth  = n_slices;
            char tag = model.at(z % parent_z, y, x);

            Block parentBlock(x, y, z, width, height, depth, tag);
            // prepare reusable slice buffer
            const size_t slice_size = static_cast<size_t>(depth) * height * width;
            if (slice_storage.size() < slice_size) slice_storage.resize(slice_size);
            slice_view.reset(depth, height, width, slice_storage.data());
            slice_model(model, slice_view, depth, y, parentBlock.y_end, x, parentBlock.x_end);

            BlockGrowth growth(slice_view, tag_table);
            growth.run(parentBlock);
        }
    }
}
