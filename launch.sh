#!/bin/zsh
# Launches Foresight with the farsight (Persian) transcription shim. Restarts it if it is already running.
# FARSIGHT_TRACE=1 launch.sh  -> log every ODML call and transcript result to ~/Library/Logs/farsight/shim.log
# UI language: Persian (right-to-left) by default. FARSIGHT_LANG=en launch.sh for English,
#              FARSIGHT_LANG=system to follow the macOS language. Without FARSIGHT_LANG set,
#              falls back to ~/tools/farsight/lang (written by Foresight's own "زبان" menu,
#              see shim/persian_menu.m), then to fa.
# Summary headings: Persian by default; FARSIGHT_HEADINGS=en launch.sh keeps "Overview"/"Details".
# Font: Persian text uses Vazirmatn when installed; FARSIGHT_FONT=system launch.sh keeps the macOS font.
SHIM="${0:A:h}/shim/libfarsight.dylib"
if pgrep -x Foresight >/dev/null; then
  osascript -e 'tell application "Foresight" to quit' >/dev/null 2>&1
  for _ in {1..20}; do pgrep -x Foresight >/dev/null || break; sleep 0.5; done
  pkill -x Foresight 2>/dev/null; sleep 1
fi
env_args=(--env "DYLD_INSERT_LIBRARIES=$SHIM")
[[ -n "$FARSIGHT_TRACE" ]] && env_args+=(--env "FARSIGHT_TRACE=$FARSIGHT_TRACE")
[[ -n "$FARSIGHT_FONT" ]] && env_args+=(--env "FARSIGHT_FONT=$FARSIGHT_FONT")
[[ -n "$FARSIGHT_HEADINGS" ]] && env_args+=(--env "FARSIGHT_HEADINGS=$FARSIGHT_HEADINGS")
[[ -n "$FARSIGHT_GEMMA_SAMPLING" ]] && env_args+=(--env "FARSIGHT_GEMMA_SAMPLING=$FARSIGHT_GEMMA_SAMPLING")
saved_lang=$(cat "$HOME/tools/farsight/lang" 2>/dev/null)
lang=${FARSIGHT_LANG:-${saved_lang:-fa}}
app_args=()
case $lang in
  system) ;;
  fa) app_args=(--args -AppleLanguages "(fa)" -AppleLocale fa_IR) ;;
  *) app_args=(--args -AppleLanguages "($lang)") ;;
esac
# -AppleLanguages/-AppleLocale as a launch *argument* only populates NSUserDefaults'
# temporary, process-local "argument domain" -- it does NOT survive Foresight
# re-executing itself shortly after a cold launch (confirmed by log evidence: a
# second, unrelated-looking Foresight process with no args at all, but the same
# DYLD_INSERT_LIBRARIES from our *environment*, appears ~6s after every launch).
# Environment variables survive that re-exec; command-line arguments don't. So
# also persist the language as a real, on-disk preference under Foresight's own
# bundle ID -- every process (including the re-exec'd one) reads its own disk
# preferences fresh, regardless of its argv.
case $lang in
  system) defaults delete com.google.AIEdgeForesight AppleLanguages 2>/dev/null
          defaults delete com.google.AIEdgeForesight AppleLocale 2>/dev/null ;;
  fa) defaults write com.google.AIEdgeForesight AppleLanguages -array fa
      defaults write com.google.AIEdgeForesight AppleLocale fa_IR ;;
  *) defaults write com.google.AIEdgeForesight AppleLanguages -array "$lang"
     defaults delete com.google.AIEdgeForesight AppleLocale 2>/dev/null ;;
esac
# macOS's own window/app state restoration replays the *previous* session's
# rendered UI (language included) over whatever this launch's locale resolves
# to -- confirmed by testing. Must happen here, before `open` starts the new
# process: deleting it from inside Foresight's own startup code was too late
# (restoration is evidently applied before that code gets a chance to run).
rm -rf "$HOME/Library/Saved Application State/com.google.AIEdgeForesight.savedState"
# Belt-and-suspenders for the actual root fix: ApplePersistenceIgnoreState
# tells AppKit to neither save nor restore state at all, so even a plain
# reopen (double-click, Dock, Spotlight -- not through this script) that
# happens before the shim's own constructor runs stays correct. Written here
# too (not only from persian_menu.m) so it's in effect from the very first
# run, not only after the shim has launched at least once before.
defaults write com.google.AIEdgeForesight ApplePersistenceIgnoreState -bool YES 2>/dev/null
# -n: force a genuinely new instance. Without it, `open -a` silently drops
# --args/--env (reactivates the existing process instead of spawning fresh)
# whenever Foresight isn't fully gone yet -- this was the real bug: the
# process this script starts for a plain reopen (caught by watcher.sh) always
# had the shim's DYLD_INSERT_LIBRARIES but never -AppleLanguages, confirmed by
# inspecting its full, untruncated argv. The menu's own relaunch path
# (persian_menu.m, NSWorkspaceOpenConfiguration.createsNewApplicationInstance
# = YES) already forces a new instance and was never affected.
exec open -n "${env_args[@]}" -a Foresight "${app_args[@]}"
