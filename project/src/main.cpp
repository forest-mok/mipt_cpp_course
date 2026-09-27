#include <charconv>
#include <cstddef>
#include <cstdio>
#include <fstream>
#include <print>
#include <string>
#include <utility>
#include <vector>

#include "event.h"
#include "event_list.h"
#include "parse.h"

int main(int argc, char** argv) {

    bool quiet = false;
    std::string path;
    std::size_t window_size = 64; 

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--quiet") {
            quiet = true;
        } else if (arg == "--window-size") {

            if (i + 1 >= argc) {
                std::print(stderr, "--window-size: не указано число\n");
                return 2;
            }
            ++i;
            std::string value = argv[i];

            std::from_chars_result result = std::from_chars(
                value.data(), value.data() + value.size(), window_size);

            if (result.ec != std::errc() || result.ptr != value.data() + value.size()) {
                std::print(stderr, "--window-size: не число: {}\n", value);
                return 2;
            }
        } else {
            path = arg;
        }
    }

    if (path.empty()) {
        std::print(stderr, "использование: nano-edr <журнал.log> [--quiet] [--window-size N]\n");
        return 2;
    }

    std::ifstream log(path);

    if (!log) {
        std::print(stderr, "не удалось открыть журнал: {}\n", path);
        return 2;
    }

    long long lines = 0;
    long long events = 0;
    std::string line;

    std::vector<std::string> signs;

    signs.push_back("wscript.exe");
    signs.push_back(".locked");
    signs.push_back("certutil.exe");
    signs.push_back("\\Startup\\");

    std::vector<std::pair<std::string, int>> typeCount;

    nano_edr::EventList window;
    window.capacity = window_size;

    while (std::getline(log, line)) {

        ++lines;

        if (nano_edr::IsBlankOrComment(&line)) {
            continue;
        }

        bool detected = false;
        for (const std::string& war : signs) {
            if (line.find(war) != std::string::npos) {
                std::print("[DETECT] строка {}, признак {}: {}\n", lines, war, line);
                detected = true;
            }
        }

        if (detected && !quiet) {

            std::size_t skip = 0;
            if (window.size > 2) {
                skip = window.size - 2;
            }

            std::size_t index = 0;
            for (const nano_edr::EventNode* it = window.head; it != nullptr; it = it->next) {
                if (index >= skip) {

                    std::print("[CTX] -{}: ts={} type={} pid={}\n",
                               window.size - index, it->event.ts, it->event.type, it->event.pid);
                }
                ++index;
            }
        }

        nano_edr::Event event;
        if (!nano_edr::ParseEventLine(&line, &event)) {
            continue;
        }

        ++events;

        bool found_type = false;
        for (std::pair<std::string, int>& p : typeCount) {
            if (p.first == event.type) {
                p.second += 1;
                found_type = true;
                break;
            }
        }
        if (!found_type) {
            typeCount.push_back({event.type, 1});
        }
        
        nano_edr::ListPushBack(&window, &event);
    }

    if (!quiet) {

        std::print("Общее число событий: {}\n", events);

        for (const std::pair<std::string, int>& p : typeCount) {
            std::print("Тип: {}, Количество: {}\n", p.first, p.second);
        }
    }

    return 0;
}
