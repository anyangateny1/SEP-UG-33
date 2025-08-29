#include "block.h"
#include <iostream>

Block::Block(int x_, int y_, int z_,
             int w_, int h_, int d_,
             char tag_,
             int x_off, int y_off, int z_off)
    : x(x_), y(y_), z(z_),
      x_offset(x_off), y_offset(y_off), z_offset(z_off),
      width(w_), height(h_), depth(d_),
      x_end(x_ + w_), y_end(y_ + h_), z_end(z_ + d_),
      tag(tag_) {}

void Block::set_width(int w) {
    width = w;
    x_end = x + w;
}

void Block::set_height(int h) {
    height = h;
    y_end = y + h;
}

void Block::set_depth(int d) {
    depth = d;
    z_end = z + d;
}

void Block::print_block(const std::string& label) const {
    std::cout << x << "," << y << "," << z << ","
              << width << "," << height << "," << depth << ","
              << label << "\n";
}