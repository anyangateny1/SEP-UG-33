#include <bits/stdc++.h>
using namespace std;

// Dimensions match python script
static constexpr int WIDTH  = 65;
static constexpr int HEIGHT = 16;
static constexpr int DEPTH  = 5;

static inline string trim(const string& s) {
    size_t a = 0, b = s.size();
    while (a < b && isspace(static_cast<unsigned char>(s[a]))) ++a;
    while (b > a && isspace(static_cast<unsigned char>(s[b-1]))) --b;
    return s.substr(a, b - a);
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    // model[z][y][x]
    vector<vector<vector<char>>> model(
        DEPTH, vector<vector<char>>(HEIGHT, vector<char>(WIDTH, '\0'))
    );
    vector<vector<vector<bool>>> seen(
        DEPTH, vector<vector<bool>>(HEIGHT, vector<bool>(WIDTH, false))
    );

    // label -> char (same as Python label_table)
    unordered_map<string, char> label_table = {
        {"sea", 'o'},
        {"WA",  'w'},
        {"NT",  'n'},
        {"SA",  's'},
        {"QLD", 'q'},
        {"NSW", 'e'},
        {"VIC", 'v'},
        {"TAS", 't'}
    };

    string line;
    while (true) {
        if (!getline(cin, line)) break;     // EOF
        if (!line.empty() && line.back() == '\r') line.pop_back();
        line = trim(line);
        if (line.empty()) break;

        // Split by commas
        vector<string> parts;
        {
            string cur;
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
            cerr << "Invalid line (need 7 fields): " << line << "\n";
            continue;
        }

        int x   = stoi(parts[0]);
        int y   = stoi(parts[1]);
        int z   = stoi(parts[2]);
        int bw  = stoi(parts[3]);
        int bh  = stoi(parts[4]);
        int bd  = stoi(parts[5]);
        string label = parts[6];

        auto it = label_table.find(label);
        if (it == label_table.end()) {
            cerr << "Unknown label \"" << label << "\"; skipping line.\n";
            continue;
        }
        char tag = it->second;

        // Fill the block; warn if any voxel is written twice
        for (int dz = 0; dz < bd; ++dz) {
            int zz = z + dz;
            if (zz < 0 || zz >= DEPTH) continue; // or report error
            for (int dy = 0; dy < bh; ++dy) {
                int yy = y + dy;
                if (yy < 0 || yy >= HEIGHT) continue;
                for (int dx = 0; dx < bw; ++dx) {
                    int xx = x + dx;
                    if (xx < 0 || xx >= WIDTH) continue;

                    if (seen[zz][yy][xx]) {
                        cout << "(" << xx << ", " << yy << ", " << zz << ") appears twice\n";
                    }
                    seen[zz][yy][xx] = true;
                    model[zz][yy][xx] = tag;
                }
            }
        }
    }

    // Print the 3D model like the Python script:
    // each slice z -> HEIGHT rows of WIDTH chars, '-' for empty, blank line between slices
    for (int z = 0; z < DEPTH; ++z) {
        for (int y = 0; y < HEIGHT; ++y) {
            for (int x = 0; x < WIDTH; ++x) {
                char c = model[z][y][x];
                if (c == '\0') {
                    cout << '-';
                } else {
                    cout << c;
                }
            }
            cout << "\n";
        }
        cout << "\n";
    }

    return 0;
}