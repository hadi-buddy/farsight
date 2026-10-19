// Shared relaunch helper for persian_menu.m's Language menu and
// model_menu.m's Reference Model menu.
//
// Previous approach (NSWorkspace openApplicationAtURL with
// createsNewApplicationInstance = YES, quitting the old instance only once
// the new one's completion handler confirmed it was running): deliberately
// overlapped the old and new instances for a moment, specifically to avoid
// a window with no Foresight running at all. That overlap turned out to be
// the actual cause of two separate reported bugs: (1) the crash report
// evidence from 2026-10-19 (SIGSEGV inside Foresight's own
// SSOAuthPlugin.signOut) happening mid-relaunch, and (2) every relaunch
// leaving an extra icon in the Dock's "recently used" section even with
// Foresight pinned -- macOS evidently doesn't merge a freshly-opened
// (createsNewApplicationInstance = YES) instance into an already-pinned
// icon's slot while the old instance is still around, so each switch grew
// the recents list instead of reusing the pinned tile.
//
// This version does the opposite: quit first, confirmed gone, *then* open a
// new instance -- so there is never a moment with two instances, and a
// relaunch always lands in the single pinned Dock slot like a normal app
// restart. The actual `open` call runs from a detached helper process
// (spawned via NSTask, not a child that dies with us) with a short delay,
// since our own process can't reliably do anything -- including launch
// another process -- once it starts quitting.
#import <AppKit/AppKit.h>
#import "relaunch.h"

extern void logf_(const char *fmt, ...);

void relaunch_foresight(NSDictionary<NSString *, NSString *> *env, NSArray<NSString *> *args) {
  NSURL *appURL = NSRunningApplication.currentApplication.bundleURL;
  if (!appURL) {
    logf_("relaunch: could not resolve Foresight's own bundle URL, cannot relaunch");
    return;
  }

  NSMutableArray<NSString *> *openArgs = [NSMutableArray arrayWithObjects:@"-n", @"-a", appURL.path, nil];
  for (NSString *key in env) {
    [openArgs addObject:@"--env"];
    [openArgs addObject:[NSString stringWithFormat:@"%@=%@", key, env[key]]];
  }
  if (args.count) {
    [openArgs addObject:@"--args"];
    [openArgs addObjectsFromArray:args];
  }

  // "$@" (not manual quoting) passes each NSTask argument through to `open`
  // exactly as given, however it's spelled -- no shell-escaping to get wrong.
  // 1.5s: Foresight's own quit (including whatever triggers its sign-out
  // crash) isn't always instant; this is a best-effort margin, not a proof
  // we're gone, but it only has to land after -- not race -- our own exit.
  NSMutableArray<NSString *> *shellArgs =
      [NSMutableArray arrayWithObjects:@"-c", @"sleep 1.5 && exec /usr/bin/open \"$@\"", @"sh", nil];
  [shellArgs addObjectsFromArray:openArgs];

  NSTask *task = [[NSTask alloc] init];
  task.executableURL = [NSURL fileURLWithPath:@"/bin/sh"];
  task.arguments = shellArgs;
  NSError *err = nil;
  BOOL ok = [task launchAndReturnError:&err];
  if (!ok) {
    logf_("relaunch: failed to spawn detached relaunch helper: %s", err.localizedDescription.UTF8String);
    return;
  }
  logf_("relaunch: spawned detached relaunch helper, quitting now");
  dispatch_async(dispatch_get_main_queue(), ^{
    [NSApp terminate:nil];
  });
}
