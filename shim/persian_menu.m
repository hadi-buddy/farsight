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
#import "relaunch_throttle.h"

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

// The actual root cause of "menu says فارسی, app opens in English": macOS's
// own window/app state restoration ("Saved Application State") replays the
// previous session's UI, language included, over whatever this launch's
// locale actually is. Confirmed by testing: deleting
// ~/Library/Saved Application State/<bundle-id>.savedState *before* Foresight
// starts, then launching, fixed it; deleting it from inside Foresight's own
// startup code (the dylib constructor -- about as early as that process ever
// lets us run anything) did NOT fix it, even though that already runs before
// main(). Restoration is evidently applied by something before our
// constructor gets a chance to run at all -- so the delete has to happen in
// whatever process does the launching, before it launches, not in the
// launched process itself. Called from both places that actually spawn a new
// Foresight: launch.sh (added there too) and relaunchWithLanguage: below.
static NSString *SavedStatePath(void) {
  NSString *bundleID = NSBundle.mainBundle.bundleIdentifier;
  if (!bundleID) return nil;
  return [NSString stringWithFormat:@"%@/Library/Saved Application State/%@.savedState", NSHomeDirectory(),
                                     bundleID];
}

static void DeleteSavedState(void) {
  NSString *path = SavedStatePath();
  if (!path) return;
  NSError *rmErr = nil;
  BOOL removed = [NSFileManager.defaultManager removeItemAtPath:path error:&rmErr];
  logf_("menu: DeleteSavedState: removed = %d (%s)", removed, rmErr ? rmErr.localizedDescription.UTF8String : "ok");
}

// Also stops macOS from saving/restoring state in the first place, so
// there's nothing to restore on a launch that runs before DeleteSavedState()
// gets a chance to (a login item, a system-triggered relaunch after a crash,
// or -- the actual bug report -- a plain reopen after the user quit normally).
// NSQuitAlwaysKeepsWindows alone does NOT do this -- that key only controls
// the "keep windows when quitting" prompt/behavior. The key that actually
// disables AppKit's state-restoration machinery (both saving on quit AND
// restoring on next launch) is ApplePersistenceIgnoreState. Safe to call
// from within Foresight's own process since it only affects future
// quits/launches, never the one currently running.
static void DisableFutureStateSaving(void) {
  NSString *bundleID = NSBundle.mainBundle.bundleIdentifier;
  if (!bundleID) return;
  CFPreferencesSetAppValue(CFSTR("NSQuitAlwaysKeepsWindows"), kCFBooleanFalse, (__bridge CFStringRef)bundleID);
  CFPreferencesSetAppValue(CFSTR("ApplePersistenceIgnoreState"), kCFBooleanTrue, (__bridge CFStringRef)bundleID);
  CFPreferencesAppSynchronize((__bridge CFStringRef)bundleID);
}

// Foresight's own UI strings turned out not to follow -AppleLanguages at all --
// they come from wherever Foresight keys its translated content off of, and
// that's this cached locale (found by inspecting
// ~/Library/Preferences/<bundle-id>.plist and confirmed by writing it
// directly and watching the UI switch). It's a Firebase SDK key by name, but
// Foresight itself is evidently reading it for this. -AppleLanguages still
// gets passed too, for anything in the process that does follow the normal
// macOS mechanism (e.g. our own injected "Language" menu's text direction).
// Kept alongside DeleteSavedState() even though that one turned out to be
// the actual fix: harmless, and whatever reads this key (confirmed:
// Foresight's own Firebase init checks it and skips overwriting if already
// present) should still see our choice, not a stale one.
//
// Must run before Foresight's own startup code reads this key (observed: a
// write from installMenu, on NSApplicationDidFinishLaunchingNotification, was
// too late -- the UI had already rendered off the old value by then, since
// Foresight's own launch handling and ours both hang off that same
// notification with no guaranteed order). Called from persian_menu_init(),
// i.e. the dylib's __attribute__((constructor)), which runs at image-load
// time, before main() -- as early as this process gets. NSBundle.mainBundle
// rather than NSRunningApplication: no AppKit/NSApp dependency, so it's safe
// this early.
static void SetFirebaseLocale(NSString *lang) {
  NSString *bundleID = NSBundle.mainBundle.bundleIdentifier;
  if (!bundleID) {
    logf_("menu: SetFirebaseLocale(%s): NSBundle.mainBundle.bundleIdentifier is nil, cannot write",
          lang.UTF8String);
    return;
  }
  CFPreferencesSetAppValue(CFSTR("com.firebase.instanceid.user_defaults.locale"), (__bridge CFStringRef)lang,
                            (__bridge CFStringRef)bundleID);
  Boolean ok = CFPreferencesAppSynchronize((__bridge CFStringRef)bundleID);
  CFPropertyListRef readback =
      CFPreferencesCopyAppValue(CFSTR("com.firebase.instanceid.user_defaults.locale"), (__bridge CFStringRef)bundleID);

  // Also via NSUserDefaults: if anything in this same process already holds
  // [NSUserDefaults standardUserDefaults] with its own in-memory snapshot
  // (populated before this constructor ran), CFPreferencesSetAppValue alone
  // may not be visible to code reading through that instance. Covers both
  // possible read paths rather than assuming which one Foresight actually
  // uses.
  [NSUserDefaults.standardUserDefaults setObject:lang forKey:@"com.firebase.instanceid.user_defaults.locale"];
  [NSUserDefaults.standardUserDefaults synchronize];
  NSString *udReadback = [NSUserDefaults.standardUserDefaults stringForKey:@"com.firebase.instanceid.user_defaults.locale"];

  logf_("menu: SetFirebaseLocale(%s) bundle=%s sync=%d cf_readback=%s ud_readback=%s", lang.UTF8String,
        bundleID.UTF8String, ok, readback ? [(__bridge NSString *)readback UTF8String] : "(nil)",
        udReadback.UTF8String ?: "(nil)");
  if (readback) CFRelease(readback);
}

