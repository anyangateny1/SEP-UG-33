#include <iostream>
#include "block_model.h"

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    BlockModel bm;
    bm.read_specification();
    bm.read_tag_table();
    bm.read_model();
    return 0;
}