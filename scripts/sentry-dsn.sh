# Sourced by scripts/make-release.sh: the Sentry DSN that --send-report posts
# to. From FRAMEPASSTHROUGH_SENTRY_DSN, else the file
# FRAMEPASSTHROUGH_SENTRY_DSN_FILE (default ~/.config/framepassthrough-dev/sentry-dsn,
# one line). Never commit it: with none the build saves and shows problem
# reports but cannot send them. Prints whether one was found, never the DSN.
if [ -z "${FRAMEPASSTHROUGH_SENTRY_DSN:-}" ]; then
  _dsn_file=${FRAMEPASSTHROUGH_SENTRY_DSN_FILE:-$HOME/.config/framepassthrough-dev/sentry-dsn}
  if [ -r "$_dsn_file" ]; then
    FRAMEPASSTHROUGH_SENTRY_DSN=$(head -n 1 "$_dsn_file" | tr -d '[:space:]')
  fi
  unset _dsn_file
fi
export FRAMEPASSTHROUGH_SENTRY_DSN=${FRAMEPASSTHROUGH_SENTRY_DSN:-}
if [ -n "$FRAMEPASSTHROUGH_SENTRY_DSN" ]; then
  echo "problem reports: --send-report goes to the Sentry DSN given (FRAMEPASSTHROUGH_SENTRY_DSN)"
else
  echo "problem reports: no Sentry DSN (FRAMEPASSTHROUGH_SENTRY_DSN): reports are saved only"
fi