// -AppleLanguages/-AppleLocale passed as a launch *argument* only populates
// NSUserDefaults' process-local "argument domain" -- it does not survive
// Foresight re-executing itself shortly after a cold launch (confirmed: a
// second, unrelated-looking Foresight process with no argv at all, but the
// same DYLD_INSERT_LIBRARIES from our *environment*, consistently appears a
// few seconds after every launch, including a menu-triggered relaunch).
// Environment variables survive that re-exec; command-line arguments don't.
// Writing the real, on-disk `AppleLanguages`/`AppleLocale` preference under
// Foresight's own bundle ID means every process -- including the re-exec'd
// one -- reads the right language straight from disk, regardless of its argv.
static void SetAppleLanguagesPref(NSString *lang) {
  NSString *bundleID = NSBundle.mainBundle.bundleIdentifier;
  if (!bundleID) return;
  if ([lang isEqualToString:@"system"]) {
    CFPreferencesSetAppValue(CFSTR("AppleLanguages"), NULL, (__bridge CFStringRef)bundleID);
    CFPreferencesSetAppValue(CFSTR("AppleLocale"), NULL, (__bridge CFStringRef)bundleID);
  } else {
    NSArray<NSString *> *langs = @[ lang ];
    CFPreferencesSetAppValue(CFSTR("AppleLanguages"), (__bridge CFArrayRef)langs, (__bridge CFStringRef)bundleID);
    if ([lang isEqualToString:@"fa"]) {
      CFPreferencesSetAppValue(CFSTR("AppleLocale"), CFSTR("fa_IR"), (__bridge CFStringRef)bundleID);
    } else {
      CFPreferencesSetAppValue(CFSTR("AppleLocale"), NULL, (__bridge CFStringRef)bundleID);
    }
  }
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
  if (!relaunch_throttle_ok()) {
    logf_("menu: relaunch throttled (too soon after the last one), ignoring %s", lang.UTF8String);
    return;
  }
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
  SetAppleLanguagesPref(lang);
  [self refreshChecks];
  [self relaunchWithLanguage:lang];
}

- (void)relaunchWithLanguage:(NSString *)lang {
  NSURL *appURL = NSRunningApplication.currentApplication.bundleURL;
  if (!appURL) {
    logf_("menu: could not resolve Foresight's own bundle URL, cannot relaunch");
    return;
  }

  // Must happen here, in this (still-running, about-to-quit) process, before
  // the new instance is spawned below -- see DeleteSavedState()'s comment.
  DeleteSavedState();

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
  DisableFutureStateSaving();             // stops future saves; doesn't fix *this* launch -- see its comment
  SetFirebaseLocale(CurrentLanguage());  // as early as possible -- see SetFirebaseLocale's comment
  SetAppleLanguagesPref(CurrentLanguage());  // survives Foresight's own self-relaunch -- see its comment
  g_controller = [FarsightMenuController new];
  [[NSNotificationCenter defaultCenter] addObserverForName:NSApplicationDidFinishLaunchingNotification
                                                     object:nil
                                                      queue:nil
                                                 usingBlock:^(NSNotification *note) {
                                                   [g_controller installMenu];
                                                 }];
}
