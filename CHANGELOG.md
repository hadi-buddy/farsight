# Changelog

Farsight (hadi-buddy/farsight), built on michaelpersonal/foresight-zh @ 8f15aea.

## 2026-10-18 — Fix: wrong preference key — `ApplePersistenceIgnoreState`, not `NSQuitAlwaysKeepsWindows`

- **Context:** the previous entry's fix (delete the saved-state folder before launch, from `launch.sh`
  and `relaunchWithLanguage:`) was built, pushed, and tested — bug still reproduced exactly as before.
  Reconsidered the whole theory from scratch instead of adding another timing tweak: the safety-net
  function `DisableFutureStateSaving()` set `NSQuitAlwaysKeepsWindows=false`, but that key is unrelated
  to state restoration itself — it only controls the "keep windows when quitting an app" system
  prompt/behavior. The actual documented AppKit key that disables saving *and* restoring window/session
  state altogether is `ApplePersistenceIgnoreState`. Nothing in this session had ever set it.
- **Why this explains the specific failure mode (menu correct, UI wrong, only after a plain reopen):**
  a direct quit+reopen (not through `launch.sh`) launches Foresight once, briefly, with no
  `-AppleLanguages` override, before `watcher.sh` notices (its own ~2.5s detection window) and relaunches
  it correctly through `launch.sh`. That brief first launch is enough for AppKit to save a fresh English
  snapshot. `launch.sh`'s `rm -rf` on the saved-state folder was correct in principle, but raced against
  macOS finishing that write — sometimes losing. With `ApplePersistenceIgnoreState=YES` set, AppKit never
  writes or reads that snapshot in the first place, so there's nothing left to race against.
- **Change:**
  - `shim/persian_menu.m` (`DisableFutureStateSaving`): also sets `ApplePersistenceIgnoreState=true` via
    `CFPreferencesSetAppValue`/`CFPreferencesAppSynchronize`, alongside the (harmless, kept as-is)
    `NSQuitAlwaysKeepsWindows=false`.
  - `launch.sh`: also runs `defaults write com.google.AIEdgeForesight ApplePersistenceIgnoreState -bool
    YES` directly, right after the existing `rm -rf` of the saved-state folder — so the preference is in
    effect from the very first run, not only after the shim itself has launched at least once before.
  - Kept the saved-state-folder deletion in both `launch.sh` and `relaunchWithLanguage:` as a
    belt-and-suspenders safety net; it should now be a no-op in practice since nothing writes that folder
    once the preference takes effect.
- **Status:** written, not yet rebuilt/verified on macOS.

## 2026-10-18 — Fix: delete the saved-state folder before launch, not inside it

