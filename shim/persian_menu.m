// "Language" menu for Foresight's own menu bar.
//
// The menu bar (Foresight/Edit/View/Window, …) is native AppKit even though
// Foresight's window content is Flutter (confirmed: `setTitle:` is a real
// selector in the binary, and the Flutter engine links AppKit only for the
// window/menu chrome around its own canvas — see CHANGELOG, "Menu-bar
// language switcher"). So unlike the window content, this part can be
// extended with ordinary, public NSMenu/NSMenuItem calls.
//
// On NSApplicationDidFinishLaunchingNotification, inserts a top-level
// "Language" menu right after "Window", with فارسی/English items. Picking one
// writes the choice to ~/tools/farsight/lang (also launch.sh's own fallback,
// for a manual `./launch.sh` later) and relaunches Foresight itself: opens a
// *new* instance via NSWorkspace with DYLD_INSERT_LIBRARIES and
// -AppleLanguages/-AppleLocale set directly (createsNewApplicationInstance =
// YES -- without it, re-opening an already-running app just activates the
// existing instance and does nothing, which looked exactly like "nothing
// happens" when first reported), then quits this instance once the new one is
// confirmed running. Deliberately not going through launch.sh/NSTask+zsh for
// this: a spawned shell depends on Foresight not being sandboxed against
// subprocess execution, and its own quit-the-old-instance step, running an
// AppleScript against an app that is quitting itself, is a second thing that
// can race or fail silently. NSWorkspace's app-launch API is the path Apple
// expects even sandboxed apps to relaunch through.

#import <AppKit/AppKit.h>

extern void logf_(const char *fmt, ...);

static char g_lang_file[700];
static char g_dylib_path[700];

// The same Vazirmatn font persian_font.c registers process-wide (see its
// comment), looked up directly here: AppKit's own menu-text layout may not
// go through the CTFontCreateForString fallback persian_font.c interposes
// (that's confirmed for Flutter/Skia's calls, not for NSMenuItem), so فارسی
// gets the right font explicitly instead of relying on it indirectly.
static NSFont *VazirmatnMenuFont(void) {
  NSFont *menuFont = [NSFont menuFontOfSize:0];
  NSFont *f = [NSFont fontWithName:@"Vazirmatn-Regular" size:menuFont.pointSize];
  return f ?: menuFont;  // FARSIGHT_FONT=system, or fonts not installed: fall back silently
}

// Font only, deliberately no custom alignment/writing-direction: both menu
// items (فارسی and English) need to start from the same left edge like any
// other NSMenuItem, not have فارسی right-justified while English stays
// left-justified.
static NSAttributedString *PersianMenuTitle(NSString *text) {
  return [[NSAttributedString alloc] initWithString:text attributes:@{
    NSFontAttributeName : VazirmatnMenuFont(),
  }];
}

static NSString *CurrentLanguage(void) {
  NSString *path = [NSString stringWithUTF8String:g_lang_file];
  NSString *saved = [[NSString stringWithContentsOfFile:path encoding:NSUTF8StringEncoding error:NULL]
      stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]];
  return (saved.length > 0) ? saved : @"fa";
}

// Foresight's own UI strings turned out not to follow -AppleLanguages at all --
// they come from wherever Foresight keys its translated content off of, and
// that's this cached locale (found by inspecting
// ~/Library/Preferences/<bundle-id>.plist and confirmed by writing it
// directly and watching the UI switch). It's a Firebase SDK key by name, but
// Foresight itself is evidently reading it for this. -AppleLanguages still
// gets passed too, for anything in the process that does follow the normal
// macOS mechanism (e.g. our own injected "Language" menu's text direction).
static void SetFirebaseLocale(NSString *lang) {
  NSString *bundleID = NSRunningApplication.currentApplication.bundleIdentifier;
  if (!bundleID) return;
  CFPreferencesSetAppValue(CFSTR("com.firebase.instanceid.user_defaults.locale"), (__bridge CFStringRef)lang,
                            (__bridge CFStringRef)bundleID);
  CFPreferencesAppSynchronize((__bridge CFStringRef)bundleID);
}

@interface FarsightMenuController : NSObject
@property(nonatomic, strong) NSMenuItem *faItem;
@property(nonatomic, strong) NSMenuItem *enItem;
- (void)installMenu;
- (void)selectFarsi:(id)sender;
- (void)selectEnglish:(id)sender;
@end

@implementation FarsightMenuController

