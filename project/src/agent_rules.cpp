#include "agent_rules.h"

#include <cstddef>
#include <string>

#include "event.h"
#include "fields.h"
#include "rules.h"

namespace nano_edr {

namespace {

bool EndsWith(const std::string& text, const std::string& suffix) {
    if (suffix.size() > text.size()) {
        return false;
    }
    return text.substr(text.size() - suffix.size()) == suffix;
}

bool ImageEndsWith(const Event& event, const std::string& name) {
    return EndsWith(NormalizePath(GetRequiredField(event, "image")), name);
}

const std::string* WrittenPath(const Event& event) {
    if (event.type == "file_create" || IsFileWrite(event)) {
        return FindField(event, "path");
    }
    if (event.type == "file_move") {
        return FindField(event, "to");
    }
    return nullptr;
}

bool ScriptHostFromTemp(const Event& event) {
    if (!IsProcessStart(event)) {
        return false;
    }
    if (!ImageEndsWith(event, "\\wscript.exe") && !ImageEndsWith(event, "\\cscript.exe")) {
        return false;
    }
    return CommandLineContains(event, "\\appdata\\local\\temp\\") ||
           CommandLineContains(event, "\\windows\\temp\\");
}

bool LolbinDownload(const Event& event) {
    if (!IsProcessStart(event)) {
        return false;
    }
    if (!ImageEndsWith(event, "\\certutil.exe") && !ImageEndsWith(event, "\\bitsadmin.exe")) {
        return false;
    }
    return CommandLineContains(event, "urlcache") || CommandLineContains(event, "transfer") ||
           CommandLineContains(event, "http:") || CommandLineContains(event, "https:");
}

bool HiddenPowershell(const Event& event) {
    if (!IsProcessStart(event)) {
        return false;
    }
    if (!ImageEndsWith(event, "\\powershell.exe") && !ImageEndsWith(event, "\\pwsh.exe")) {
        return false;
    }
    return CommandLineContains(event, "-w hidden") ||
           CommandLineContains(event, "-windowstyle hidden") ||
           CommandLineContains(event, "-enc") ||
           CommandLineContains(event, "-encodedcommand");
}

bool AutostartWrite(const Event& event) {
    const std::string* path = WrittenPath(event);
    if (path == nullptr) {
        return false;
    }
    return NormalizePath(*path).find("\\start menu\\programs\\startup\\") != std::string::npos;
}

bool RansomExtension(const Event& event) {
    const std::string* path = WrittenPath(event);
    if (path == nullptr) {
        return false;
    }
    return EndsWith(NormalizePath(*path), ".locked");
}

constexpr Rule kRules[] = {
    {"script_host_from_temp", ScriptHostFromTemp, Severity::kHigh},
    {"lolbin_download", LolbinDownload, Severity::kHigh},
    {"hidden_powershell", HiddenPowershell, Severity::kMedium},
    {"autostart_write", AutostartWrite, Severity::kHigh},
    {"ransom_extension", RansomExtension, Severity::kCritical},
};

}

const Rule* AgentRules() {
    return kRules;
}

size_t AgentRuleCount() {
    return sizeof(kRules) / sizeof(kRules[0]);
}

}
