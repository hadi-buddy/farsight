// "Reference Model" menu for Foresight's own menu bar.
//
// Same approach as persian_menu.m's "Language" menu (see its header comment
// for why the menu bar itself, unlike the Flutter window content, can be
// extended with ordinary NSMenu/NSMenuItem calls). Inserted right after the
// "Language" menu item (or after "Window" if that one isn't there yet).
//
// Lets the user pick which model Foresight's chat/summaries actually run on,
// without touching a terminal:
//   - "Foresight (Gemma)" -- the app's own built-in model, no bridge.
//   - Auto-detected local servers (Ollama :11434, LM Studio :1234, a
//     llama.cpp server on :8080) -- each one's installed models, probed live
//     each time the menu opens.
//   - "Add..." -- a small dialog for any other OpenAI-compatible server/model.
//
// Writes the exact same format shim/chat_bridge.c already reads from
// ~/tools/farsight/chat ("<base_url> [model]", or "off") -- chat_bridge.c
// itself needed no changes. Like the Language menu, chat_bridge_init() only
// reads that file once, at process startup (see its own comment), so picking
// a different model relaunches Foresight the same way, through the shared
// relaunch_foresight() helper (see relaunch.m for why it quits first instead
// of overlapping old/new instances). The relaunch also carries the current
// UI language (read from ~/tools/farsight/lang) so switching models never
// resets it.

#import <AppKit/AppKit.h>
#import "relaunch_throttle.h"
#import "relaunch.h"

extern void logf_(const char *fmt, ...);

static char g_chat_file[700];
static char g_lang_file[700];
static char g_dylib_path[700];

typedef struct {
  NSString *label;    // shown in the menu, e.g. "Ollama"
  NSString *baseURL;  // e.g. http://127.0.0.1:11434/v1
} LocalServer;

// Common local OpenAI-compatible server ports. Each is probed for
// {baseURL}/models on every menu open; unreachable ones are simply skipped
// (most users won't have all three running). LM Studio's port is
// user-editable in its own settings (Settings -> Local Model API -> Edit
// port) -- confirmed one install was actually on 1235, not the 1234 default
// -- so both are probed.
static LocalServer kLocalServers[] = {
    {@"Ollama", @"http://127.0.0.1:11434/v1"},
    {@"LM Studio", @"http://127.0.0.1:1234/v1"},
    {@"LM Studio", @"http://127.0.0.1:1235/v1"},
    {@"llama.cpp", @"http://127.0.0.1:8080/v1"},
};
static const int kLocalServerCount = 4;

// -------------------------------------------------------------- chat file ---

// Returns @{"url":..., "model":...} for the current non-off config, or nil
// for "off"/missing (Gemma). Mirrors chat_bridge_init()'s own parsing of the
// "<base_url> [model] [{extra}]" line, minus the extra-JSON part (the menu
// doesn't offer that -- still settable by hand in the file, same as before).
static NSDictionary<NSString *, NSString *> *CurrentChatConfig(void) {
  NSString *path = [NSString stringWithUTF8String:g_chat_file];
  NSString *line = [[NSString stringWithContentsOfFile:path encoding:NSUTF8StringEncoding error:NULL]
      stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]];
  if (line.length == 0 || [line isEqualToString:@"off"]) return nil;
  NSArray<NSString *> *parts = [line componentsSeparatedByCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];
  NSString *url = parts.firstObject ?: @"";
  NSString *model = (parts.count > 1 && ![parts[1] hasPrefix:@"{"]) ? parts[1] : @"";
  if (url.length == 0) return nil;
  return @{@"url" : url, @"model" : model};
}

static NSString *CurrentLanguage(void) {
  NSString *path = [NSString stringWithUTF8String:g_lang_file];
  NSString *saved = [[NSString stringWithContentsOfFile:path encoding:NSUTF8StringEncoding error:NULL]
      stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]];
  return (saved.length > 0) ? saved : @"fa";
}

// ------------------------------------------------------------- detection ---

