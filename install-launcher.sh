#!/bin/zsh
# Builds (or with --uninstall removes) a stable Dock/Spotlight launcher for Farsight
# at ~/Applications/Farsight.app.
#
# Replaces the old `osacompile -o ~/Applications/Farsight.app -e '...'` approach
# (still shown in older install notes). osacompile doesn't set a CFBundleIdentifier,
# so Launch Services had no stable way to recognize "this is the same app as last
# time" across re-runs -- it could register a fresh entry each time, which is why
# pinning it to the Dock kept producing a new launcher icon instead of updating the
# one already there. This script sets a fixed identifier and forces Launch Services
# to update that same entry (`lsregister -f`) instead of creating a new one.
APP="$HOME/Applications/Farsight.app"
BUNDLE_ID="io.github.hadi-buddy.farsight.launcher"
LSREGISTER="/System/Library/Frameworks/CoreServices.framework/Frameworks/LaunchServices.framework/Support/lsregister"

if [[ "$1" == --uninstall ]]; then
  "$LSREGISTER" -u "$APP" 2>/dev/null
  rm -rf "$APP"
  echo "launcher removed"
  exit 0
fi

rm -rf "$APP"
mkdir -p "$APP/Contents/MacOS" "$APP/Contents/Resources"

cat > "$APP/Contents/Info.plist" <<PLIST
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <key>CFBundleIdentifier</key><string>$BUNDLE_ID</string>
  <key>CFBundleName</key><string>Farsight</string>
  <key>CFBundleDisplayName</key><string>Farsight</string>
  <key>CFBundleExecutable</key><string>Farsight</string>
  <key>CFBundleIconFile</key><string>icon</string>
  <key>CFBundlePackageType</key><string>APPL</string>
  <key>CFBundleShortVersionString</key><string>1.0</string>
</dict>
</plist>
PLIST

cat > "$APP/Contents/MacOS/Farsight" <<'SCRIPT'
#!/bin/zsh
exec "$HOME/tools/farsight/launch.sh"
SCRIPT
chmod +x "$APP/Contents/MacOS/Farsight"

# Reuse Foresight's own icon so the launcher is visually recognizable. Harmless if
# not found -- the Dock just falls back to a generic app icon.
FORESIGHT_ICON=$(find /Applications/Foresight.app/Contents/Resources -maxdepth 1 -iname '*.icns' 2>/dev/null | head -1)
[[ -n "$FORESIGHT_ICON" ]] && cp "$FORESIGHT_ICON" "$APP/Contents/Resources/icon.icns"

touch "$APP"
"$LSREGISTER" -f "$APP"
echo "launcher installed: $APP (drag it to the Dock, or find it with Spotlight)"
