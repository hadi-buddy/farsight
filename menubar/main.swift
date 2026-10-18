// Menu-bar language switcher for Farsight.
//
// A tiny standalone status-bar app (no Dock icon, via .accessory activation
// policy) with two items, فارسی/English. Picking one writes it to
// ~/tools/farsight/lang (read by launch.sh as the default when FARSIGHT_LANG
// isn't set) and relaunches Foresight through launch.sh, so the shim and the
// -AppleLanguages/-AppleLocale switch stay exactly as launch.sh already does
// them. Runs independently of Foresight's own process; never touches its
// binary or menu.
//
// Build:   ./build.sh
// Install: ../install-menubar.sh   (LaunchAgent, starts this at login)

import Cocoa

let farsightDir = ("~/tools/farsight" as NSString).expandingTildeInPath
let langFile = farsightDir + "/lang"
let launchScript = farsightDir + "/launch.sh"

func currentLanguage() -> String {
  let saved = (try? String(contentsOfFile: langFile, encoding: .utf8))?
    .trimmingCharacters(in: .whitespacesAndNewlines)
  return (saved?.isEmpty == false) ? saved! : "fa"
}

final class AppDelegate: NSObject, NSApplicationDelegate {
  var statusItem: NSStatusItem!
  var faItem: NSMenuItem!
  var enItem: NSMenuItem!

  func applicationDidFinishLaunching(_ notification: Notification) {
    FileHandle.standardError.write("farsight-menubar: launched, creating status item\n".data(using: .utf8)!)
    statusItem = NSStatusBar.system.statusItem(withLength: NSStatusItem.squareLength)
    if let button = statusItem.button {
      // A text title is the reliable fallback: if the SF Symbol fails to load
      // for any reason, an empty image still leaves a visible, clickable item.
      button.title = "FA"
      if let icon = NSImage(systemSymbolName: "globe", accessibilityDescription: "Farsight language") {
        icon.isTemplate = true
        button.image = icon
        button.imagePosition = .imageOnly
      }
    }

    let menu = NSMenu()

    faItem = NSMenuItem(title: "فارسی", action: #selector(selectFarsi), keyEquivalent: "")
    faItem.target = self
    menu.addItem(faItem)

    enItem = NSMenuItem(title: "English", action: #selector(selectEnglish), keyEquivalent: "")
    enItem.target = self
    menu.addItem(enItem)

    menu.addItem(NSMenuItem.separator())
    menu.addItem(NSMenuItem(title: "Quit", action: #selector(NSApplication.terminate(_:)), keyEquivalent: "q"))

    statusItem.menu = menu
    refreshCheckmarks()
    FileHandle.standardError.write("farsight-menubar: status item set up, button present: \(statusItem.button != nil)\n".data(using: .utf8)!)
  }

  func refreshCheckmarks() {
    let lang = currentLanguage()
    faItem.state = (lang == "fa") ? .on : .off
    enItem.state = (lang == "en") ? .on : .off
  }

  @objc func selectFarsi() { setLanguage("fa") }
  @objc func selectEnglish() { setLanguage("en") }

  func setLanguage(_ lang: String) {
    guard lang != currentLanguage() else { return }
    try? FileManager.default.createDirectory(atPath: farsightDir, withIntermediateDirectories: true)
    try? lang.write(toFile: langFile, atomically: true, encoding: .utf8)
    refreshCheckmarks()
    relaunchForesight()
  }

  func relaunchForesight() {
    let task = Process()
    task.executableURL = URL(fileURLWithPath: "/bin/zsh")
    task.arguments = [launchScript]
    try? task.run()
  }
}

let app = NSApplication.shared
let delegate = AppDelegate()
app.delegate = delegate
app.setActivationPolicy(.accessory)
app.run()