// GETs {baseURL}/models with a short timeout and calls back on the main
// queue with the model ids found (possibly empty), or nil if the server
// didn't answer in time. Timeouts match chat_bridge.c's own health check
// (server_up()) so a menu click never hangs noticeably even with all three
// servers down, the common case.
static void ProbeServer(NSString *baseURL, void (^done)(NSArray<NSString *> *models)) {
  NSURL *url = [NSURL URLWithString:[baseURL stringByAppendingString:@"/models"]];
  NSMutableURLRequest *req = [NSMutableURLRequest requestWithURL:url];
  req.timeoutInterval = 0.8;
  NSURLSessionDataTask *task = [NSURLSession.sharedSession
      dataTaskWithRequest:req
        completionHandler:^(NSData *data, NSURLResponse *resp, NSError *err) {
          NSArray<NSString *> *models = nil;
          NSHTTPURLResponse *http = (NSHTTPURLResponse *)resp;
          if (!err && http.statusCode == 200 && data) {
            id j = [NSJSONSerialization JSONObjectWithData:data options:0 error:NULL];
            NSArray *items = [j isKindOfClass:NSDictionary.class] ? j[@"data"] : nil;
            if ([items isKindOfClass:NSArray.class]) {
              NSMutableArray<NSString *> *ids = [NSMutableArray array];
              for (id item in items) {
                NSString *mid = [item isKindOfClass:NSDictionary.class] ? item[@"id"] : nil;
                if ([mid isKindOfClass:NSString.class]) [ids addObject:mid];
              }
              models = ids;
            }
          }
          dispatch_async(dispatch_get_main_queue(), ^{
            done(models);
          });
        }];
  [task resume];
}

@interface FarsightModelMenuController : NSObject <NSMenuDelegate>
@property(nonatomic, strong) NSMenu *menu;
@property(nonatomic, strong) NSMenuItem *gemmaItem;
@property(nonatomic, strong) NSMutableArray<NSMenuItem *> *dynamicItems;  // detected models + "no servers" placeholder
@property(nonatomic, strong) NSMenuItem *customItem;  // shown only when the saved config isn't one we detected
- (void)installMenu;
@end

@implementation FarsightModelMenuController

- (instancetype)init {
  if ((self = [super init])) _dynamicItems = [NSMutableArray array];
  return self;
}

- (void)installMenu {
  NSMenu *mainMenu = NSApp.mainMenu;
  if (!mainMenu) {
    logf_("model-menu: no mainMenu yet, Reference Model menu not added");
    return;
  }
  for (NSMenuItem *item in mainMenu.itemArray) {
    if ([item.submenu.title isEqualToString:@"Reference Model"]) return;  // already installed
  }

  NSMenuItem *topItem = [[NSMenuItem alloc] initWithTitle:@"Reference Model" action:NULL keyEquivalent:@""];
  self.menu = [[NSMenu alloc] initWithTitle:@"Reference Model"];
  self.menu.delegate = self;
  topItem.submenu = self.menu;

  self.gemmaItem = [[NSMenuItem alloc] initWithTitle:@"Foresight (Gemma)"
                                               action:@selector(selectGemma:)
                                        keyEquivalent:@""];
  self.gemmaItem.target = self;
  [self.menu addItem:self.gemmaItem];
  [self.menu addItem:[NSMenuItem separatorItem]];
  NSMenuItem *addItem = [[NSMenuItem alloc] initWithTitle:@"Add…" action:@selector(selectAdd:) keyEquivalent:@""];
  addItem.target = self;
  [self.menu addItem:addItem];

  [self rebuildDynamicItems:@[]];  // placeholder until the first menuNeedsUpdate: probe finishes

  NSUInteger insertAfter = NSNotFound;
  for (NSUInteger i = 0; i < mainMenu.itemArray.count; i++) {
    NSString *t = mainMenu.itemArray[i].submenu.title ?: mainMenu.itemArray[i].title;
    if ([t isEqualToString:@"Language"]) {
      insertAfter = i;
      break;
    }
  }
  if (insertAfter == NSNotFound) {
    for (NSUInteger i = 0; i < mainMenu.itemArray.count; i++) {
      if ([mainMenu.itemArray[i].title isEqualToString:@"Window"]) {
        insertAfter = i;
        break;
      }
    }
  }
  if (insertAfter == NSNotFound) {
    [mainMenu addItem:topItem];
  } else {
    [mainMenu insertItem:topItem atIndex:insertAfter + 1];
  }
  logf_("model-menu: Reference Model menu added");
}

