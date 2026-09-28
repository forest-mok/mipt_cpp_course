#include "parse.h"
#include <cstddef>
#include <string>
#include "event.h"

namespace nano_edr {

namespace {

bool IsSpace(char c) {
    return c == ' ' || c == '\t';
}

} 

bool IsBlankOrComment(const std::string* line) {
    std::size_t i = 0;
    while (i < line->size() && IsSpace((*line)[i])) {
        ++i;
    }

    if (i == line->size()) {
        return true;
    }

    return (*line)[i] == '#' || (*line)[i] == ';';
}

bool ParseEventLine(const std::string* line, Event* out) {

    if (IsBlankOrComment(line)) {
        return false;
    }

    bool has_ts = false;
    bool has_type = false;
    bool has_pid = false;

    std::size_t n = line->size();
    std::size_t i = 0;

    while (i < n) {

        if (IsSpace((*line)[i])) {
            ++i;
            continue;
        }

        std::size_t key_start = i;
        while (i < n && (*line)[i] != '=' && !IsSpace((*line)[i])) {
            ++i;
        }
        if (i == n || (*line)[i] != '=') {
            return false;  
        }
        if (i == key_start) {
            return false; 
        }
        std::string key = line->substr(key_start, i - key_start);
        ++i; 

        std::string value;
        if (i < n && (*line)[i] == '"') {
            ++i;  
            std::size_t value_start = i;
            while (i < n && (*line)[i] != '"') {
                ++i;
            }
            if (i == n) {
                return false;
            }
            value = line->substr(value_start, i - value_start);
            ++i;  

            if (i < n && !IsSpace((*line)[i])) {
                return false; 
            }
        } else {

            std::size_t value_start = i;
            while (i < n && !IsSpace((*line)[i])) {
                ++i;
            }
            value = line->substr(value_start, i - value_start);
        }

        if (key == "ts" && !has_ts) {
            out->ts = value;
            has_ts = true;
        } else if (key == "type" && !has_type) {
            out->type = value;
            has_type = true;
        } else if (key == "pid" && !has_pid) {
            out->pid = value;
            has_pid = true;
        } else {
            out->fields.push_back(Field{key, value});
        }
    }

    return has_ts && has_type;
}

}  
