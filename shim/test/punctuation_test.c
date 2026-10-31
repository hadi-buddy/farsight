// Tests for persian_punctuation (spoken symbols <-> characters).
// Build and run: shim/test/itn_test.sh
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../persian_punctuation.h"

static int fails;

static void check_spoken(const char *in, const char *want) {
  char *got = persian_spoken_symbols(in);
  if (strcmp(got, want) != 0) {
    printf("FAIL (spoken)\n  in:   %s\n  want: %s\n  got:  %s\n", in, want, got);
    fails++;
  }
  free(got);
}

// Round-trips spoken -> symbols -> restore, checking the final visible text.
static void check_roundtrip(const char *in, const char *want) {
  char *mid = persian_spoken_symbols(in);
  char *got = persian_restore_symbols(mid);
  if (strcmp(got, want) != 0) {
    printf("FAIL (roundtrip)\n  in:   %s\n  mid:  %s\n  want: %s\n  got:  %s\n", in, mid, want, got);
    fails++;
  }
  free(mid);
  free(got);
}

int main(void) {
  // Ambiguous phrases without "علامت" stay words.
  check_spoken("نقطه ضعف این کار چیه", "نقطه ضعف این کار چیه");
  check_spoken("دو نقطه ضعف داره", "دو نقطه ضعف داره");

  // "علامت" unlocks the symbol reading.
  check_roundtrip("علامت سوال", "؟");
  check_roundtrip("علامت تعجب", "!");
  check_roundtrip("علامت دو نقطه", ":");
  check_roundtrip("نقطه ویرگول", "؛");

  // Backslash/slash/underscore/hashtag and GitHub.com-style addresses.
  check_roundtrip("بک اسلش و اسلش و هشتگ و آندر لاین", "\\و/و#و_");
  check_roundtrip("گیت هاب دات کام", "گیت هاب.com");
  check_roundtrip("دات ارگ", ".org");
  check_roundtrip("دات آی او", ".io");

  if (!fails) printf("all passed\n");
  return fails != 0;
}
