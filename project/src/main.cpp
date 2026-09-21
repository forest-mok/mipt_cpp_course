// Каркас агента: читает журнал событий построчно, считает события по типам
// и печатает детекты.
//
// Запуск:
//   nano-edr <журнал.log> [--quiet]
#include <cstdio>
#include <fstream>
#include <print>
#include <string>
#include <utility>
#include <vector>

int main(int argc, char** argv) {
    std::string log_path;
    bool quiet = false;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--quiet") {
            quiet = true;
        } else {
            log_path = arg;
        }
    }

    if (log_path.empty()) {
        std::print(stderr, "использование: nano-edr <журнал.log> [--quiet]\n");
        return 2;
    }

    std::ifstream log(log_path);
    if (!log) {
        std::print(stderr, "не удалось открыть журнал: {}\n", log_path);
        return 2;
    }

    std::vector<std::string> patterns = {
        "wscript.exe",
        ".locked",
        "certutil.exe",
        "\\Startup\\"
    };

    std::vector<std::pair<std::string, int>> type_counts;

    long long lines = 0;
    std::string line;

    while (std::getline(log, line)) {
        ++lines;

        auto pos = line.find_first_not_of(" \t");
        if (pos == std::string::npos || line[pos] == '#' || line[pos] == ';') {
            continue;
        }

        auto tpos = line.find("type=");
        std::string type;
        if (tpos != std::string::npos) {
            auto start = tpos + 5;
            auto end = line.find(' ', start);
            type = line.substr(start, end - start);
        }

        bool found = false;
        for (auto& tc : type_counts) {
            if (tc.first == type) {
                tc.second++;
                found = true;
                break;
            }
        }
        if (!found) {
            type_counts.push_back({type, 1});
        }

        for (const auto& pattern : patterns) {
            if (line.find(pattern) != std::string::npos) {
                std::print("[DETECT] строка {}, признак {}: {}\n", lines, pattern, line);
            }
        }
    }

    if (!quiet) {
        for (const auto& tc : type_counts) {
            std::print("{}: {}\n", tc.first, tc.second);
        }
    }

    return 0;
}
