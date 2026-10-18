#!/bin/zsh
# Installs (or with --uninstall removes) the LaunchAgent that keeps the Farsight
# menu-bar language switcher (فارسی/English) running. Build it first: menubar/build.sh
LABEL=io.github.hadi-buddy.farsight.menubar
PLIST="$HOME/Library/LaunchAgents/$LABEL.plist"
DIR=${0:A:h}
launchctl bootout "gui/$(id -u)/$LABEL" 2>/dev/null
if [[ "$1" == --uninstall ]]; then rm -f "$PLIST"; echo "menu bar switcher removed"; exit 0; fi
if [[ ! -x "$DIR/menubar/farsight-menubar" ]]; then
  echo "Build it first: $DIR/menubar/build.sh"
  exit 1
fi
cat > "$PLIST" <<PLIST
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <key>Label</key><string>$LABEL</string>
  <key>ProgramArguments</key><array><string>$DIR/menubar/farsight-menubar</string></array>
  <key>RunAtLoad</key><true/>
  <key>KeepAlive</key><true/>
  <key>ProcessType</key><string>Interactive</string>
  <key>StandardErrorPath</key><string>$HOME/Library/Logs/farsight/menubar.err</string>
</dict>
</plist>
PLIST
launchctl bootstrap "gui/$(id -u)" "$PLIST" && echo "menu bar switcher installed: $PLIST"
