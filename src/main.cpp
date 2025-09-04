#include <iostream>
#include <cstdlib>
#include <thread>
#ifdef USE_TBB
#include <tbb/global_control.h>
#endif
#include "block_model.h"
#include <iostream>

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    BlockModel bm;
    // Permanently use 4 threads
    unsigned int thread_limit = 4u;

    bm.set_num_threads(thread_limit);

#ifdef USE_TBB
    // Enforce TBB global thread cap for the duration of main
    tbb::global_control tbb_limit(tbb::global_control::max_allowed_parallelism, thread_limit);
#endif
    bm.read_specification();
    bm.read_tag_table();
    bm.read_model();
    return 0;
}