- **Context:** the previous entry's fix (`DisableStateRestoration` deleting the saved-state folder
  inside `persian_menu_init`, the dylib constructor) was built and tested — bug still reproduced,
  screenshot confirmed: فارسی checked in the menu, UI still English. The deletion itself was proven to
  work (that's the whole previous entry), so the remaining variable was timing: our constructor runs
  before `main()`, about as early as a launched process ever lets injected code run — but apparently
  *not* early enough. Whatever applies the saved state is evidently done by the time our constructor
  gets a chance to run at all, even though nothing in Foresight's own code should execute before ours
  does by the usual C/ObjC load order. The one combination that did work in manual testing was always:
  delete the folder, *then* launch Foresight as a fresh step — i.e. the delete has to happen in the
  process doing the launching, before it launches, never inside the launched process itself.
- **Change:**
  - `launch.sh`: deletes `~/Library/Saved Application State/com.google.AIEdgeForesight.savedState`
    right before the `exec open` line that actually starts Foresight. Covers both direct use and
    `watcher.sh`'s catch-and-relaunch path, since both end up here.
  - `shim/persian_menu.m`: split the old `DisableStateRestoration` into `DeleteSavedState()` (called
    from `relaunchWithLanguage:`, in the still-running old process, right before
    `openApplicationAtURL` spawns the new one — same timing as `launch.sh`'s fix, just from Objective-C)
    and `DisableFutureStateSaving()` (kept in `persian_menu_init`, the dylib constructor): sets
    `NSQuitAlwaysKeepsWindows=false` so macOS stops saving state going forward, as a safety net for any
    launch path that runs before `launch.sh`/`relaunchWithLanguage:` get a chance to delete anything
    fresh (a login item, a system-triggered relaunch after a crash).
- **Status:** written, not yet rebuilt/verified on macOS.

## 2026-10-18 — Found the real cause: macOS window-state restoration, not a locale key at all

- **Context:** the "menu says فارسی, app opens in English" bug survived every previous fix in this
  session's thread (constructor-timing, NSUserDefaults dual-write), even though logs proved each of
  those writes landed correctly every time. Captured Foresight's own unified-log output
  (`log stream --predicate 'process == "Foresight"'`) across a kill/reopen cycle: found a line
  confirming Foresight's own Firebase init checks `com.firebase.instanceid.user_defaults.locale` and
  *skips* writing it if a value is already present — ruling out "Foresight overwrites our value" as
  well. With every write-based theory eliminated by direct evidence, tested a completely different
  mechanism: macOS's own **Saved Application State** (window/session restore across quits). Deleted
  `~/Library/Saved Application State/com.google.AIEdgeForesight.savedState` by hand and relaunched —
  fixed it immediately, with no other change. This is what the whole Firebase-locale-key chase had been
  missing: restoration replays the *previous* session's rendered state (language included) on top of
  whatever this launch's locale actually resolves to, regardless of any preference or launch argument.
- **Change (`shim/persian_menu.m`):** `DisableStateRestoration()`, called first in `persian_menu_init()`
  (before `SetFirebaseLocale`): deletes that saved-state folder on every launch and sets
  `NSQuitAlwaysKeepsWindows=false` for Foresight's bundle, so there's nothing saved to restore on a
  future launch this code doesn't run before either (a login item, a system-triggered relaunch).
  `SetFirebaseLocale` itself is kept (now explained as likely harmless rather than load-bearing) since
  something does read that key and skip overwriting it if present — no reason to remove a correct,
  low-risk write now that the actual fix is identified.
- **Status:** `DisableStateRestoration`'s core mechanism (manually deleting the saved-state folder,
  then relaunching) verified fixing the bug on the maintainer's Mac; the automated code path itself not
  yet rebuilt/tested.

## 2026-10-18 — Also write the Firebase locale via NSUserDefaults

- **Context:** the constructor-timing fix (previous entry) still didn't fix the reported bug, but this
  time the log evidence ruled out timing as the cause: `SetFirebaseLocale(fa) ... sync=1
  readback=fa`, 8 ms after `farsight loaded`, and the on-disk plist confirmed `fa` too — the write is
  provably early, successful, and persisted. Yet the UI reportedly still showed English.
  The one real difference between this and the working manual test (`defaults write` then `killall` then
  `./launch.sh`) is *which process* does the writing: the manual test wrote from a separate `defaults`
  process before Foresight even started; our code writes from inside Foresight's own process, very
  early but still during its lifetime. `CFPreferencesSetAppValue`/`CFPreferencesAppSynchronize` is one
  read/write path; if Foresight (or a library inside it, e.g. the Firebase SDK that names this key)
  actually reads this value through `[NSUserDefaults standardUserDefaults]` instead, and something
  already caused that singleton to exist and cache the old value before our constructor got a chance to
  run, our CFPreferences-level write might not be visible through that cached instance even though it's
  genuinely correct at the CFPreferences/plist level.
- **Change (`shim/persian_menu.m`):** `SetFirebaseLocale()` now also writes the same key through
  `[NSUserDefaults standardUserDefaults] setObject:forKey:]` + `synchronize`, covering both possible
  read paths instead of assuming which one Foresight actually uses. Logs both readbacks
  (`cf_readback=`/`ud_readback=`) so a next mismatch, if there is one, is visible directly instead of
  needing another round of `plutil`/log archaeology.
- **Status:** written, not yet verified on macOS.

## 2026-10-18 — Fix: the Firebase-locale write was still too late

- **Context:** the previous entry's fix (`SetFirebaseLocale` in `installMenu`) didn't actually fix the
  reported bug: with فارسی selected and checked in the Language menu, force-quitting Foresight and
  reopening it still showed English. Root cause was a timing race, not a logic error: `installMenu`
  runs on `NSApplicationDidFinishLaunchingNotification`, and Foresight's own startup code apparently
  reads this cached locale key earlier than that — both hang off the same notification (or earlier)
  with no guaranteed order, so our write landed after Foresight had already rendered off the stale
  value. Two places deciding the same thing on different schedules, the same shape of bug this whole
  shim's history keeps running into with timers/observers (see `persian_font.c`'s own interpose-based
  approach, which sidesteps exactly this by intercepting the read itself rather than racing to write
  first).
- **Change (`shim/persian_menu.m`):** moved the `SetFirebaseLocale(CurrentLanguage())` call out of
  `installMenu` and into `persian_menu_init()` directly — i.e. the dylib's
  `__attribute__((constructor))`, which runs at image-load time, before Foresight's own `main()` even
  starts. As early in the process as this shim gets a chance to run anything. Switched
  `NSRunningApplication.currentApplication.bundleIdentifier` to `NSBundle.mainBundle.bundleIdentifier`
  for this call specifically, since the former touches AppKit/NSApp machinery that may not be safely
  usable this early in process startup; `NSBundle` has no such dependency.
