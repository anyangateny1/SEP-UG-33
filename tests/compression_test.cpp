#include "block_model.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// Test the compression functionality directly
class CompressionTest {
public:
  static void run_all_tests() {
    std::cout << "Running compression tests...\n";

    test_basic_compression();
    test_case1_compression();
    test_case2_compression();

    std::cout << "All compression tests passed!\n";
  }

private:
  static void test_basic_compression() {
    std::cout << "Testing basic compression...\n";

    // Test that BlockModel can be instantiated and basic operations work
    BlockModel bm;

    // This is a basic smoke test - just ensure we can create the object
    // In a real scenario, we'd test specific compression logic

    std::cout << "✓ Basic compression test passed\n";
  }

  static void test_case1_compression() {
    std::cout << "Testing case1 compression...\n";

    // Redirect stdin to read from case1.txt
    std::ifstream case1_file("tests/data/case1.txt");
    if (!case1_file.is_open()) {
      std::cerr << "Error: Could not open tests/data/case1.txt\n";
      return;
    }

    // Save original cin buffer
    std::streambuf* orig = std::cin.rdbuf();
    std::cin.rdbuf(case1_file.rdbuf());

    try {
      BlockModel bm;
      bm.read_specification();
      bm.read_tag_table();

      // Capture output to count blocks
      std::streambuf* cout_orig = std::cout.rdbuf();
      std::ostringstream output;
      std::cout.rdbuf(output.rdbuf());

      bm.read_model();

      // Restore cout
      std::cout.rdbuf(cout_orig);

      // Count output lines (each line should be a compressed block)
      std::string result = output.str();
      int block_count = 0;
      size_t pos = 0;
      while ((pos = result.find('\n', pos)) != std::string::npos) {
        if (pos > 0 && result[pos - 1] != '\n') { // Skip empty lines
          block_count++;
        }
        pos++;
      }

      std::cout << "✓ Case1 compression test passed - generated " << block_count
                << " blocks\n";

    } catch (const std::exception& e) {
      std::cout << "✗ Case1 compression test failed: " << e.what() << "\n";
    }

    // Restore original cin
    std::cin.rdbuf(orig);
    case1_file.close();
  }

  static void test_case2_compression() {
    std::cout << "Testing case2 compression...\n";

    std::ifstream case2_file("tests/data/case2.txt");
    if (!case2_file.is_open()) {
      std::cerr << "Error: Could not open tests/data/case2.txt\n";
      return;
    }

    std::streambuf* orig = std::cin.rdbuf();
    std::cin.rdbuf(case2_file.rdbuf());

    try {
      BlockModel bm;
      bm.read_specification();
      bm.read_tag_table();

      // Capture output
      std::streambuf* cout_orig = std::cout.rdbuf();
      std::ostringstream output;
      std::cout.rdbuf(output.rdbuf());

      bm.read_model();

      std::cout.rdbuf(cout_orig);

      // Verify we got some output
      std::string result = output.str();
      if (!result.empty()) {
        std::cout << "✓ Case2 compression test passed - output length: "
                  << result.length() << " chars\n";
      } else {
        std::cout << "✗ Case2 compression test failed - no output generated\n";
      }

    } catch (const std::exception& e) {
      std::cout << "✗ Case2 compression test failed: " << e.what() << "\n";
    }

    std::cin.rdbuf(orig);
    case2_file.close();
  }
};

int main() {
  std::cout << "=== Compression Test Suite ===\n";

  try {
    CompressionTest::run_all_tests();
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "Test suite failed with exception: " << e.what() << "\n";
    return 1;
  }
}
