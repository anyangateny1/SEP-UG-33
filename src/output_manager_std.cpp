#include "output_manager_std.h"
#include <iostream>

OutputManagerStd::OutputManagerStd() 
    : multithreaded_mode(false), next_expected_index(0) {
}

void OutputManagerStd::add_block_result(size_t order_index, const std::string& block_output) {
    std::lock_guard<std::mutex> lock(output_mutex);
    ordered_results[order_index] = block_output;
    
    if (!multithreaded_mode) {
        // In single-threaded mode, print immediately
        std::cout << block_output;
    } else {
        // In multi-threaded mode, try to print any ready results
        print_available_results();
    }
}

void OutputManagerStd::print_all_results() {
    std::lock_guard<std::mutex> lock(output_mutex);
    
    // Print any remaining results in order
    for (const auto& pair : ordered_results) {
        if (pair.first >= next_expected_index) {
            std::cout << pair.second;
        }
    }
    
    // Clear the results
    ordered_results.clear();
    next_expected_index = 0;
}

void OutputManagerStd::set_multithreaded(bool enable) {
    std::lock_guard<std::mutex> lock(output_mutex);
    multithreaded_mode = enable;
}

void OutputManagerStd::direct_print(const std::string& output) {
    if (!multithreaded_mode) {
        std::cout << output;
    } else {
        std::lock_guard<std::mutex> lock(output_mutex);
        std::cout << output;
    }
}

void OutputManagerStd::print_available_results() {
    // This should be called with output_mutex already locked
    auto it = ordered_results.find(next_expected_index);
    while (it != ordered_results.end()) {
        std::cout << it->second;
        ordered_results.erase(it);
        ++next_expected_index;
        it = ordered_results.find(next_expected_index);
    }
}

