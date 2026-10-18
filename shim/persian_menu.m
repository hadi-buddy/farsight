// "زبان" (Language) menu for Foresight's own menu bar.
//
// The menu bar (Foresight/Edit/View/Window, …) is native AppKit even though
// Foresight's window content is Flutter (confirmed: `setTitle:` is a real
// selector in the binary, and the Flutter engine links AppKit only for the
// window/menu chrome around its own canvas — see CHANGELOG, "Menu-bar
// language switcher"). So unlike the window content, this part can be
// extended with ordinary, public NSMenu/NSMenuItem calls.
//
// On NSApplicationDidFinishLaunchingNotification, inserts a top-level "زبان"
// menu right after "Window", with فارسی/English items. Picking one writes the
// choice to ~/tools/farsight/lang (the launch.sh fallback added alongside
// this) and runs launch.sh in the background; launch.sh's own logic quits the
// running Foresight (which includes us) and reopens it with the right
// -AppleLanguages/-AppleLocale. We never call -terminate: ourselves, to avoid
// racing launch.sh's own quit-then-relaunch sequence.

#import <AppKit/AppKit.h>

extern void logf_(const char *fmt, ...);

static char g_lang_file[700];
static char g_launch_script[700];

static NSString *CurrentLanguage(void) {
  NSString *path = [NSString stringWithUTF8String:g_lang_file];
  NSString *saved = [[NSString stringWithContentsOfFile:path encoding:NSUTF8StringEncoding error:NULL]
      stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]];
  return (saved.length > 0) ? saved : @"fa";
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
    if ([item.submenu.title isEqualToString:@"زبان"]) return;  // already installed
  }

  NSMenuItem *langMenuItem = [[NSMenuItem alloc] initWithTitle:@"زبان" action:NULL keyEquivalent:@""];
  NSMenu *langMenu = [[NSMenu alloc] initWithTitle:@"زبان"];
  langMenuItem.submenu = langMenu;

  self.faItem = [[NSMenuItem alloc] initWithTitle:@"فارسی" action:@selector(selectFarsi:) keyEquivalent:@""];
  self.faItem.target = self;
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
  [self refreshChecks];
  logf_("menu: language set to %s, relaunching", lang.UTF8String);

  NSTask *task = [[NSTask alloc] init];
  task.executableURL = [NSURL fileURLWithPath:@"/bin/zsh"];
  task.arguments = @[ [NSString stringWithUTF8String:g_launch_script] ];
  @try {
    [task launch];
  } @catch (NSException *ex) {
    logf_("menu: failed to launch launch.sh: %s", ex.reason.UTF8String);
  }
}

- (void)selectFarsi:(id)sender { [self setLanguage:@"fa"]; }
- (void)selectEnglish:(id)sender { [self setLanguage:@"en"]; }

@end

static FarsightMenuController *g_controller;

void persian_menu_init(const char *home) {
  snprintf(g_lang_file, sizeof g_lang_file, "%s/tools/farsight/lang", home);
  snprintf(g_launch_script, sizeof g_launch_script, "%s/tools/farsight/launch.sh", home);
  g_controller = [FarsightMenuController new];
  [[NSNotificationCenter defaultCenter] addObserverForName:NSApplicationDidFinishLaunchingNotification
                                                     object:nil
                                                      queue:nil
                                                 usingBlock:^(NSNotification *note) {
                                                   [g_controller installMenu];
                                                 }];
}