// Each entry: @{"label":"Ollama: llama3.1:8b", "url":..., "model":...}, or
// nil entries for a disabled "No local servers detected" placeholder.
- (void)rebuildDynamicItems:(NSArray<NSDictionary<NSString *, NSString *> *> *)entries {
  for (NSMenuItem *old in self.dynamicItems) [self.menu removeItem:old];
  if (self.customItem) {
    [self.menu removeItem:self.customItem];
    self.customItem = nil;
  }
  [self.dynamicItems removeAllObjects];

  NSUInteger insertAt = [self.menu indexOfItem:self.gemmaItem] + 1;

  if (entries.count == 0) {
    NSMenuItem *none = [[NSMenuItem alloc] initWithTitle:@"No local servers detected" action:NULL keyEquivalent:@""];
    none.enabled = NO;
    [self.menu insertItem:none atIndex:insertAt];
    [self.dynamicItems addObject:none];
  } else {
    for (NSDictionary<NSString *, NSString *> *e in entries) {
      NSMenuItem *item = [[NSMenuItem alloc] initWithTitle:e[@"label"]
                                                      action:@selector(selectModel:)
                                               keyEquivalent:@""];
      item.target = self;
      item.representedObject = e;
      [self.menu insertItem:item atIndex:insertAt++];
      [self.dynamicItems addObject:item];
    }
  }

  // The saved config might not be one just detected (its server isn't
  // running right now, or it was set by hand in the chat file with a port
  // none of the three probes cover). Show it anyway so the checkmark is
  // never just silently missing.
  NSDictionary<NSString *, NSString *> *current = CurrentChatConfig();
  if (current) {
    BOOL matched = NO;
    for (NSDictionary<NSString *, NSString *> *e in entries) {
      if ([e[@"url"] isEqualToString:current[@"url"]] && [e[@"model"] isEqualToString:current[@"model"]]) {
        matched = YES;
        break;
      }
    }
    if (!matched) {
      NSString *label = current[@"model"].length ? [NSString stringWithFormat:@"Custom: %@ (%@)", current[@"model"], current[@"url"]]
                                                   : [NSString stringWithFormat:@"Custom: %@", current[@"url"]];
      self.customItem = [[NSMenuItem alloc] initWithTitle:label action:@selector(selectModel:) keyEquivalent:@""];
      self.customItem.target = self;
      self.customItem.representedObject = current;
      [self.menu insertItem:self.customItem atIndex:[self.menu indexOfItem:self.gemmaItem] + 1];
    }
  }

  [self refreshChecks];
}

- (void)refreshChecks {
  NSDictionary<NSString *, NSString *> *current = CurrentChatConfig();
  self.gemmaItem.state = current ? NSControlStateValueOff : NSControlStateValueOn;
  for (NSMenuItem *item in self.dynamicItems) {
    NSDictionary<NSString *, NSString *> *e = item.representedObject;
    item.state = (current && e && [e[@"url"] isEqualToString:current[@"url"]] && [e[@"model"] isEqualToString:current[@"model"]])
                     ? NSControlStateValueOn
                     : NSControlStateValueOff;
  }
  if (self.customItem) self.customItem.state = NSControlStateValueOn;  // only shown when it IS the current config
}

// NSMenuDelegate: probes all three local servers each time the menu is
// about to open, then rebuilds. Async (NSURLSession, not libcurl's blocking
// calls like chat_bridge.c's own health check) so the menu never freezes
// waiting on an unreachable port.
- (void)menuNeedsUpdate:(NSMenu *)menu {
  __block NSInteger remaining = kLocalServerCount;
  NSMutableArray<NSDictionary<NSString *, NSString *> *> *found = [NSMutableArray array];
  for (int i = 0; i < kLocalServerCount; i++) {
    LocalServer server = kLocalServers[i];
    ProbeServer(server.baseURL, ^(NSArray<NSString *> *models) {
      for (NSString *mid in models) {
        [found addObject:@{
          @"label" : [NSString stringWithFormat:@"%@: %@", server.label, mid],
          @"url" : server.baseURL,
          @"model" : mid,
        }];
      }
      if (--remaining == 0) [self rebuildDynamicItems:found];
    });
  }
}

- (void)setChatLine:(NSString *)line {
  NSDictionary<NSString *, NSString *> *before = CurrentChatConfig();
  NSString *nowLine = before ? [NSString stringWithFormat:@"%@ %@", before[@"url"], before[@"model"]] : @"off";
  if ([line isEqualToString:nowLine] || (before == nil && [line isEqualToString:@"off"])) return;
  if (!relaunch_throttle_ok()) {
    logf_("model-menu: relaunch throttled (too soon after the last one), ignoring %s", line.UTF8String);
    return;
  }

  NSError *err = nil;
  [line writeToFile:[NSString stringWithUTF8String:g_chat_file] atomically:YES encoding:NSUTF8StringEncoding error:&err];
  if (err) {
    logf_("model-menu: failed to write chat file: %s", err.localizedDescription.UTF8String);
    return;
  }
  [self refreshChecks];
  [self relaunch];
}

