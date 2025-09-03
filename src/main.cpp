#include "block_model.h"
#include <iostream>

int main() {
  std::ios::sync_with_stdio(false);
  std::cin.tie(nullptr);

  BlockModel bm;
  bm.read_specification();
  bm.read_tag_table();
  bm.read_model();
  return 0;
} // Test comment
// Test comment for pre-commit hook
