#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <vector>
#include <string>
#include <cassert>
#include "block_model.h"

class PerformanceBenchmark {
public:
    static void run_benchmark(const std::string& test_file) {
        std::cout << "=== Performance Benchmark ===\n";
        std::cout << "Test file: " << test_file << "\n";
        
        // Benchmark single-threaded
        auto single_threaded_time = benchmark_single_threaded(test_file);
        std::cout << "Single-threaded time: " << single_threaded_time << " ms\n";
        
        // Benchmark multi-threaded with different thread counts
        std::vector<size_t> thread_counts = {2, 4, 8};
        
        for (size_t threads : thread_counts) {
            auto multi_threaded_time = benchmark_multi_threaded(test_file, threads);
            std::cout << "Multi-threaded (" << threads << " threads) time: " << multi_threaded_time << " ms\n";
            
            double speedup = static_cast<double>(single_threaded_time) / multi_threaded_time;
            std::cout << "Speedup: " << speedup << "x\n";
        }
        
        // Test auto thread count
        auto auto_threaded_time = benchmark_multi_threaded(test_file, 0);
        std::cout << "Multi-threaded (auto) time: " << auto_threaded_time << " ms\n";
        double auto_speedup = static_cast<double>(single_threaded_time) / auto_threaded_time;
        std::cout << "Auto speedup: " << auto_speedup << "x\n";
        
        std::cout << "=== Benchmark Complete ===\n";
    }

private:
    static long long benchmark_single_threaded(const std::string& test_file) {
        std::ifstream file(test_file);
        if (!file.is_open()) {
            throw std::runtime_error("Could not open test file: " + test_file);
        }
        
        std::streambuf* orig = std::cin.rdbuf();
        std::cin.rdbuf(file.rdbuf());
        
        auto start = std::chrono::high_resolution_clock::now();
        
        try {
            BlockModel bm;
            bm.enable_multithreading(false); // Force single-threaded for baseline
            
            // Capture output to avoid I/O affecting timing
            std::streambuf* cout_orig = std::cout.rdbuf();
            std::ostringstream output;
            std::cout.rdbuf(output.rdbuf());
            
            bm.read_specification();
            bm.read_tag_table();
            bm.read_model();
            
            std::cout.rdbuf(cout_orig);
            
        } catch (const std::exception& e) {
            std::cin.rdbuf(orig);
            file.close();
            throw;
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        
        std::cin.rdbuf(orig);
        file.close();
        
        return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    }
    
    static long long benchmark_multi_threaded(const std::string& test_file, size_t thread_count) {
        std::ifstream file(test_file);
        if (!file.is_open()) {
            throw std::runtime_error("Could not open test file: " + test_file);
        }
        
        std::streambuf* orig = std::cin.rdbuf();
        std::cin.rdbuf(file.rdbuf());
        
        auto start = std::chrono::high_resolution_clock::now();
        
        try {
            BlockModel bm;
            // Multi-threading enabled by default, just set thread count if specified
            if (thread_count > 0) {
                bm.set_thread_count(thread_count);
            }
            
            // Capture output to avoid I/O affecting timing
            std::streambuf* cout_orig = std::cout.rdbuf();
            std::ostringstream output;
            std::cout.rdbuf(output.rdbuf());
            
            bm.read_specification();
            bm.read_tag_table();
            bm.read_model();
            
            std::cout.rdbuf(cout_orig);
            
        } catch (const std::exception& e) {
            std::cin.rdbuf(orig);
            file.close();
            throw;
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        
        std::cin.rdbuf(orig);
        file.close();
        
        return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    }
};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <test_file>\n";
        std::cerr << "Example: " << argv[0] << " tests/data/case1.txt\n";
        return 1;
    }
    
    try {
        PerformanceBenchmark::run_benchmark(argv[1]);
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Benchmark failed: " << e.what() << "\n";
        return 1;
    }
}
