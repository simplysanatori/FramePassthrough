#pragma once

// Problem reports: the text saved after a crash or a fatal error, and the
// Sentry envelope --send-report posts. SteamVR-free so the tests can reach it.
// Nothing here touches the network; see docs/PRIVACY.md.

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>

namespace fps {

// What identifies this machine and its user in a report.
struct ScrubInfo {
    std::string home; // $HOME, replaced with ~
    std::string user; // replaced with <user>
    std::string host; // replaced with <host>
};

// The home folder becomes ~, the user name <user> and the host name <host>
// (whole words only), IPv4 / IPv6 addresses <ip> and MAC addresses <mac>.
std::string scrub(std::string_view text, const ScrubInfo& info);
std::string scrub_network(std::string_view text);

// The last `count` lines of `text`.
std::string tail_lines(std::string_view text, std::size_t count);

const char* signal_name(int signal);

// https://<key>@<host>/<project>
struct Dsn {
    std::string key;
    std::string host;
    std::string project;
    std::string envelope_url() const;
};
std::optional<Dsn> parse_dsn(const std::string& text);

// The "Key: value" lines at the top of a saved report, up to the first blank line.
std::map<std::string, std::string> report_header(std::string_view report);

// One Sentry envelope: the event (reason, release, tags) and the report
// itself as the attachment report.txt. `event_id` is 32 hex digits.
std::string build_envelope(const std::string& report, const std::string& dsn_text,
    const std::string& event_id, std::int64_t unix_time, const std::string& sdk_version,
    const std::string& environment);

} // namespace fps
