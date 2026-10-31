// Tests for persian_lexicon (lexicon correction + sound-alike respelling).
// Build and run: shim/test/itn_test.sh
//
// Loads the real shim/data/fa-words.txt, so these exercise the actual data
// the shim ships with, not a fixture.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../persian_lexicon.h"

static int fails;

static void check(const char *label, const char *got, const char *want) {
  int ok = strcmp(got, want) == 0;
  if (!ok) fails++;
  printf("%-14s got=[%s] want=[%s] %s\n", label, got, want, ok ? "OK" : "FAIL");
}

int main(void) {
  // shim/test/ -> repo root is two levels up.
  PersianWords *words = persian_words_load("../..");
  if (!words) { printf("could not load shim/data/fa-words.txt -- skipping\n"); return 0; }
  SoundIndex *idx = persian_sound_index_build("../..", 30);

  // Field case from the project doc: exact lexicon spelling is left alone,
  // the following count words are not swallowed into it.
  char *r;
  r = persian_correct_with_lexicon("\xd8\xaa\xdb\x8c\xd9\x85 \xd8\xb5\xd8\xa8\xd8\xa7\xd8\xa7\xdb\x8c\xd8\xaf\xd9\x87 \xdb\x8c\xda\xa9 \xd8\xaf\xd9\x88 \xd8\xb3\xd9\x87", words);
  check("exact-stays", r, "\xd8\xaa\xdb\x8c\xd9\x85 \xd8\xb5\xd8\xa8\xd8\xa7\xd8\xa7\xdb\x8c\xd8\xaf\xd9\x87 \xdb\x8c\xda\xa9 \xd8\xaf\xd9\x88 \xd8\xb3\xd9\x87");
  free(r);

  // A sound-alike misspelling of a lexicon entry gets corrected.
  r = persian_correct_with_lexicon("\xd8\xaa\xdb\x8c\xd9\x85 \xd8\xb3\xd8\xa8\xd8\xa7\xdb\x8c\xdb\x8c\xd8\xaf\xd9\x87", words);  // تیم سباییده
  check("misspelled", r, "\xd8\xaa\xdb\x8c\xd9\x85 \xd8\xb5\xd8\xa8\xd8\xa7\xd8\xa7\xdb\x8c\xd8\xaf\xd9\x87");  // تیم صباایده
  free(r);

  // Real Persian word that merely sounds similar to a lexicon entry
  // ("کمیته" ~ "کامیت") must NOT be force-corrected -- it's a known word.
  r = persian_correct_with_lexicon("\xda\xa9\xd9\x85\xdb\x8c\xd8\xaa\xd9\x87 \xd8\xa8\xd8\xb1\xda\xaf\xd8\xb2\xd8\xa7\xd8\xb1 \xd8\xb4\xd8\xaf", words);  // کمیته برگزار شد
  check("real-word-kept", r, "\xda\xa9\xd9\x85\xdb\x8c\xd8\xaa\xd9\x87 \xd8\xa8\xd8\xb1\xda\xaf\xd8\xb2\xd8\xa7\xd8\xb1 \xd8\xb4\xd8\xaf");
  free(r);

  if (idx) {
    // مطمئن misheard as متمئن -> corrected back via the frequency list.
    r = persian_fix_sound_alikes("\xd9\x85\xd8\xaa\xd9\x85\xd8\xa6\xd9\x86", words, idx);  // متمئن
    check("sound-alike", r, "\xd9\x85\xd8\xb7\xd9\x85\xd8\xa6\xd9\x86");  // مطمئن
    free(r);
    persian_sound_index_free(idx);
  } else {
    printf("could not build sound index -- skipping that check\n");
  }

  persian_words_free(words);
  if (!fails) printf("all passed\n");
  return fails != 0;
}
