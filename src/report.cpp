#include "report.hpp"

#include <json.hpp>
#include <algorithm>
#include <cctype>
#include <csignal>
#include <ctime>
#include <vector>

namespace fps {
namespace {

using json = nlohmann::json;

bool word_char(char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '_'; }
bool hex_char(char c) { return std::isxdigit(static_cast<unsigned char>(c)) != 0; }

// Replaces `needle` where it stands as a whole word, or (for a path) where
// it is followed by a non-word character.
std::string replace_token(std::string_view text, const std::string& needle, const std::string& with, bool path) {
    if (needle.empty()) return std::string(text);
    std::string out;
    std::size_t at = 0;
    while (true) {
        const auto found = text.find(needle, at);
        if (found == std::string_view::npos) break;
        const auto end = found + needle.size();
        const bool start_ok = path || found == 0 || !word_char(text[found - 1]);
        const bool end_ok = end == text.size() || !word_char(text[end]) || (path && text[end] == '/');
        out.append(text.substr(at, found - at));
        if (start_ok && end_ok) out += with;
        else out.append(needle);
        at = end;
    }
    out.append(text.substr(at));
    return out;
}

bool is_ipv4(std::string_view s) {
    int parts = 0;
    std::size_t at = 0;
    while (true) {
        const auto dot = s.find('.', at);
        const auto part = s.substr(at, dot == std::string_view::npos ? std::string_view::npos : dot - at);
        if (part.empty() || part.size() > 3 || !std::all_of(part.begin(), part.end(), [](char c) { return c >= '0' && c <= '9'; }))
            return false;
        if (std::stoi(std::string(part)) > 255) return false;
        ++parts;
        if (dot == std::string_view::npos) break;
        at = dot + 1;
    }
    return parts == 4;
}

bool is_mac(std::string_view s) {
    if (s.size() != 17 || (s[2] != ':' && s[2] != '-')) return false;
    for (std::size_t i = 0; i < s.size(); ++i)
        if (i % 3 == 2 ? s[i] != s[2] : !hex_char(s[i])) return false;
    return true;
}

// Needs "::" or eight groups, so clock times such as 12:34:56 stay.
bool is_ipv6(std::string_view s) {
    const auto colons = std::count(s.begin(), s.end(), ':');
    if (colons < 2 || colons > 7 || (colons != 7 && s.find("::") == std::string_view::npos)) return false;
    std::size_t group = 0;
    for (char c : s) {
        if (c == ':') group = 0;
        else if (!hex_char(c) || ++group > 4) return false;
    }
    return std::any_of(s.begin(), s.end(), hex_char);
}

} // namespace

std::string scrub_network(std::string_view text) {
    const auto in_run = [](char c) { return hex_char(c) || c == '.' || c == ':' || c == '-'; };
    std::string out;
    std::size_t i = 0;
    while (i < text.size()) {
        if (!in_run(text[i]) || (i > 0 && (word_char(text[i - 1]) || text[i - 1] == '.'))) {
            out.push_back(text[i++]);
            continue;
        }
        auto j = i;
        while (j < text.size() && in_run(text[j])) ++j;
        while (j > i && (text[j - 1] == '.' || text[j - 1] == ':' || text[j - 1] == '-')) --j;
        if (j == i || (j < text.size() && word_char(text[j]))) {
            // Part of a longer word: copy it up to the end of that word.
            auto k = std::max(j, i + 1);
            while (k < text.size() && (in_run(text[k]) || word_char(text[k]))) ++k;
            out.append(text.substr(i, k - i));
            i = k;
            continue;
        }
        const auto run = text.substr(i, j - i);
        const auto colon = run.rfind(':');
        if (is_mac(run)) out += "<mac>";
        else if (is_ipv4(run)) out += "<ip>";
        else if (colon != std::string_view::npos && is_ipv4(run.substr(0, colon))) { out += "<ip>"; out.append(run.substr(colon)); }
        else if (is_ipv6(run)) out += "<ip>";
        else out.append(run);
        i = j;
    }
    return out;
}

std::string scrub(std::string_view text, const ScrubInfo& info) {
    std::string out(text);
    if (info.home.size() > 1) out = replace_token(out, info.home, "~", true);
    if (info.user.size() >= 2) out = replace_token(out, info.user, "<user>", false);
    if (info.host.size() >= 2) out = replace_token(out, info.host, "<host>", false);
    return scrub_network(out);
}

std::string tail_lines(std::string_view text, std::size_t count) {
    if (count == 0) return {};
    auto end = text.size();
    if (end > 0 && text[end - 1] == '\n') --end;
    std::size_t lines = 0;
    auto at = end;
    while (at > 0) {
        if (text[at - 1] == '\n' && ++lines == count) break;
        --at;
    }
    return std::string(text.substr(at));
}

const char* signal_name(int signal) {
    switch (signal) {
    case SIGSEGV: return "SIGSEGV";
    case SIGABRT: return "SIGABRT";
    case SIGBUS: return "SIGBUS";
    case SIGILL: return "SIGILL";
    case SIGFPE: return "SIGFPE";
    default: return "signal";
    }
}

std::string Dsn::envelope_url() const { return "https://" + host + "/api/" + project + "/envelope/"; }

std::optional<Dsn> parse_dsn(const std::string& text) {
    const std::string scheme = "https://";
    if (text.rfind(scheme, 0) != 0) return std::nullopt;
    const auto at = text.find('@', scheme.size());
    if (at == std::string::npos) return std::nullopt;
    const auto slash = text.find('/', at + 1);
    if (slash == std::string::npos) return std::nullopt;
    Dsn dsn;
    dsn.key = text.substr(scheme.size(), at - scheme.size());
    if (const auto colon = dsn.key.find(':'); colon != std::string::npos) dsn.key.resize(colon); // legacy secret
    dsn.host = text.substr(at + 1, slash - at - 1);
    dsn.project = text.substr(slash + 1);
    while (!dsn.project.empty() && dsn.project.back() == '/') dsn.project.pop_back();
    const auto plain = [](const std::string& s, const char* extra) {
        return !s.empty() && std::all_of(s.begin(), s.end(), [&](char c) {
            return std::isalnum(static_cast<unsigned char>(c)) || std::string(extra).find(c) != std::string::npos;
        });
    };
    if (!plain(dsn.key, "") || !plain(dsn.host, ".-:") || !plain(dsn.project, "")) return std::nullopt;
    return dsn;
}

std::map<std::string, std::string> report_header(std::string_view report) {
    std::map<std::string, std::string> header;
    std::size_t at = 0;
    while (at < report.size()) {
        auto end = report.find('\n', at);
        if (end == std::string_view::npos) end = report.size();
        const auto line = report.substr(at, end - at);
        if (line.empty()) break;
        if (const auto colon = line.find(": "); colon != std::string_view::npos)
            header.emplace(std::string(line.substr(0, colon)), std::string(line.substr(colon + 2)));
        at = end + 1;
    }
    return header;
}

std::string build_envelope(const std::string& report, const std::string& dsn_text,
    const std::string& event_id, std::int64_t unix_time, const std::string& sdk_version,
    const std::string& environment) {
    const auto header = report_header(report);
    const auto field = [&](const char* key, const char* fallback = "") {
        const auto found = header.find(key);
        return found == header.end() ? std::string(fallback) : found->second;
    };
    const auto kind = field("Kind", "error");
    const auto reason = field("Reason", "Unknown problem");

    char sent_at[32];
    const auto seconds = static_cast<std::time_t>(unix_time);
    std::tm utc{};
    gmtime_r(&seconds, &utc);
    std::strftime(sent_at, sizeof(sent_at), "%Y-%m-%dT%H:%M:%SZ", &utc);

    json event = {
        {"event_id", event_id},
        {"timestamp", unix_time},
        {"platform", "native"},
        {"logger", "frame-passthrough-shortcuts"},
        {"level", kind == "manual" ? "warning" : "fatal"},
        {"release", field("Release", "framepassthrough@unknown")},
        {"environment", environment},
        {"tags", {{"report", kind}, {"command", field("Command")}, {"system", field("System")}}},
        // No user, no server name; Sentry is asked not to take the address.
        {"user", {{"geo", json::object()}}},
        {"sdk", {{"name", "framepassthrough"}, {"version", sdk_version}, {"settings", {{"infer_ip", "never"}}}}},
    };
    if (kind == "manual") {
        event["message"] = {{"formatted", reason.substr(0, 200)}};
    } else {
        const bool crash = kind == "crash";
        event["exception"] = {{"values", json::array({{
            {"type", crash ? field("Signal", "Crash") : std::string("Error")},
            {"value", reason},
            {"mechanism", {{"type", crash ? "signalhandler" : "generic"}, {"handled", false}}},
        }})}};
    }
    const auto dump = [](const json& value) { return value.dump(-1, ' ', false, json::error_handler_t::replace); };
    const auto event_text = dump(event);
    std::string envelope;
    envelope += dump({{"event_id", event_id}, {"sent_at", sent_at}, {"dsn", dsn_text}}) + "\n";
    envelope += dump({{"type", "event"}, {"length", event_text.size()}}) + "\n" + event_text + "\n";
    envelope += dump({{"type", "attachment"}, {"length", report.size()}, {"filename", "report.txt"},
        {"content_type", "text/plain"}}) + "\n" + report + "\n";
    return envelope;
}

} // namespace fps
