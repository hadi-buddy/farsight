#!/bin/zsh
# Launches Foresight with the farsight (Persian) transcription shim. Restarts it if it is already running.
# FARSIGHT_TRACE=1 launch.sh  -> log every ODML call and transcript result to ~/Library/Logs/farsight/shim.log
# FARSIGHT_LANG=fa launch.sh   -> start Foresight with its UI language set to fa (or ar, ...);
#                                 if the app ships that locale, Flutter lays the UI out right-to-left
SHIM="${0:A:h}/shim/libfarsight.dylib"
if pgrep -x Foresight >/dev/null; then
  osascript -e 'tell application "Foresight" to quit' >/dev/null 2>&1
  for _ in {1..20}; do pgrep -x Foresight >/dev/null || break; sleep 0.5; done
  pkill -x Foresight 2>/dev/null; sleep 1
fi
env_args=(--env "DYLD_INSERT_LIBRARIES=$SHIM")
[[ -n "$FARSIGHT_TRACE" ]] && env_args+=(--env "FARSIGHT_TRACE=$FARSIGHT_TRACE")
app_args=()
[[ -n "$FARSIGHT_LANG" ]] && app_args=(--args -AppleLanguages "($FARSIGHT_LANG)" -AppleLocale "${FARSIGHT_LANG}_IR")
exec open "${env_args[@]}" -a Foresight "${app_args[@]}"