- (void)selectGemma:(id)sender { [self setChatLine:@"off"]; }

- (void)selectModel:(NSMenuItem *)sender {
  NSDictionary<NSString *, NSString *> *e = sender.representedObject;
  NSString *line = e[@"model"].length ? [NSString stringWithFormat:@"%@ %@", e[@"url"], e[@"model"]] : e[@"url"];
  [self setChatLine:line];
}

- (void)selectAdd:(id)sender {
  NSAlert *alert = [[NSAlert alloc] init];
  alert.messageText = @"Add Reference Model";
  alert.informativeText = @"Base URL of an OpenAI-compatible server, and the model name.";
  [alert addButtonWithTitle:@"Add"];
  [alert addButtonWithTitle:@"Cancel"];

  NSView *accessory = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 300, 54)];
  NSTextField *urlField = [NSTextField textFieldWithString:@""];
  urlField.frame = NSMakeRect(0, 28, 300, 24);
  urlField.placeholderString = @"http://127.0.0.1:8080/v1";
  NSTextField *modelField = [NSTextField textFieldWithString:@""];
  modelField.frame = NSMakeRect(0, 0, 300, 24);
  modelField.placeholderString = @"model name (optional)";
  [accessory addSubview:urlField];
  [accessory addSubview:modelField];
  alert.accessoryView = accessory;
  [alert.window setInitialFirstResponder:urlField];

  if ([alert runModal] != NSAlertFirstButtonReturn) return;
  NSString *url = [urlField.stringValue stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];
  if (url.length == 0) return;
  NSString *model = [modelField.stringValue stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];
  [self setChatLine:model.length ? [NSString stringWithFormat:@"%@ %@", url, model] : url];
}

static void AddEnvIfSet(NSMutableDictionary<NSString *, NSString *> *env, NSString *key) {
  const char *v = getenv(key.UTF8String);
  if (v && *v) env[key] = [NSString stringWithUTF8String:v];
}

// Carries the current UI language along so switching models never resets it.
// See relaunch.m for the actual relaunch mechanism.
- (void)relaunch {
  NSString *bundleID = NSBundle.mainBundle.bundleIdentifier;
  if (bundleID) {
    NSString *savedStatePath =
        [NSString stringWithFormat:@"%@/Library/Saved Application State/%@.savedState", NSHomeDirectory(), bundleID];
    [NSFileManager.defaultManager removeItemAtPath:savedStatePath error:NULL];  // see persian_menu.m's DeleteSavedState
  }

  NSMutableDictionary<NSString *, NSString *> *env = [NSMutableDictionary dictionary];
  env[@"DYLD_INSERT_LIBRARIES"] = [NSString stringWithUTF8String:g_dylib_path];
  AddEnvIfSet(env, @"FARSIGHT_TRACE");
  AddEnvIfSet(env, @"FARSIGHT_FONT");
  AddEnvIfSet(env, @"FARSIGHT_HEADINGS");
  AddEnvIfSet(env, @"FARSIGHT_GEMMA_SAMPLING");

  NSString *lang = CurrentLanguage();
  NSMutableArray<NSString *> *args = [NSMutableArray arrayWithObject:@"-AppleLanguages"];
  if ([lang isEqualToString:@"fa"]) {
    [args addObjectsFromArray:@[ @"(fa)", @"-AppleLocale", @"fa_IR" ]];
  } else if (![lang isEqualToString:@"system"]) {
    [args addObject:[NSString stringWithFormat:@"(%@)", lang]];
  } else {
    args = [NSMutableArray array];
  }

  logf_("model-menu: reference model changed, relaunching");
  relaunch_foresight(env, args);
}

@end

static FarsightModelMenuController *g_controller;

void model_menu_init(const char *home) {
  snprintf(g_chat_file, sizeof g_chat_file, "%s/tools/farsight/chat", home);
  snprintf(g_lang_file, sizeof g_lang_file, "%s/tools/farsight/lang", home);
  snprintf(g_dylib_path, sizeof g_dylib_path, "%s/tools/farsight/shim/libfarsight.dylib", home);
  g_controller = [FarsightModelMenuController new];
  [[NSNotificationCenter defaultCenter] addObserverForName:NSApplicationDidFinishLaunchingNotification
                                                     object:nil
                                                      queue:nil
                                                 usingBlock:^(NSNotification *note) {
                                                   [g_controller installMenu];
                                                 }];
}
