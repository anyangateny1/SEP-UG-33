#include <iostream>
#include <cstring>
#include "block_model.h"

void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [options]\n";
    std::cout << "Options:\n";
    std::cout << "  --threads N        Use N threads for parallel processing\n";
    std::cout << "  --single-threaded  Use single-threaded processing (default)\n";
    std::cout << "  --multi-threaded   Enable multi-threading with auto thread count\n";
    std::cout << "  --help             Show this help message\n";
}

int main(int argc, char* argv[]) {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    BlockModel bm; // Single-threaded by default for optimal speed
    bool multithreading_explicitly_enabled = false;
    size_t thread_count = 0;

    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (std::strcmp(argv[i], "--multi-threaded") == 0) {
            multithreading_explicitly_enabled = true;
        } else if (std::strcmp(argv[i], "--single-threaded") == 0) {
            multithreading_explicitly_enabled = false; // Override default
        } else if (std::strcmp(argv[i], "--threads") == 0) {
            if (i + 1 < argc) {
                thread_count = std::atoi(argv[++i]);
                multithreading_explicitly_enabled = true;
            } else {
                std::cerr << "Error: --threads requires a number\n";
                return 1;
            }
        } else {
            std::cerr << "Error: Unknown option " << argv[i] << "\n";
            print_usage(argv[0]);
            return 1;
        }
    }

    // Configure threading (single-threaded by default for optimal performance)
    if (multithreading_explicitly_enabled) {
        bm.enable_multithreading(true);
        if (thread_count > 0) {
            bm.set_thread_count(thread_count);
        }
    }

    try {
        bm.read_specification();
        bm.read_tag_table();
        bm.read_model();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}