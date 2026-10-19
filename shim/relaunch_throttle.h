#ifndef RELAUNCH_THROTTLE_H
#define RELAUNCH_THROTTLE_H
// Returns 1 if a relaunch may proceed now (and records this moment), 0 if
// one happened too recently. Shared by persian_menu.m and model_menu.m --
// both trigger the same NSWorkspace relaunch, and switching language then
// immediately switching model (or vice versa) should be throttled together,
// not per-menu.
int relaunch_throttle_ok(void);
#endif
