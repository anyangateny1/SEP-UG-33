#ifndef BLOCK_H
#define BLOCK_H

#include <string>

// Represents an axis-aligned rectangular prism ("block") in the model.
// Coordinates (x,y,z) are absolute in the global grid.
// Offsets (x_offset,y_offset,z_offset) are indices within the local sub-volume
// used by the BlockGrowth algorithm (i.e., model_slices).
class Block {
public:
  // Absolute position in the global model grid
  int x, y, z;

  // Offsets within the parent/model_slices buffer (local indexing)
  int x_offset, y_offset, z_offset;

  // Size (dimensions) of the block
  int width, height, depth;

  // Volume of the block
  int volume;

  // Absolute end coordinates (exclusive)
  int x_end, y_end, z_end;

  // Single-character tag (equivalent to Python dtype 'U1' element)
  char tag;

  // Construct a block at (x,y,z) of size (w,h,d), with tag.
  // Optional local offsets default to 0.
  Block(int x, int y, int z, int w, int h, int d, char tag, int x_off = 0,
        int y_off = 0, int z_off = 0);

  // Adjust dimensions and keep end coordinates consistent
  void set_width(int w);
  void set_height(int h);
  void set_depth(int d);

  // Print in the exact Python format:
  //   x,y,z,width,height,depth,label
  // (label is looked up by BlockGrowth and passed in here)
  void print_block(const std::string& label) const;
};

#endif // BLOCK_H