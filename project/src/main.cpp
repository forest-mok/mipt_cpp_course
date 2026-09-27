#include <charconv>
#include <cstddef>
#include <cstdio>
#include <exception>
#include <fstream>
#include <print>
#include <string>
#include <utility>
#include <vector>

#include "agent_rules.h"
#include "event.h"
#include "event_list.h"
#include "parse.h"
#include "rules.h"

namespace {
struct Options {
    std::string path;
    bool quiet = false;
    std::size_t window_size = 64;
};

bool ParseArgs(int argc, char** argv, Options& options) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--quiet") {
            options.quiet = true;
        } else if (arg == "--window-size") {
            if (i + 1 >= argc) {
                std::print(stderr, "--window-size: не указано число\n");
                return false;
            }
            ++i;
            std::string value = argv[i];
            std::from_chars_result result = std::from_chars(
                value.data(), value.data() + value.size(), options.window_size);
            if (result.ec != std::errc() || result.ptr != value.data() + value.size()) {
                std::print(stderr, "--window-size: не число: {}\n", value);
                return false;
            }
        } else {
            options.path = arg;
        }
    }

    if (options.path.empty()) {
        std::print(stderr, "использование: nano-edr <журнал.log> [--quiet] [--window-size N]\n");
        return false;
    }
    return true;
}

void CountType(std::vector<std::pair<std::string, int>>& counts, const std::string& type) {
    for (std::pair<std::string, int>& item : counts) {
        if (item.first == type) {
            ++item.second;
            return;
        }
    }
    counts.push_back({type, 1});
}

void PrintContext(const nano_edr::EventList& window) {
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

void PrintSummary(long long events, const std::vector<std::pair<std::string, int>>& types) {
    std::print("Общее число событий: {}\n", events);
    for (const std::pair<std::string, int>& item : types) {
        std::print("Тип: {}, Количество: {}\n", item.first, item.second);
    }
}

int Run(const Options& options) {
    std::ifstream log(options.path);
    if (!log) {
        std::print(stderr, "не удалось открыть журнал: {}\n", options.path);
        return 2;
    }

    nano_edr::EventList window;
    window.capacity = options.window_size;

    long long events = 0;
    std::vector<std::pair<std::string, int>> types;
    std::string line;

    while (std::getline(log, line)) {
        if (nano_edr::IsBlankOrComment(&line)) {
            continue;
        }

        nano_edr::Event event;
        if (!nano_edr::ParseEventLine(&line, &event)) {
            continue;
        }

        ++events;
        CountType(types, event.type);

        std::size_t detects =
            nano_edr::CheckRules(event, nano_edr::AgentRules(), nano_edr::AgentRuleCount());
        if (detects > 0 && !options.quiet) {
            PrintContext(window);
        }

        nano_edr::ListPushBack(&window, &event);
    }

    if (!options.quiet) {
        PrintSummary(events, types);
    }
    return 0;
}

}

int main(int argc, char** argv) {
    try {
        Options options;
        if (!ParseArgs(argc, argv, options)) {
            return 2;
        }
        return Run(options);
    } catch (const std::exception& error) {
        std::print(stderr, "прогон оборван: {}\n", error.what());
        return 1;
    }
}
