#ifndef RELAUNCH_H
#define RELAUNCH_H
#import <Foundation/Foundation.h>
// Quits this Foresight instance, then opens a fresh one with the given
// environment and launch arguments. See relaunch.m for why this replaced the
// previous "launch new, then quit old" approach.
void relaunch_foresight(NSDictionary<NSString *, NSString *> *env, NSArray<NSString *> *args);
#endif
