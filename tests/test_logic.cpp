// Unit tests for the SteamVR-free logic: gate state machine, passthrough
// signal parsing and configuration. Run with ctest; no SteamVR needed.
#include "config.hpp"
#include "controller.hpp"
#include "gate.hpp"
#include "passthrough_state.hpp"

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <unistd.h>

namespace fs = std::filesystem;
using namespace fps;

namespace {

int failures = 0;
int checks = 0;

#define CHECK(condition)                                                                 \
    do {                                                                                 \
        ++checks;                                                                        \
        if (!(condition)) {                                                              \
            ++failures;                                                                  \
            std::cerr << __FILE__ << ":" << __LINE__ << ": CHECK failed: " #condition "\n"; \
        }                                                                                \
    } while (0)

template <typename F>
bool throws(F&& f) {
    try { f(); } catch (const std::exception&) { return true; }
    return false;
}

struct TempDir {
    fs::path path;
    TempDir() {
        char pattern[] = "/tmp/fps-test-XXXXXX";
        if (!::mkdtemp(pattern)) throw std::runtime_error("mkdtemp");
        path = pattern;
    }
    ~TempDir() { std::error_code e; fs::remove_all(path, e); }
};

void append(const fs::path& file, const std::string& text) {
    std::ofstream out(file, std::ios::app | std::ios::binary);
    out << text;
}

GateInputs inputs(Visibility v, bool game = false, bool disable_in_game = false) {
    GateInputs in;
    in.passthrough = v;
    in.game_running = game;
    in.disable_while_game_running = disable_in_game;
    return in;
}

void test_gate_open_closed() {
    InputGate gate;
    CHECK(gate.update(inputs(Visibility::Hidden), 1000));
    CHECK(!gate.open());
    CHECK(gate.reason() == GateReason::PassthroughHidden);
    CHECK(!gate.update(inputs(Visibility::Hidden), 1020)); // no change, no log
    CHECK(!gate.accept(GestureKind::DoublePress, 5000));
    CHECK(!gate.accept(GestureKind::Hold, 5000));

    CHECK(gate.update(inputs(Visibility::Visible), 2000));
    CHECK(gate.open());
    CHECK(gate.reason() == GateReason::PassthroughVisible);
    CHECK(gate.opened_at_ms() == 2000);

    CHECK(gate.update(inputs(Visibility::Hidden), 3000));
    CHECK(!gate.open());
}

void test_gate_unknown_fails_open() {
    InputGate gate;
    CHECK(gate.update(inputs(Visibility::Unknown), 0));
    CHECK(gate.open());
    CHECK(gate.reason() == GateReason::PassthroughUnknown);
    // Unknown -> visible keeps the gate open and does not restart the guard.
    CHECK(gate.update(inputs(Visibility::Visible), 5000)); // reason changed
    CHECK(gate.open());
    CHECK(gate.opened_at_ms() == 0);
}

void test_gate_mid_gesture_guard() {
    GateTimings timings;
    timings.double_press_guard_ms = 250;
    timings.hold_guard_ms = 1050;
    InputGate gate(timings);
    gate.update(inputs(Visibility::Hidden), 0);
    gate.update(inputs(Visibility::Visible), 10000);
    // A double press whose first press happened before opening completes
    // within double_press_delay of opening: rejected.
    CHECK(!gate.accept(GestureKind::DoublePress, 10000));
    CHECK(!gate.accept(GestureKind::DoublePress, 10249));
    CHECK(gate.accept(GestureKind::DoublePress, 10250));
    // A hold that began while closed fires < 1 s after opening: rejected.
    CHECK(!gate.accept(GestureKind::Hold, 10500));
    CHECK(!gate.accept(GestureKind::Hold, 11049));
    // A hold that began at or after opening fires >= 1 s later: accepted.
    CHECK(gate.accept(GestureKind::Hold, 11050));
    // Reopening restarts the guard.
    gate.update(inputs(Visibility::Hidden), 20000);
    gate.update(inputs(Visibility::Visible), 21000);
    CHECK(!gate.accept(GestureKind::DoublePress, 21100));
    CHECK(gate.accept(GestureKind::DoublePress, 21300));
    // Clock going backwards never accepts.
    CHECK(!gate.accept(GestureKind::DoublePress, 100));
}

void test_gate_game_mode() {
    InputGate gate;
    // Default: option off, a running game does not matter.
    gate.update(inputs(Visibility::Visible, true, false), 0);
    CHECK(gate.open());
    // Option on and a game running: closed even with passthrough visible.
    CHECK(gate.update(inputs(Visibility::Visible, true, true), 100));
    CHECK(!gate.open());
    CHECK(gate.reason() == GateReason::GameRunning);
    // Game exits: open again, guard restarts.
    CHECK(gate.update(inputs(Visibility::Visible, false, true), 5000));
    CHECK(gate.open());
    CHECK(gate.opened_at_ms() == 5000);
    // Option on, no game, passthrough hidden: closed for passthrough.
    gate.update(inputs(Visibility::Hidden, false, true), 6000);
    CHECK(gate.reason() == GateReason::PassthroughHidden);
    // Game mode wins over unknown passthrough too.
    gate.update(inputs(Visibility::Unknown, true, true), 7000);
    CHECK(!gate.open());
}

void test_press_gate_debounce() {
    PressGate press(250);
    CHECK(press.update(true, true, true, 1000));   // rising edge fires
    CHECK(!press.update(true, true, false, 1020)); // still held
    CHECK(!press.update(true, false, true, 1040)); // release
    CHECK(!press.update(true, true, true, 1100));  // within debounce
    CHECK(!press.update(true, false, true, 1150));
    CHECK(press.update(true, true, true, 1300));   // after debounce
    // A press seen without bChanged (already down when first read) never fires.
    PressGate held(250);
    CHECK(!held.update(true, true, false, 0));
    CHECK(!held.update(true, true, true, 10)); // consumed until release
    CHECK(!held.update(true, false, true, 20));
    CHECK(held.update(true, true, true, 30));
    // Inactive action resets arming.
    PressGate inactive(0);
    CHECK(!inactive.update(false, true, true, 0));
    CHECK(inactive.update(true, true, true, 10));
}

void test_parse_rendering_mode() {
    CHECK(parse_room_view_rendering_mode("                    \"currentRoomViewRenderingMode\": 6") == 6);
    CHECK(parse_room_view_rendering_mode("\"currentRoomViewRenderingMode\":0") == 0);
    CHECK(parse_room_view_rendering_mode("\"currentRoomViewRenderingMode\" : 12,") == 12);
    CHECK(!parse_room_view_rendering_mode("\"currentRoomViewRenderingMode\": x"));
    CHECK(!parse_room_view_rendering_mode("\"currentRoomViewRenderingMode\""));
    CHECK(!parse_room_view_rendering_mode("isCameraViewEnabled\": 0"));
    CHECK(!parse_room_view_rendering_mode(""));
}

void test_log_follower() {
    TempDir dir;
    const auto file = dir.path / "vrserver.txt";
    LogFollower follower(file);
    std::vector<std::string> lines;
    const auto collect = [&](const std::string& line) { lines.push_back(line); };
    follower.poll(collect); // missing file is fine
    CHECK(lines.empty());
    append(file, "one\ntwo\npart");
    follower.poll(collect);
    CHECK(lines.size() == 2 && lines[0] == "one" && lines[1] == "two");
    append(file, "ial\n");
    follower.poll(collect);
    CHECK(lines.size() == 3 && lines[2] == "partial");
    follower.poll(collect); // nothing new
    CHECK(lines.size() == 3);
    // Rotation: SteamVR renames the log and starts a new one.
    fs::rename(file, dir.path / "vrserver.previous.txt");
    append(file, "fresh\n");
    follower.poll(collect);
    CHECK(lines.size() == 4 && lines[3] == "fresh");
    // Truncation in place.
    { std::ofstream truncate(file, std::ios::trunc); truncate << "t\n"; }
    follower.poll(collect);
    CHECK(lines.back() == "t");
}

const char* kBlock =
    "Thu Nov 05 2026 08:35:46.761981 [Info] - cv: [XRClientDelegate] SessionSettingsChanged: {\n"
    "            \"xrSessionSettings\": {\n"
    "                \"isCameraViewEnabled\": 0,\n"
    "                \"renderingOptions\": {\n"
    "                    \"currentRoomViewRenderingMode\": %d\n"
    "                }\n";

std::string block(int mode) {
    char text[512];
    std::snprintf(text, sizeof(text), kBlock, mode);
    return text;
}

void test_passthrough_state() {
    TempDir dir;
    const auto file = dir.path / "vrserver.txt";
    // History: the newest logged mode is known after the first poll.
    append(file, block(0) + block(6) + block(0) + block(6));

    // Home view (no scene app): camera.enableConstructRoomView decides,
    // whatever the log or stray events say.
    PassthroughState state(PassthroughSignal::Auto, file);
    state.poll();
    CHECK(state.visibility() == Visibility::Unknown); // setting not read yet
    state.on_construct_setting(true);
    CHECK(state.visibility() == Visibility::Visible);
    state.on_construct_setting(false);
    CHECK(state.visibility() == Visibility::Hidden);
    state.on_room_view_event(true);
    CHECK(state.visibility() == Visibility::Hidden);
    // The probe session's sequence: off, on, off, on, off, on.
    for (bool on : {false, true, false, true, false, true}) {
        state.on_construct_setting(on);
        CHECK(state.visibility() == (on ? Visibility::Visible : Visibility::Hidden));
    }

    // A scene app starts: hidden until an event or a logged mode says
    // otherwise, even though older observations said visible.
    state.set_scene_app_running(true);
    CHECK(state.visibility() == Visibility::Hidden);
    append(file, block(6));
    state.poll();
    CHECK(state.visibility() == Visibility::Visible);
    append(file, block(0));
    state.poll();
    CHECK(state.visibility() == Visibility::Hidden);
    state.on_room_view_event(true);
    CHECK(state.visibility() == Visibility::Visible);
    state.on_construct_setting(false); // ignored while a scene app runs
    CHECK(state.visibility() == Visibility::Visible);
    state.on_room_view_event(false);
    CHECK(state.visibility() == Visibility::Hidden);
    // Scene app quits: back to the setting.
    state.set_scene_app_running(false);
    state.on_construct_setting(true);
    CHECK(state.visibility() == Visibility::Visible);

    PassthroughState scene_log_only(PassthroughSignal::Auto, file);
    scene_log_only.set_scene_app_running(true);
    scene_log_only.poll();
    CHECK(scene_log_only.visibility() == Visibility::Hidden); // newest logged mode is 0
    PassthroughState scene_no_log(PassthroughSignal::Auto, dir.path / "absent.txt");
    scene_no_log.set_scene_app_running(true);
    scene_no_log.poll();
    CHECK(scene_no_log.visibility() == Visibility::Hidden);
    scene_no_log.on_room_view_event(true);
    CHECK(scene_no_log.visibility() == Visibility::Visible);

    // The app starting while a game already runs: the logged history is
    // newer than the scene start and decides.
    PassthroughState late(PassthroughSignal::Auto, file); // newest logged mode is 0
    late.set_scene_app_running(true);
    late.poll();
    CHECK(late.visibility() == Visibility::Hidden);

    PassthroughState setting(PassthroughSignal::Setting, file);
    setting.set_scene_app_running(true);
    setting.on_construct_setting(true);
    CHECK(setting.visibility() == Visibility::Visible);

    PassthroughState events(PassthroughSignal::Events, file);
    events.poll();
    events.on_construct_setting(true);
    CHECK(events.visibility() == Visibility::Unknown); // log and setting ignored
    events.on_room_view_event(false);
    CHECK(events.visibility() == Visibility::Hidden);

    PassthroughState log_only(PassthroughSignal::SteamVrLog, file);
    log_only.on_room_view_event(true);
    CHECK(log_only.visibility() == Visibility::Unknown); // events ignored
    log_only.poll();
    CHECK(log_only.visibility() == Visibility::Hidden);

    PassthroughState off(PassthroughSignal::Off, file);
    CHECK(off.visibility() == Visibility::Visible);
}

// Replays the in-game probe session of 2026-11-05 11:26:47-11:27:30 (OpenFrame).
void test_in_game_replay() {
    TempDir dir;
    const auto file = dir.path / "vrserver.txt";
    append(file, block(6)); // home view with passthrough showing
    PassthroughState state(PassthroughSignal::Auto, file);
    state.on_construct_setting(true);
    state.poll();
    CHECK(state.visibility() == Visibility::Visible);
    struct Step { char kind; int value; Visibility expect; const char* when; };
    // kind: s = scene app start / stop, e = RoomView event, m = logged mode
    const Step steps[] = {
        {'s', 1, Visibility::Hidden, "11:26:47.41 game starts"},
        {'m', 0, Visibility::Hidden, "11:26:54.98 mode 0"},
        {'e', 1, Visibility::Visible, "11:26:56.29 RoomViewShown"},
        {'m', 6, Visibility::Visible, "11:26:56.33 mode 6"},
        {'e', 0, Visibility::Hidden, "11:26:59.07 RoomViewHidden"},
        {'m', 0, Visibility::Hidden, "11:26:59.30 mode 0"},
        {'e', 1, Visibility::Visible, "11:27:00.53 RoomViewShown"},
        {'m', 6, Visibility::Visible, "11:27:00.55 mode 6"},
        {'e', 0, Visibility::Hidden, "11:27:09.92 RoomViewHidden"},
        {'m', 0, Visibility::Hidden, "11:27:10.16 mode 0"},
        {'e', 1, Visibility::Visible, "11:27:10.94 RoomViewShown"},
        {'m', 6, Visibility::Visible, "11:27:10.96 mode 6"},
        {'e', 0, Visibility::Hidden, "11:27:12.13 RoomViewHidden"},
        {'m', 0, Visibility::Hidden, "11:27:12.33 mode 0"},
        {'m', 6, Visibility::Visible, "11:27:16.30 mode 6, no event"},
        {'m', 0, Visibility::Hidden, "11:27:17.41 mode 0, no event"},
        {'m', 6, Visibility::Visible, "11:27:24.59 mode 6, no event"},
        {'s', 0, Visibility::Visible, "11:27:30.25 game quits, setting visible"},
    };
    for (const auto& step : steps) {
        if (step.kind == 's') state.set_scene_app_running(step.value != 0);
        if (step.kind == 'e') state.on_room_view_event(step.value != 0);
        if (step.kind == 'm') { append(file, block(step.value)); state.poll(); }
        if (state.visibility() != step.expect) std::cerr << "  at " << step.when << "\n";
        CHECK(state.visibility() == step.expect);
    }
}

void test_scene_classification() {
    CHECK(!is_game_scene_app(0, ""));
    CHECK(!is_game_scene_app(0, "steam.app.620"));
    CHECK(is_game_scene_app(1234, "steam.app.620"));
    CHECK(is_game_scene_app(1234, "system.generated.openxr.primedgun.primedgun"));
    CHECK(is_game_scene_app(1234, "")); // unknown key: still a scene app
    CHECK(!is_game_scene_app(1234, "steam.app.250820"));
    CHECK(!is_game_scene_app(1234, "openvr.tool.steamvr_environments"));
    CHECK(!is_game_scene_app(1234, "openvr.component.vrcompositor"));
    CHECK(!is_game_scene_app(1234, "system.generated.gamescope"));
}

void test_config() {
    CHECK(parse_config("{}").passthrough_signal == "auto");
    CHECK(parse_config("{\"passthrough_signal\": \"steamvr_log\"}").passthrough_signal == "steamvr_log");
    CHECK(parse_config("{\"passthrough_signal\": \"setting\"}").passthrough_signal == "setting");
    CHECK(throws([] { parse_config("{\"passthrough_signal\": \"maybe\"}"); }));
    // The 0.1.x config files keep working.
    const auto old = parse_config("{\n  \"poll_interval_ms\": 20,\n  \"debounce_ms\": 250\n}\n");
    CHECK(old.poll_interval_ms == 20 && old.debounce_ms == 250);
    CHECK(throws([] { parse_config("{\"unknown\": 1}"); }));
    CHECK(!parse_config("{}").disable_while_game_running);
    CHECK(parse_config("{\"disable_while_game_running\": true}").disable_while_game_running);
    CHECK(throws([] { parse_config("{\"disable_while_game_running\": \"yes\"}"); }));
}

void test_set_config_option() {
    const std::string original = "{\n  \"poll_interval_ms\": 20,\n  \"debounce_ms\": 250\n}\n";
    const auto on = set_config_option(original, "disable_while_game_running", true);
    CHECK(parse_config(on).disable_while_game_running);
    CHECK(parse_config(on).poll_interval_ms == 20);
    // Existing keys keep their order; the new key goes last.
    CHECK(on.find("poll_interval_ms") < on.find("debounce_ms"));
    CHECK(on.find("debounce_ms") < on.find("disable_while_game_running"));
    const auto off = set_config_option(on, "disable_while_game_running", false);
    CHECK(!parse_config(off).disable_while_game_running);
    CHECK(off.find("disable_while_game_running") != std::string::npos);
    // Never rewrite an invalid file, never write an unknown key.
    CHECK(throws([] { set_config_option("{\"bad\": 1}", "disable_while_game_running", true); }));
    CHECK(throws([&] { set_config_option(original, "no_such_option", true); }));
}

} // namespace

int main() {
    const std::vector<std::pair<const char*, std::function<void()>>> tests = {
        {"gate_open_closed", test_gate_open_closed},
        {"gate_unknown_fails_open", test_gate_unknown_fails_open},
        {"gate_mid_gesture_guard", test_gate_mid_gesture_guard},
        {"gate_game_mode", test_gate_game_mode},
        {"press_gate_debounce", test_press_gate_debounce},
        {"parse_rendering_mode", test_parse_rendering_mode},
        {"log_follower", test_log_follower},
        {"passthrough_state", test_passthrough_state},
        {"in_game_replay", test_in_game_replay},
        {"scene_classification", test_scene_classification},
        {"config", test_config},
        {"set_config_option", test_set_config_option},
    };
    for (const auto& [name, test] : tests) {
        const int before = failures;
        try {
            test();
        } catch (const std::exception& error) {
            ++failures;
            std::cerr << name << ": unexpected exception: " << error.what() << "\n";
        }
        std::cout << (failures == before ? "PASS " : "FAIL ") << name << "\n";
    }
    std::cout << checks << " checks, " << failures << " failures\n";
    return failures == 0 ? 0 : 1;
}
