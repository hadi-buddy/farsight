#!/bin/zsh
# A persistent loop (see install-watcher.sh -- launchd starts this once and keeps
# it alive, not a periodic invocation) that polls every 0.5s. If Foresight is
# running without the shim (plain launch from the Dock, a notification, a login
# item or an update), relaunches it through launch.sh. 0.5s instead of launchd's
# own StartInterval because that key is whole seconds only, and a few seconds of
# an unshimmed Foresight on screen before the catch-up relaunch was a visible,
# unwanted flash.
#
# Pause:  touch ~/Library/Logs/farsight/watcher.paused   (rm to resume)
LOGDIR="$HOME/Library/Logs/farsight"
DIR=${0:A:h}
mkdir -p "$LOGDIR"
log() { print -r -- "[$(date '+%Y-%m-%d %H:%M:%S')] $*" >> "$LOGDIR/watcher.log"; }

while true; do
  sleep 0.5

  [[ -e "$LOGDIR/watcher.paused" ]] && continue
  [[ -f "$DIR/shim/libfarsight.dylib" ]] || continue
  pid=$(pgrep -x Foresight | head -1)
  [[ -z "$pid" ]] && continue
  [[ "$(cat "$LOGDIR/shim.pid" 2>/dev/null)" == "$pid" ]] && continue

  # Give a launch that is in progress (launch.sh itself) a moment to settle.
  sleep 2
  pid=$(pgrep -x Foresight | head -1)
  [[ -z "$pid" || "$(cat "$LOGDIR/shim.pid" 2>/dev/null)" == "$pid" ]] && continue

  # Rate limit: at most 3 relaunches in 5 minutes, so a broken shim cannot cause a restart loop.
  stamps="$LOGDIR/watcher.relaunches"
  now=$(date +%s)
  recent=$(awk -v n="$now" '$1 > n - 300' "$stamps" 2>/dev/null | wc -l | tr -d ' ')
  if (( recent >= 3 )); then
    log "Foresight (pid $pid) runs without the shim, but relaunched 3 times in 5 min - giving up for now"
    continue
  fi
  print "$now" >> "$stamps"
  tail -n 20 "$stamps" > "$stamps.tmp" && mv "$stamps.tmp" "$stamps"
  log "Foresight (pid $pid) started without the shim - relaunching with it"
  "$DIR/launch.sh"
done
