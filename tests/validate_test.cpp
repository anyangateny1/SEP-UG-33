#include <cctype>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

// Dimensions match python script
static constexpr int WIDTH = 65;
static constexpr int HEIGHT = 16;
static constexpr int DEPTH = 5;

static inline std::string trim(const std::string& s) {
  size_t a = 0, b = s.size();
  while (a < b && std::isspace(static_cast<unsigned char>(s[a])))
    ++a;
  while (b > a && std::isspace(static_cast<unsigned char>(s[b - 1])))
    --b;
  return s.substr(a, b - a);
}

int main() {
  std::ios::sync_with_stdio(false);
  std::cin.tie(nullptr);

  // model[z][y][x]
  std::vector<std::vector<std::vector<char>>> model(
      DEPTH,
      std::vector<std::vector<char>>(HEIGHT, std::vector<char>(WIDTH, '\0')));
  std::vector<std::vector<std::vector<bool>>> seen(
      DEPTH,
      std::vector<std::vector<bool>>(HEIGHT, std::vector<bool>(WIDTH, false)));

  // label -> char (same as Python label_table)
  std::unordered_map<std::string, char> label_table = {
      {"sea", 'o'}, {"WA", 'w'},  {"NT", 'n'},  {"SA", 's'},
      {"QLD", 'q'}, {"NSW", 'e'}, {"VIC", 'v'}, {"TAS", 't'}};

  std::string line;
  while (true) {
    if (!std::getline(std::cin, line))
      break; // EOF
    if (!line.empty() && line.back() == '\r')
      line.pop_back();
    line = trim(line);
    if (line.empty())
      break;

    // Split by commas
    std::vector<std::string> parts;
    {
      std::string cur;
      for (char c : line) {
        if (c == ',') {
          parts.push_back(trim(cur));
          cur.clear();
        } else {
          cur.push_back(c);
        }
      }
      parts.push_back(trim(cur));
    }

    if (parts.size() != 7) {
      std::cerr << "Invalid line (need 7 fields): " << line << "\n";
      continue;
    }

    int x = std::stoi(parts[0]);
    int y = std::stoi(parts[1]);
    int z = std::stoi(parts[2]);
    int bw = std::stoi(parts[3]);
    int bh = std::stoi(parts[4]);
    int bd = std::stoi(parts[5]);
    std::string label = parts[6];

    auto it = label_table.find(label);
    if (it == label_table.end()) {
      std::cerr << "Unknown label \"" << label << "\"; skipping line.\n";
      continue;
    }
    char tag = it->second;

    // Fill the block; warn if any voxel is written twice
    for (int dz = 0; dz < bd; ++dz) {
      int zz = z + dz;
      if (zz < 0 || zz >= DEPTH)
        continue; // or report error
      for (int dy = 0; dy < bh; ++dy) {
        int yy = y + dy;
        if (yy < 0 || yy >= HEIGHT)
          continue;
        for (int dx = 0; dx < bw; ++dx) {
          int xx = x + dx;
          if (xx < 0 || xx >= WIDTH)
            continue;

          if (seen[zz][yy][xx]) {
            std::cout << "(" << xx << ", " << yy << ", " << zz
                      << ") appears twice\n";
          }
          seen[zz][yy][xx] = true;
          model[zz][yy][xx] = tag;
        }
      }
    }
  }

  // Print the 3D model like the Python script:
  // each slice z -> HEIGHT rows of WIDTH chars, '-' for empty, blank line
  // between slices
  for (int z = 0; z < DEPTH; ++z) {
    for (int y = 0; y < HEIGHT; ++y) {
      for (int x = 0; x < WIDTH; ++x) {
        char c = model[z][y][x];
        if (c == '\0') {
          std::cout << '-';
        } else {
          std::cout << c;
        }
      }
      std::cout << "\n";
    }
    std::cout << "\n";
  }

  return 0;
}