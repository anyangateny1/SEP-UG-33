#ifndef OUTPUT_MANAGER_STD_H
#define OUTPUT_MANAGER_STD_H

#include <mutex>
#include <vector>
#include <string>
#include <map>

// Thread-safe output manager using standard library (Windows compatible)
class OutputManagerStd {
public:
    OutputManagerStd();
    
    // Add a block result with its processing order index
    void add_block_result(size_t order_index, const std::string& block_output);
    
    // Print all results in order
    void print_all_results();
    
    // Enable/disable multi-threading mode
    void set_multithreaded(bool enable);
    
    // Direct print (for single-threaded mode)
    void direct_print(const std::string& output);

private:
    std::mutex output_mutex;
    std::map<size_t, std::string> ordered_results;
    bool multithreaded_mode;
    size_t next_expected_index;
    
    void print_available_results();
};

#endif // OUTPUT_MANAGER_STD_H