- (void)installMenu {
  // Self-heals every launch, not just on a menu click: if this process somehow
  // started (plain Dock launch caught by watcher.sh, a stale cached value from
  // before SetFirebaseLocale existed, etc.) with Foresight's cached locale out
  // of sync with our saved choice, fix it now rather than leave the UI in the
  // wrong language until the user happens to click the menu again.
  SetFirebaseLocale(CurrentLanguage());

  NSMenu *mainMenu = NSApp.mainMenu;
  if (!mainMenu) {
    logf_("menu: no mainMenu yet, Language menu not added");
    return;
  }
  for (NSMenuItem *item in mainMenu.itemArray) {
    if ([item.submenu.title isEqualToString:@"Language"]) return;  // already installed
  }

  NSMenuItem *langMenuItem = [[NSMenuItem alloc] initWithTitle:@"Language" action:NULL keyEquivalent:@""];
  NSMenu *langMenu = [[NSMenu alloc] initWithTitle:@"Language"];
  langMenuItem.submenu = langMenu;

  self.faItem = [[NSMenuItem alloc] initWithTitle:@"فارسی" action:@selector(selectFarsi:) keyEquivalent:@""];
  self.faItem.target = self;
  self.faItem.attributedTitle = PersianMenuTitle(@"فارسی");
  [langMenu addItem:self.faItem];

  self.enItem = [[NSMenuItem alloc] initWithTitle:@"English" action:@selector(selectEnglish:) keyEquivalent:@""];
  self.enItem.target = self;
  [langMenu addItem:self.enItem];

  [self refreshChecks];

  NSUInteger windowIndex = NSNotFound;
  for (NSUInteger i = 0; i < mainMenu.itemArray.count; i++) {
    if ([mainMenu.itemArray[i].title isEqualToString:@"Window"]) {
      windowIndex = i;
      break;
    }
  }
  if (windowIndex == NSNotFound) {
    [mainMenu addItem:langMenuItem];  // fallback: append at the end
  } else {
    [mainMenu insertItem:langMenuItem atIndex:windowIndex + 1];
  }
  logf_("menu: Language menu added (current: %s)", CurrentLanguage().UTF8String);
}

- (void)refreshChecks {
  NSString *lang = CurrentLanguage();
  self.faItem.state = [lang isEqualToString:@"fa"] ? NSControlStateValueOn : NSControlStateValueOff;
  self.enItem.state = [lang isEqualToString:@"en"] ? NSControlStateValueOn : NSControlStateValueOff;
}

static void AddEnvIfSet(NSMutableDictionary<NSString *, NSString *> *env, NSString *key) {
  const char *v = getenv(key.UTF8String);
  if (v && *v) env[key] = [NSString stringWithUTF8String:v];
}

- (void)setLanguage:(NSString *)lang {
  if ([lang isEqualToString:CurrentLanguage()]) return;
  NSError *err = nil;
  [lang writeToFile:[NSString stringWithUTF8String:g_lang_file]
         atomically:YES
           encoding:NSUTF8StringEncoding
              error:&err];
  if (err) {
    logf_("menu: failed to write lang file: %s", err.localizedDescription.UTF8String);
    return;
  }
  SetFirebaseLocale(lang);
  [self refreshChecks];
  [self relaunchWithLanguage:lang];
}

- (void)relaunchWithLanguage:(NSString *)lang {
  NSURL *appURL = NSRunningApplication.currentApplication.bundleURL;
  if (!appURL) {
    logf_("menu: could not resolve Foresight's own bundle URL, cannot relaunch");
    return;
  }

  NSMutableDictionary<NSString *, NSString *> *env = [NSMutableDictionary dictionary];
  env[@"DYLD_INSERT_LIBRARIES"] = [NSString stringWithUTF8String:g_dylib_path];
  AddEnvIfSet(env, @"FARSIGHT_TRACE");
  AddEnvIfSet(env, @"FARSIGHT_FONT");
  AddEnvIfSet(env, @"FARSIGHT_HEADINGS");
  AddEnvIfSet(env, @"FARSIGHT_GEMMA_SAMPLING");

  NSMutableArray<NSString *> *args = [NSMutableArray arrayWithObject:@"-AppleLanguages"];
  if ([lang isEqualToString:@"fa"]) {
    [args addObjectsFromArray:@[ @"(fa)", @"-AppleLocale", @"fa_IR" ]];
  } else {
    [args addObject:[NSString stringWithFormat:@"(%@)", lang]];
  }

  NSWorkspaceOpenConfiguration *config = [NSWorkspaceOpenConfiguration configuration];
  config.environment = env;
  config.arguments = args;
  config.createsNewApplicationInstance = YES;

  logf_("menu: language set to %s, opening a new Foresight instance", lang.UTF8String);
  [NSWorkspace.sharedWorkspace openApplicationAtURL:appURL
                                       configuration:config
                                   completionHandler:^(NSRunningApplication *app, NSError *openErr) {
                                     if (openErr) {
                                       logf_("menu: relaunch failed: %s", openErr.localizedDescription.UTF8String);
                                       return;
                                     }
                                     logf_("menu: new Foresight instance running (pid %d), quitting this one",
                                           app.processIdentifier);
                                     dispatch_async(dispatch_get_main_queue(), ^{
                                       [NSApp terminate:nil];
                                     });
                                   }];
}

- (void)selectFarsi:(id)sender { [self setLanguage:@"fa"]; }
- (void)selectEnglish:(id)sender { [self setLanguage:@"en"]; }

@end

static FarsightMenuController *g_controller;

void persian_menu_init(const char *home) {
  snprintf(g_lang_file, sizeof g_lang_file, "%s/tools/farsight/lang", home);
  snprintf(g_dylib_path, sizeof g_dylib_path, "%s/tools/farsight/shim/libfarsight.dylib", home);
  g_controller = [FarsightMenuController new];
  [[NSNotificationCenter defaultCenter] addObserverForName:NSApplicationDidFinishLaunchingNotification
                                                     object:nil
                                                      queue:nil
                                                 usingBlock:^(NSNotification *note) {
                                                   [g_controller installMenu];
                                                 }];
}
