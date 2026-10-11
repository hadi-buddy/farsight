// Replays a captured Foresight conversation through the chat bridge.
// usage: chat_harness <create_conversation_v2 config.json> <message.json> [<message.json>...]
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "../chat_bridge.h"

void logf_(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  fprintf(stderr, "[log] ");
  vfprintf(stderr, fmt, ap);
  fputc('\n', stderr);
  va_end(ap);
}

static char *slurp(const char *p) {
  FILE *f = fopen(p, "rb");
  if (!f) { perror(p); exit(1); }
  fseek(f, 0, SEEK_END);
  long n = ftell(f);
  fseek(f, 0, SEEK_SET);
  char *s = malloc(n + 1);
  s[fread(s, 1, n, f)] = 0;
  fclose(f);
  return s;
}

static void *fake(void) { fprintf(stderr, "real litert_lm call (fallback)\n"); return NULL; }
static volatile int done, nchunks;
static size_t nbytes;
static double t0, tfirst;
static double now(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec + t.tv_nsec / 1e9; }
static void cb(char *chunk, void *ud) {
  (void)ud;
  if (!chunk[0]) {
    printf("[end] ttft %.2fs total %.2fs chunks %d bytes %zu\n", tfirst - t0, now() - t0, nchunks, nbytes);
    done = 1;
  } else {
    if (!nchunks++) tfirst = now();
    nbytes += strlen(chunk);
    printf("%s\n", chunk);
  }
  fflush(stdout);
  free(chunk);
}

int main(int argc, char **argv) {
  if (argc < 3) return 2;
  chat_bridge_init(getenv("HOME"), 1);
  void *(*create)(void *, const char *) = chat_bridge_wrap("litert_lm_create_conversation_v2", (void *)fake);
  int (*send)(void *, const char *, void (*)(char *, void *), void *) =
      chat_bridge_wrap("litert_lm_conversation_send_message_stream", (void *)fake);
  void (*del)(void *) = chat_bridge_wrap("litert_lm_delete_conversation", (void *)fake);
  void *conv = create(NULL, slurp(argv[1]));
  if (!conv) return 1;
  for (int i = 2; i < argc; i++) {
    done = 0;
    nchunks = 0;
    nbytes = 0;
    t0 = now();
    send(conv, slurp(argv[i]), cb, NULL);
    while (!done) usleep(10000);
  }
  del(conv);
  return 0;
}