- **Status:** written, not yet verified on macOS — the actual timing-sensitive part (does the
  constructor really run early enough relative to Foresight's own locale read) can only be confirmed by
  testing the force-quit/reopen scenario again.

## 2026-10-18 — Found what actually drives Foresight's UI language

- **Context:** with language set to فارسی, force-quitting Foresight and reopening it (through
  `watcher.sh`'s catch-up relaunch) showed the UI in English, even though `ps` confirmed the process
  had launched with `-AppleLanguages (fa) -AppleLocale fa_IR` correctly set. Searched
  `~/Library/Application Support/com.google.AIEdgeForesight/{key_value_store,database}.sqlite`
  (Foresight's own Flutter `shared_preferences`/app-data stores) for any internal language setting
  overriding it — found none, ruling that theory out. Found instead, in
  `~/Library/Preferences/com.google.AIEdgeForesight.plist`, a key named
  `com.firebase.instanceid.user_defaults.locale` stuck at `en`. Writing it to `fa` by hand
  (`defaults write com.google.AIEdgeForesight com.firebase.instanceid.user_defaults.locale -string fa`)
  and relaunching switched the UI to Persian — confirming Foresight keys its own displayed language off
  this cached value (a Firebase-named key, but evidently read by Foresight itself for this), not off
  `-AppleLanguages`. Consistent with finding no `fa.lproj`/`Localizable.strings` anywhere in the bundle
  earlier (see "Menu-bar language switcher"): the UI strings likely don't come from a bundled
  localization table at all.
- **Change (`shim/persian_menu.m`):** `SetFirebaseLocale()` writes this key via
  `CFPreferencesSetAppValue`/`CFPreferencesAppSynchronize` against Foresight's own bundle ID (read from
  `NSRunningApplication`, not hardcoded) — called from `setLanguage:` (every menu click) and again from
  `installMenu` on every launch, so a launch that bypassed the menu (a plain Dock open caught by
  `watcher.sh`) still self-heals instead of leaving the UI on a stale cached language.
  `-AppleLanguages`/`-AppleLocale` are still passed on relaunch too, for whatever in the process does
  follow the normal macOS mechanism (confirmed: our own "Language" menu's own text/RTL handling does).
- **Not changed:** considered and reverted a separate attempt at eliminating the launch flash entirely
  via `LSEnvironment` in Foresight's own `Info.plist` (so the OS sets `DYLD_INSERT_LIBRARIES` with zero
  restart) — it required ad-hoc re-signing the app, which broke Foresight's existing Keychain-stored
  Google Sign-In token access (ad-hoc signatures have no stable Team ID, so macOS can't make an "Always
  Allow" Keychain grant persist across launches the way it does for a real Developer ID signature — the
  prompt kept coming back). Not something a non-developer could be asked to work around (creating and
  trusting a local code-signing certificate), so reverted: reinstalled Foresight fresh from the official
  source to restore its original Google signature and Keychain access, and kept the 500 ms watcher loop
  (previous entry) as the actual answer for the launch flash.
- **Status:** written; `SetFirebaseLocale` verified working via the manual `defaults write` test above,
  the code path itself not yet rebuilt/tested on macOS.

## 2026-10-18 — Watcher poll: 1s → 0.5s, via a persistent loop

- **Context:** asked for the unshimmed-launch flash to be cut to 500ms. launchd's `StartInterval` key
  is whole seconds only (no fractional values) — `1` was already as fine-grained as that mechanism
  allows, so getting below a second needed a different approach, not a smaller number in the plist.
- **Change:**
  - `watcher.sh`: restructured from a script launchd re-invokes periodically into a persistent `while
    true` loop with `sleep 0.5` between checks, run once and kept alive. Every `exit 0` "nothing to do"
    path became `continue` (the loop must keep running); the rate-limit and 2s-settle logic are
    unchanged.
  - `install-watcher.sh`: dropped `StartInterval`, added `KeepAlive` (so launchd restarts the loop if
    it ever dies) alongside the existing `RunAtLoad`.
- **Status:** written, not yet verified on macOS. Re-run `./install-watcher.sh` to replace the old
  periodic LaunchAgent with this one — launchd won't pick up the ProgramArguments/KeepAlive change on
  its own.

## 2026-10-18 — Faster watcher poll (5s → 1s)

- **Context:** opening Foresight directly (Dock/Spotlight, not through `launch.sh`) starts it without
  the shim; `watcher.sh` catches that and relaunches it with the shim, which is correct but visible —
  the user sees Foresight open, then restart itself a few seconds later with the Language menu
  appearing only after that. The real fix (an `LSEnvironment` entry in Foresight.app's own
  `Info.plist`, so the OS sets `DYLD_INSERT_LIBRARIES` on every launch with zero restart) was
  considered and explicitly declined: it edits a signed Google binary on disk, risks invalidating its
  code signature (possible "Foresight is damaged" failure, worse than today's visible-but-working
  restart), and gets wiped by every Foresight update. Not worth that risk for a cosmetic fix.
- **Change:** `install-watcher.sh`: `StartInterval` 5 → 1. Cuts how long an unshimmed launch is visible
  before `watcher.sh` catches it and relaunches with the shim; doesn't eliminate the restart, just
  shortens it. `watcher.sh`'s own 2 s settle delay (avoids racing a relaunch already in progress) is
  unchanged — it's a correctness guard, not a cosmetic one.
- **Status:** written, not yet verified on macOS; run `./install-watcher.sh` again to pick up the new
  interval (a previously-installed LaunchAgent won't update itself).

## 2026-10-18 — Language menu: reliable relaunch via NSWorkspace; left-aligned items

- **Context:** the NSTask+`launch.sh` relaunch (previous entry) wasn't restarting Foresight for the
  user, with no way to tell why without a `shim.log` round-trip (several shell layers — zsh, osascript
  quit, pkill, open — any of which could fail silently, and no evidence it was actually sandbox-safe).
  Separately, the فارسی item's custom RTL paragraph style made it right-justified while English stayed
  left-justified in the same dropdown — visually inconsistent.
- **Change (`shim/persian_menu.m`):**
  - `relaunchWithLanguage:` replaces the NSTask/launch.sh path entirely: `NSWorkspace
    openApplicationAtURL:configuration:` on Foresight's own bundle URL
    (`NSRunningApplication.currentApplication.bundleURL`, not a guessed path), with
    `DYLD_INSERT_LIBRARIES` and `-AppleLanguages`/`-AppleLocale` set directly in the
    `NSWorkspaceOpenConfiguration`, `createsNewApplicationInstance = YES`. That flag matters: without
    it, re-opening an already-running app just activates the existing instance and does nothing — the
    likely actual cause of "doesn't restart". Quits this instance only after the completion handler
    confirms the new one is running, so there's no window where neither instance is up.
  - `PersianMenuTitle()`: dropped the custom paragraph style (RTL/right-alignment), font only — فارسی
    and English now both start from the same left edge like any other menu item.
- **Not changed:** the RTL issue in the AI-summary *content* (bullet on the wrong side, LTR block
  layout for Persian text) is a separate thing, reported in chat but not fixed — it's inside Flutter's
  own text layout, the same code path confirmed unreachable in the "Jalali date" entry's investigation
  (no `CTLine`/`CTTypesetter`/`CTFrame` hook point exists). Only this native AppKit menu, not the
  window content, can be touched this way.
- **Status:** written, not yet verified on macOS.

## 2026-10-18 — Language menu: English label, Vazirmatn+RTL for فارسی

- **Change (`shim/persian_menu.m`):**
  - Top-level menu renamed "زبان" → "Language" (English, as asked).
  - The "فارسی" item gets an explicit `attributedTitle`: `Vazirmatn-Regular` at the menu's own font
    size (`VazirmatnMenuFont()`), plus `NSMutableParagraphStyle.baseWritingDirection = .rightToLeft`
    (`PersianMenuTitle()`). This is explicit, not left to the `CTFontCreateForString` interpose in
    `persian_font.c`: that one is confirmed to work for Flutter/Skia's calls (see the previous
    CHANGELOG entry's investigation), but AppKit's own menu-text layout is a different code path that
    was never verified to go through the same function, so relying on it here would be the same kind
    of guess this project tries not to make. Falls back to the plain menu font if Vazirmatn isn't
    registered (`FARSIGHT_FONT=system`, or fonts not installed) — `?:` in `VazirmatnMenuFont()`, same
    as `persian_font.c`'s own fallback behavior.
  - Restart was already automatic since the previous entry (`setLanguage:` already runs `launch.sh` in
    the background after writing the lang file, no manual step) — clarified in the file's header
    comment since it came up as if it weren't there; nothing to fix in the relaunch logic itself.
- **Status:** written, not yet verified on macOS — in particular whether `attributedTitle` lines up
  visually with the `.state` checkmark the way a plain `title` does (an AppKit quirk worth checking
  once built).

## 2026-10-18 — Language switcher moved into Foresight's own menu bar

- **Context:** the first version of the language switcher (previous entry below) was a separate,
  standalone `NSStatusItem` app (`menubar/`) — independent of Foresight entirely. The user wanted it
  *inside* Foresight's own menu bar instead, next to "Window", not a second icon elsewhere.
- **Change:** removed `menubar/` and `install-menubar.sh`. Added `shim/persian_menu.m`
  (`persian_menu_init()`, called from `farsight.c`'s constructor like `persian_font_init`): on
  `NSApplicationDidFinishLaunchingNotification`, inserts a "زبان" top-level menu right after "Window"
  in `NSApp.mainMenu`, with فارسی/English items (checkmark on the active one). This works because the
  menu bar is native AppKit even though the window content is Flutter (confirmed in the previous
  entry: `setTitle:` is a real selector in the binary, for this exact chrome) — ordinary public
  `NSMenu`/`NSMenuItem` calls apply here, unlike the window content. Picking an item writes the choice
  to `~/tools/farsight/lang` (same file and `launch.sh` fallback as before) and runs `launch.sh` in the
  background; `launch.sh`'s own quit-then-relaunch logic handles closing the current Foresight (which
  includes us) and reopening it in the right language — the menu code never calls `-terminate:` itself,
  to avoid racing that sequence.
  `shim/build.sh` now also compiles `persian_menu.m` and links `-framework AppKit`.
- **Status:** written, not yet built/run (needs `clang`+AppKit on macOS). To verify: `shim/build.sh`,
  launch Foresight, confirm "زبان" appears right after "Window", confirm both items relaunch Foresight
  in the right language with the right item checked, confirm it doesn't duplicate itself across
  relaunches (the `installMenu` guard checks for an existing "زبان" submenu title first).

## 2026-10-18 — Menu-bar language switcher; confirmed Foresight's UI can't be safely translated

- **Context:** asked to translate Foresight's remaining English labels ("Start transcribing", "Live
  Q&A", …) to Persian. Investigated on the maintainer's Mac: `fa.lproj`/`Localizable.strings` don't
  exist anywhere in the app bundle (confirmed with `find`/`plutil` — these labels are plain Swift/Dart
  string literals, not looked up through any localization table). The app is a Flutter app
  (`Powered by Flutter`, Dart/Skia error strings throughout the binary, `FlutterMacOS.framework`
  present) hosted in a thin native shell, not SwiftUI as first assumed. Checked every CoreText symbol
  `FlutterMacOS.framework`'s engine actually imports (`dyld_info -imports`, since `nm -gU` silently
  misses chained-fixup imports on this binary — verified against `CTFontCreateForStringWithLanguage`,
  the symbol `persian_font.c` already interposes successfully, as a sanity check): only font-selection
  and glyph-metric calls (`CTFontCreateForStringWithLanguage`, `CTFontGetGlyphsForCharacters`,
  `CTFontDrawGlyphs`, …), no `CTLineCreateWithAttributedString`/`CTTypesetter*`/`CTFrame*`/`CTRun*`.
  Skia does its own text shaping and only asks CoreText "which font for this (unchanged) text" and
  "draw these already-decided glyphs" — by the time any CoreText call happens the text content and
  layout are fixed, unlike `persian_font.c`'s font swap (which only changes *which font* draws an
  *unchanged* string, a much narrower problem). The remaining option — patching literal strings inside
  the Dart AOT snapshot (`App.framework`) — is a different order of complexity/risk than anything else
  in this shim and was not attempted.
- **Change:** since Foresight's own UI can't be translated, made the *existing* `FARSIGHT_LANG`
  Persian/English toggle (previously terminal-only) reachable without a terminal:
  - `menubar/main.swift` (new): a standalone macOS status-bar app (`NSStatusItem`, `.accessory`
    activation policy so it has no Dock icon), independent of Foresight's own process — never touches
    its binary or menu. Two items, فارسی/English; picking one writes the choice to
    `~/tools/farsight/lang` and relaunches Foresight through `launch.sh`.
  - `launch.sh`: `lang` now falls back to `~/tools/farsight/lang` when `FARSIGHT_LANG` isn't set
    (before falling back to `fa`), so the menu-bar choice persists across logins/relaunches, including
    the existing `watcher.sh` LaunchAgent's own relaunches.
  - `menubar/build.sh`, `install-menubar.sh` (new): build and LaunchAgent install/uninstall, mirroring
    `watcher.sh`/`install-watcher.sh`'s existing pattern exactly.
  - `README.md`/`README_fa.md`: documented under "UI language".
- **Not changed:** no file under `shim/` touched; this is fully independent of the shim and the
  transcription/chat pipeline.
- **Status:** `menubar/main.swift` written and reviewed, not yet built/run (needs `swiftc` on macOS;
  not available in this environment). To verify: `menubar/build.sh`, `./install-menubar.sh`, click
  both menu items, confirm Foresight relaunches in the right language each time, confirm the choice
  survives a Mac restart.

## 2026-10-18 — Jalali date on Persian summaries

- **Context:** Persian summaries had no date at all; the user has to be able to tell, from the
  summary text itself, which session it is from — in the Solar Hijri (Jalali) calendar, not Gregorian.
- **Change:**
  - `shim/persian_date.c`/`.h` (new): `jalali_from_gregorian()`, a pure Gregorian→Jalali conversion
    (public-domain Borkowski algorithm; checked against the Unix epoch, Y2K, three Nowruz dates and
    22 Bahman 1357); `jalali_date_string(time_t)`, formats it in Persian digits and month names
    («۲۶ مهر ۱۴۰۵»); `persian_summary_note(en_headings, time_t)`, the instruction text itself
    (moved out of `chat_bridge.c`'s `persian_note()`, which is now a 2-line wrapper passing
    `time(NULL)` — the actual session time, not a previous summary's or a hardcoded one).
  - The instruction now also asks the model to start its reply with that exact date line, then a
    blank line, before the first heading. Same instruction text otherwise, so `FARSIGHT_HEADINGS=en`
    still keeps the English headings; only one sentence with the date was added to each template.
  - `persian_note()`'s return is now malloc'ed (was a static string); its two callers
    (`match_transcript_language`, `gemma_language_msg`) free it on every exit path.
  - `shim/build.sh`: links `persian_date.c` into the dylib.
- **Tests:** `shim/test/date_test.c` (new) — conversion against the known reference dates above,
  formatting against fixed `time_t` values (`TZ=UTC` for determinism), and that
  `persian_summary_note()` carries the exact date and the right heading language. Wired into
  `shim/test/itn_test.sh`.
- **Status:** unit tests pass (`sh shim/test/itn_test.sh`); to verify on macOS that a real Persian
  summary starts with the date line.

## 2026-10-18 — Lower Gemma temperature for Persian summaries

- **Context:** Gemma's Persian summaries had garbled words in every run («تحلی» for «تحلیلی»,
  «تعاری» for «تعاریف», «موفقیت‌آمی», «مثلهفعل»). Foresight samples Gemma at temperature 0.4 /
  top_k 40, which a small on-device model handles poorly in Persian.
- **Change:** `gemma_create()` in `shim/chat_bridge.c` wraps `litert_lm_create_conversation_v2`
  for conversations served by Gemma and sets every `temperature` / `top_k` in the config to
  0.1 / 10. `FARSIGHT_GEMMA_SAMPLING="<temp>,<top_k>"` changes the values, `app` keeps Foresight's.
  Logs `chat: Gemma sampling …` once.
- **Status:** config rewrite unit-tested; to verify on macOS that summaries have fewer misspellings.

## 2026-10-18 — Persian summary headings

- **Context:** Persian summaries still had the English template headings «Overview» and «Details»,
  which the language instruction asked the model to keep.
- **Change:** `persian_note()` in `shim/chat_bridge.c`: for a Persian transcript the model is asked to
  write the headings in Persian too («# مرور کلی», «# جزئیات»); JSON keys stay in English.
  `FARSIGHT_HEADINGS=en ./launch.sh` restores the English headings. Applies to Gemma and the
  local-server bridge.
- **Status:** to verify on macOS that Foresight still shows the summary correctly (the app might
  use the English headings to split sections).
- **Not changed:** UI labels hard-coded in Foresight («Start transcribing», «Live Q&A», «Now»,
  «Transcript», «Start new session», «Link scheduled meeting») are compiled into Google's binary and
  absent from its Persian translation table; Farsight does not modify the app, so they stay English.

## 2026-10-18 — Echo dedup, Persian summaries from Gemma, «نه» stays a word

- **Context:** a real test (a video playing through the speakers) gave every sentence twice,
  once as "Microphone" and once as "Remote" (system audio); phrases cut by the 15 s limit
  ended in a wrong «.»; «نه» ("not") became «۹»; Foresight's summary came back in English.
- **Change:**
  - `shim/sensevoice_engine.c`: cross-stream dedup. Each phrase is compared, by word pairs
    (single words like «و» match everything), with what the other stream said in the last
    30 s. If ≥60 % of its pairs are there it is a repeat: dropped, or trimmed to its new
    trailing words (3+). Repeats are still remembered for the other stream. On the test
    transcript: 33 lines → 20, no content lost. Tuning: `dedup`, `dedup_window`, `dedup_overlap`.
  - A phrase cut by `max_speech` (now 20 s) gets no final «.» (`persian_punctuate(text, complete)`).
  - `shim/persian_itn.c`: a lone «نه» is left as a word, like «یک»; «بیست و نه», «نه هزار» still convert.
  - `shim/chat_bridge.c`: when Gemma serves a request (bridge off or server down), a summary
    request with a Persian transcript gets the "write in Persian" instruction appended to the
    user message. The Persian share is now measured inside the transcript tags only.
- **Status:** dedup tested on the real transcript, other parts unit-tested; to verify on macOS.

## 2026-10-18 — Punctuation

- **Context:** with Koochik and ZWNJ the live test was accurate («در جلسه دیزاین امروز ما بررسی
  می‌کنیم که فیدبک‌ها همیشه بهش پرداخته بشه»), but Shenava outputs no punctuation.
- **Change:** `persian_punctuate` in `shim/persian_text.c`, applied per phrase (a phrase ends at a
  ~0.9 s pause): «؟» after a leading question word (چرا، چطور، کی، …) or a tag («… نه»); no mark
  when the phrase ends on a connector (که، و، ولی، به، …) since the sentence continues; «.»
  otherwise; «،» before اما/ولی/چون/یعنی after two or more words. `punctuation = 0` in the
  tuning file turns it off. Tests in `shim/test/text_test.c`.
- **Status:** unit tests pass; to verify on macOS.

## 2026-10-18 — Tuning file, postroll, phrase audio dump

- **Context:** recognition still makes mistakes with Koochik; tuning needs real examples and
  parameters that can change without a rebuild.
- **Change:** `~/tools/farsight/tuning` (see `tuning.example`) sets vad_threshold, min_silence,
  min_speech, max_speech, preroll, postroll, threads, dump; values are logged as `tuning:`.
  Phrases are now decoded with postroll (0.15 s) as well as preroll. `dump = 1` (or
  `FARSIGHT_DUMP=1`) writes each phrase as 16 kHz WAV plus `index.tsv` in
  `~/Library/Logs/farsight/audio/`.
- **Status:** compiles; to verify on macOS. Vazirmatn verified on macOS (whole UI in Vazirmatn).

## 2026-10-18 — Vazirmatn font for Persian text

- **Context:** Persian text in Foresight used the macOS fallback font; Vazirmatn is the expected
  Persian UI font.
- **Change:**
  - `shim/persian_font.c` (new): registers `~/Library/Application Support/farsight/fonts/Vazirmatn-*.ttf`
    for the process and interposes `CTFontCreateForString` / `…WithLanguage`. For a run that starts
    with an Arabic-script character it returns Vazirmatn at the same size and the nearest of nine
    weights (from the requested font's `kCTFontWeightTrait`); everything else is unchanged.
    Logs `font: Vazirmatn in use` the first time. `FARSIGHT_FONT=system` disables it.
  - `setup-persian.sh` downloads the nine static Vazirmatn TTFs (pinned commit 6e553e3, SIL OFL 1.1).
  - `shim/build.sh` links CoreText and CoreFoundation.
- **Status:** verified on macOS 2026-10-18.

## 2026-10-18 — Bigger Persian model; longer phrase pause

- **Context:** with Rizeh (32M) the live test gave «حدمند» for «هدفمند», and the phrase was split
  mid-word into «حدمند» + «ذ بشه» at a short pause.
- **Change:**
  - `setup-persian.sh` downloads Shenava Koochik v1.0 (114M, same NeMo CTC API; 7.49% WER /
    2.30% CER on visualears-golden-6669) by default; `FARSIGHT_MODEL=rizeh` keeps the small one.
  - The engine prefers `persian/shenava-koochik-v1.0`, falling back to Rizeh, and logs which.
  - VAD `min_silence_duration` 0.6 s → 0.9 s for Persian, so short pauses don't split phrases.
- **Status:** to verify on macOS.

## 2026-10-18 — Persian UI by default

- **Context:** testing `FARSIGHT_LANG=fa` showed Foresight ships a Persian translation: the whole UI
  switched to Persian and right-to-left, with no patching needed.
- **Change:** `launch.sh` now starts Foresight in Persian by default (`-AppleLanguages (fa)`,
  `-AppleLocale fa_IR`); `FARSIGHT_LANG=en` or `system` overrides. READMEs updated; the
  "left-to-right layout" limitation is gone.
- **Status:** verified on macOS (Persian RTL UI).

## 2026-10-18 — Persian orthography, VAD pre-roll, RTL experiment

- **Context:** the first live run showed «می شه» / «طرح های» without ZWNJ, and a clipped first
  syllable («ظه ضبط»).
- **Change:**
  - `shim/persian_text.c` (new): ي/ى→ی, ك→ک, Arabic-Indic→Persian digits; ZWNJ for «می/نمی + verb»,
    «word + ها/های/…», «…ه + ای». Whole tokens only. Tests in `shim/test/text_test.c`.
  - `shim/sensevoice_engine.c`: per-stream 32 s ring of 16 kHz audio; each phrase is decoded with
    0.35 s of audio before the VAD start. Output goes through ITN, then `persian_fix`.
  - `launch.sh`: `FARSIGHT_LANG=fa` starts Foresight with that UI language, to test whether the
    app lays out right-to-left.
- **Status:** unit tests pass on Linux; to verify on macOS.

## 2026-10-18 — First live run on macOS

- **Context:** first end-to-end test on the maintainer's Mac (Apple Silicon, Foresight with Gemma 4 E4B).
- **Result:** `config: engine=persian`, `filter patch: applied`, `asr (persian): ready`; Persian speech
  shows up in Foresight's Transcript tab phrase by phrase, letters joined correctly.
- **Seen in the test:** the first syllable of a phrase can be clipped by VAD («ظه ضبط» for «...ه ضبط»);
  no zero-width non-joiner in «می شه», «طرح های».
- **Status:** working. Next: ZWNJ/Arabic-letter normalization, VAD pre-roll.

## 2026-10-18 — Resumable downloads in setup-persian.sh

- **Context:** on a slow or unstable connection the sherpa-onnx download (silent `curl | tar`)
  looked frozen, and any drop meant starting over.
- **Change:** a `fetch` helper downloads to `<file>.part` with a progress bar and resumes with
  `curl -C -` after a drop (up to 50 tries; a stall under 1 KB/s for 60 s counts as a drop).
  Used for sherpa-onnx, Shenava and Silero VAD.
- **Status:** done.

## 2026-10-17 — Rename to Farsight; new README

- **Context:** the fork should stand as its own project, findable by people searching for
  Persian speech-to-text on macOS, instead of reading as a copy of foresight-zh.
- **Change:**
  - Paths: repo `~/tools/foresight-zh` → `~/tools/farsight`; logs `~/Library/Logs/farsight`;
    models `~/Library/Application Support/farsight` (was `~/local-llm/models/special/foresight-zh`).
  - `shim/foresight_zh.c` → `shim/farsight.c`, output `libfarsight.dylib`; env vars
    `FORESIGHT_ZH_*` → `FARSIGHT_*`; LaunchAgent label `io.github.hadi-buddy.farsight.watcher`.
  - `launch.sh` finds the shim next to itself instead of a hard-coded path.
  - `README.md` and `README_fa.md` rewritten from scratch: Persian-first, install steps,
    limitations, credits to foresight-zh and Shenava.
- **Status:** done; GitHub repo renamed to `farsight`.

## 2026-10-17 — Remove the Chinese README

- **Context:** this fork only targets Persian.
- **Change:** deleted `README_zh.md` and its language links in `README.md` and `README_fa.md`.
  The Chinese code paths (SenseVoice engine, CJK spacing) stay as a fallback.
- **Status:** done.

## 2026-10-17 — Persian as the default engine

- **Context:** the fork should transcribe Persian without any extra configuration.
- **Change:** `load_config` in `shim/foresight_zh.c` defaults to `persian` when
  `~/tools/foresight-zh/engine` is absent. `setup-persian.sh` removes that file instead of
  writing it. Fallback order: Persian → SenseVoice → Foresight's own English engine.
- **Status:** done; verified on macOS 2026-10-18.

## 2026-10-17 — Persian transcription engine

- **Context:** SenseVoice-Small only covers zh/en/yue/ja/ko, so it cannot transcribe Persian.
- **Change:**
  - `shim/sensevoice_engine.c`: `sv_available(home, lang)` selects SenseVoice or
    Shenava Rizeh v1.0 (Persian NeMo CTC, Apache-2.0) via `model_config.nemo_ctc`.
    The model's ONNX metadata disables NeMo feature normalization, so the feature config
    is left untouched.
  - `shim/persian_itn.c` (new, written from scratch): spoken Persian numbers → Persian
    digits; a lone «یک» stays a word. Tests: `shim/test/itn_test.sh` (17 cases, passing).
  - `shim/chat_bridge.c`: Arabic-script share ≥ 30% → ask the chat model for Persian output.
  - `setup-persian.sh`, `README_fa.md`, Persian section in `README.md`.
- **Status:** verified on macOS 2026-10-18 (see "First live run").
