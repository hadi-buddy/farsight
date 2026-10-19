// Throttles how often persian_menu.m / model_menu.m may relaunch Foresight.
//
// A crash report (SIGSEGV/EXC_BAD_ACCESS, Foresight-2026-10-19-130322.ips)
// showed the crash was entirely inside Foresight's own code -- a Swift
// over-release in its SSOAuthPlugin/LocalhostRedirectAuthFlow sign-out path,
// not in anything of ours -- and happened mid-burst of four language
// switches inside about 40 seconds. [NSApp terminate:nil], called on the
// old instance once the new one is confirmed running, evidently runs into
// that pre-existing race when it's hit back-to-back. We can't fix a bug
// inside Foresight's closed-source Swift plugin, but we can stop triggering
// it so often: refuse a relaunch within this many seconds of the last one.
// This is also the likely cause of the duplicate-Dock-icon bug reported
// alongside it -- a crashed-instead-of-cleanly-quit old process can leave
// an orphaned Dock tile behind.

#include <time.h>

#define MIN_SECONDS_BETWEEN_RELAUNCHES 3

static time_t g_last_relaunch;

int relaunch_throttle_ok(void) {
  time_t now = time(NULL);
  if (g_last_relaunch && now - g_last_relaunch < MIN_SECONDS_BETWEEN_RELAUNCHES) return 0;
  g_last_relaunch = now;
  return 1;
}
