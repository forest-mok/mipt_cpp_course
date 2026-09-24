#include <cstdio>
#include <fstream>
#include <print>
#include <string>
#include <vector>
#include <utility>

int main(int argc, char** argv) {

    bool quiet = false;
    std::string path;

    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--quiet") {
            quiet = true;
        } else {
            path = std::string(argv[i]);
        }

    }
    
    if (path.empty()) {
        std::print(stderr, "использование: nano-edr <журнал.log> [--quiet]\n");
        return 2;
    }

    std::ifstream log(path);

    
    if (!log) {
        std::print(stderr, "не удалось открыть журнал: {}\n", path);
        return 2;
    }

    long long lines = 0;
    long long no_sign = 0;
    std::string line;

    std::vector<std::string> signs;

    signs.push_back("wscript.exe");
    signs.push_back(".locked");
    signs.push_back("certutil.exe");
    signs.push_back("\\Startup\\");

    std::vector<std::pair<std::string, int>> typeCount;
    
    while (std::getline(log, line)) {
        
        ++lines;
        
        size_t ind = line.find_first_not_of(" \t");

        if (ind == std::string::npos || line[ind] == '#' || line[ind] == ';') {
            ++no_sign;
            continue;
        }
        
        size_t pos_type = line.find("type=");
        if (pos_type != std::string::npos) {

            size_t start_num = pos_type + 5;
            size_t end_num = line.find_first_of(" \t\r", start_num);

            if (end_num == std::string::npos) {
                end_num = line.size(); 
            }

            std::string type = line.substr(start_num, end_num - start_num);

            bool found_type = false;

            for (auto& p : typeCount) {
                if (p.first == type) {
                    p.second += 1;
                    found_type = true;
                    break;
                }
            }
            if (!found_type) {
                typeCount.push_back({type,1});
            }
            
        }
        for (const auto& war : signs) {
            if (line.find(war) != std::string::npos) {
                std::print ("[DETECT] строка {}, признак {}: {}\n", lines, war, line);
            }
        }   
    }

    long long all_sign = lines - no_sign;
    
    if (!quiet) {

        std::print("Общее число событий: {}\n", all_sign);

        for (const auto& p : typeCount) {
            std::print ("Тип: {}, Количество: {}\n", p.first, p.second); 
        }
    }
    
    return 0;
}
