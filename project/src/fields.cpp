// Функции доступа к полям события (занятие 1.3).
//
// Как каждая функция сообщает об ошибке и почему:
//
//   FindField         -> nullptr. Поля может не быть законно: у file_write
//                        нет domain, и спросить про него — обычный вопрос.
//   GetRequiredField  -> throw std::invalid_argument. Поле объявлено
//                        обязательным, его нет — журнал нарушил формат,
//                        и продолжать на пустой строке значит спрятать ошибку.
//   GetIntField (out) -> false. Значение приходит извне, "size=абв" или
//                        переполнение — битые данные, а не ошибка программы.
//   GetIntField (fallback) -> значение по умолчанию. Для полей, отсутствие
//                        которых осмысленно, вызывающему проверять нечего.
//   Предикаты         -> просто false. Вопрос о событии — всегда да или нет,
//                        поэтому они не бросают и не печатают.

#include "fields.h"

#include <cctype>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>

#include "event.h"

namespace nano_edr {

const std::string* FindField(const Event& event, const std::string& key) {
    for (const Field& field : event.fields) {
        if (field.key == key) {
            return &field.value;
        }
    }
    return nullptr;
}

const std::string& GetRequiredField(const Event& event, const std::string& key) {
    const std::string* value = FindField(event, key);
    if (value == nullptr) {
        throw std::invalid_argument("нет обязательного поля: " + key);
    }
    return *value;
}

bool GetIntField(const Event& event, const std::string& key, uint64_t* out) {
    const std::string* text = FindField(event, key);
    if (text == nullptr || text->empty()) {
        return false;
    }

    uint64_t result = 0;
    for (char c : *text) {
        if (c < '0' || c > '9') {
            return false;
        }
        uint64_t digit = static_cast<uint64_t>(c - '0');

        if (result > (UINT64_MAX - digit) / 10) {
            return false;
        }
        result = result * 10 + digit;
    }

    *out = result;
    return true;
}

uint64_t GetIntField(const Event& event, const std::string& key,
                     uint64_t fallback) {
    uint64_t value = 0;
    if (GetIntField(event, key, &value)) {
        return value;
    }
    return fallback;
}

bool IsProcessStart(const Event& event) {
    return event.type == "process_start";
}

bool IsFileWrite(const Event& event) {
    return event.type == "file_write";
}

bool IsNetConnect(const Event& event) {
    return event.type == "net_connect";
}

bool PathEndsWith(const Event& event, const std::string& suffix) {
    const std::string* path = FindField(event, "path");
    if (path == nullptr) {
        return false;
    }

    std::string normal_path = NormalizePath(*path);
    std::string normal_suffix = NormalizePath(suffix);

    if (normal_suffix.size() > normal_path.size()) {
        return false;
    }
    return normal_path.substr(normal_path.size() - normal_suffix.size()) == normal_suffix;
}

bool CommandLineContains(const Event& event, const std::string& needle) {
    const std::string* cmdline = FindField(event, "cmdline");
    if (cmdline == nullptr) {
        return false;
    }
    return NormalizePath(*cmdline).find(NormalizePath(needle)) != std::string::npos;
}

std::string NormalizePath(const std::string& path) {
    std::string lower;
    for (char c : path) {
        if (c == '/') {
            lower.push_back('\\');
        } else {
            lower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
        }
    }

    std::string expanded;
    std::size_t i = 0;
    while (i < lower.size()) {
        if (lower.substr(i, 6) == "%temp%") {
            expanded += "\\appdata\\local\\temp";
            i += 6;
        } else if (lower.substr(i, 5) == "%tmp%") {
            expanded += "\\appdata\\local\\temp";
            i += 5;
        } else {
            expanded.push_back(lower[i]);
            ++i;
        }
    }

    std::string result;
    for (char c : expanded) {
        if (c == '\\' && !result.empty() && result[result.size() - 1] == '\\') {
            continue;
        }
        result.push_back(c);
    }
    return result;
}

}
