#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cassert>
#include <set>
#include <algorithm>
#include "block_model.h"

// Test that multi-threaded compression produces identical results to single-threaded
class ThreadingTest {
public:
    static void run_all_tests() {
        std::cout << "Running threading tests...\n";
        
        test_output_consistency("tests/data/case1.txt");
        test_output_consistency("tests/data/case2.txt");
        test_thread_safety();
        
        std::cout << "All threading tests passed!\n";
    }

private:
    static void test_output_consistency(const std::string& test_file) {
        std::cout << "Testing output consistency for " << test_file << "...\n";
        
        // Get single-threaded output
        std::string single_threaded_output = run_compression(test_file, false, 0);
        
        // Get multi-threaded output with different thread counts
        std::vector<size_t> thread_counts = {2, 4, 8};
        
        for (size_t threads : thread_counts) {
            std::string multi_threaded_output = run_compression(test_file, true, threads);
            
            // Parse outputs into sets of blocks for comparison
            auto single_blocks = parse_blocks(single_threaded_output);
            auto multi_blocks = parse_blocks(multi_threaded_output);
            
            // Check that block counts match
            if (single_blocks.size() != multi_blocks.size()) {
                std::cerr << "ERROR: Block count mismatch for " << threads << " threads\n";
                std::cerr << "Single-threaded: " << single_blocks.size() << " blocks\n";
                std::cerr << "Multi-threaded: " << multi_blocks.size() << " blocks\n";
                assert(false);
            }
            
            // Check that all blocks match (order independent)
            if (single_blocks != multi_blocks) {
                std::cerr << "ERROR: Block content mismatch for " << threads << " threads\n";
                std::cerr << "Single-threaded output:\n" << single_threaded_output << "\n";
                std::cerr << "Multi-threaded output:\n" << multi_threaded_output << "\n";
                assert(false);
            }
            
            std::cout << "✓ " << threads << " threads: output matches single-threaded\n";
        }
        
        // Test auto thread count
        std::string auto_threaded_output = run_compression(test_file, true, 0);
        auto auto_blocks = parse_blocks(auto_threaded_output);
        auto single_blocks = parse_blocks(single_threaded_output);
        
        if (single_blocks != auto_blocks) {
            std::cerr << "ERROR: Auto thread count output mismatch\n";
            assert(false);
        }
        
        std::cout << "✓ Auto thread count: output matches single-threaded\n";
    }
    
    static void test_thread_safety() {
        std::cout << "Testing thread safety with multiple runs...\n";
        
        const std::string test_file = "tests/data/case1.txt";
        std::string reference_output = run_compression(test_file, true, 4);
        
        // Run multiple times to check for race conditions
        for (int i = 0; i < 10; ++i) {
            std::string current_output = run_compression(test_file, true, 4);
            
            auto reference_blocks = parse_blocks(reference_output);
            auto current_blocks = parse_blocks(current_output);
            
            if (reference_blocks != current_blocks) {
                std::cerr << "ERROR: Thread safety issue detected on run " << i + 1 << "\n";
                std::cerr << "Reference output:\n" << reference_output << "\n";
                std::cerr << "Current output:\n" << current_output << "\n";
                assert(false);
            }
        }
        
        std::cout << "✓ Thread safety test passed (10 runs)\n";
    }
    
    static std::string run_compression(const std::string& test_file, bool multithreaded, size_t thread_count) {
        std::ifstream file(test_file);
        if (!file.is_open()) {
            throw std::runtime_error("Could not open test file: " + test_file);
        }
        
        std::streambuf* orig = std::cin.rdbuf();
        std::cin.rdbuf(file.rdbuf());
        
        std::string result;
        
        try {
            BlockModel bm;
            if (!multithreaded) {
                bm.enable_multithreading(false);
            } else if (thread_count > 0) {
                bm.set_thread_count(thread_count);
            }
            
            // Capture output
            std::streambuf* cout_orig = std::cout.rdbuf();
            std::ostringstream output;
            std::cout.rdbuf(output.rdbuf());
            
            bm.read_specification();
            bm.read_tag_table();
            bm.read_model();
            
            std::cout.rdbuf(cout_orig);
            result = output.str();
            
        } catch (const std::exception& e) {
            std::cin.rdbuf(orig);
            file.close();
            throw;
        }
        
        std::cin.rdbuf(orig);
        file.close();
        
        return result;
    }
    
    static std::set<std::string> parse_blocks(const std::string& output) {
        std::set<std::string> blocks;
        std::istringstream stream(output);
        std::string line;
        
        while (std::getline(stream, line)) {
            if (!line.empty()) {
                blocks.insert(line);
            }
        }
        
        return blocks;
    }
};

int main() {
    std::cout << "=== Threading Test Suite ===\n";
    
    try {
        ThreadingTest::run_all_tests();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Threading test suite failed with exception: " << e.what() << "\n";
        return 1;
    }
}
