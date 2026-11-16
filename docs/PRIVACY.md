# Privacy

FramePassthrough makes no network connection of its own. It has no telemetry, no update check and nothing that runs in the background to send data. The only exception is `--send-report`, below, and only when you run it.

## What stays on the Frame

`~/.config/frame-passthrough-shortcuts/`:

| File | What it holds |
| --- | --- |
| `application.log` | What the app did: gate changes, scene app keys, binding errors, the camera source. It can contain your home folder path |
| `runtime.json` | The live gate state and the app's process ID |
| `camera-preference.json` | Your colour / mono choice |
| `problem-report.txt` | A problem report, after a crash or a fatal error (below) |

## Problem reports

When the resident app crashes (a signal such as SIGSEGV) or stops with an error, it saves `problem-report.txt`. `--report-problem "what happened"` saves one by hand. A crash is written by the crash handler as `crash-signal.txt` and turned into the report on the next start, or by `--show-report` / `--send-report`.

The report holds: the kind (crash, error or manual) and the reason, the signal, the version and release name, the command, when it started and when it was saved, the system name, kernel and CPU type, for a crash the stack as code addresses in the program and its libraries, for a manual report what you typed, and the last 200 lines of `application.log`.

Before the file is written, the whole text is scrubbed (`src/report.cpp`): your home folder becomes `~`, your user name `<user>` and the host name `<host>` (whole words only), IPv4 and IPv6 addresses `<ip>` and MAC addresses `<mac>`. Scene app keys such as `steam.app.620` stay, so the report can show which game was running.

## Sending a report

Nothing is sent unless you run `frame-passthrough-shortcuts --send-report`. That sends that one report, and only it, then deletes it. There is no setting that sends reports by itself. `--show-report` prints exactly the file that is sent, and `--discard-report` deletes it unsent.

`--send-report` runs `curl` once:

| Host | Request | What |
| --- | --- | --- |
| the Sentry DSN's host (`o<id>.ingest.us.sentry.io`) | `POST /api/<project>/envelope/` with the header `X-Sentry-Auth` (the DSN's public key) | One Sentry event and `problem-report.txt` as its attachment |

The event repeats the report's kind, reason, signal, release, command and system as fields and tags. It has no user, no server name and no device context. The program cannot hide the address a request comes from: the event asks Sentry not to attach it (`sdk.settings.infer_ip` is `never`) and carries an empty `user.geo`, and the Sentry project's "Prevent Storing of IP Addresses" option is to be kept on. Only HTTPS is allowed, and the certificate is checked against the system's store.

The DSN is built in at compile time (`FPS_SENTRY_DSN`, see [TECHNICAL.md](TECHNICAL.md#building)). A build without one saves and shows reports but cannot send them; attach the file to a GitHub issue instead.
