// Integration test: the same call sequence sensevoice_engine.c's
// postprocess() now runs (that function itself is file-static, so this
// mirrors it rather than calling it directly). Exercises the real
// shim/data/fa-words.txt, not a fixture.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../persian_itn.h"
#include "../persian_latin.h"
#include "../persian_lexicon.h"
#include "../persian_punctuation.h"
#include "../persian_text.h"

static char *postprocess(const char *raw, PersianWords *words, SoundIndex *idx) {
  char *s = persian_spoken_symbols(raw);
  char *s2 = persian_normalize_chars(s); free(s); s = s2;
  if (words) {
    s2 = persian_correct_with_lexicon(s, words); free(s); s = s2;
    if (idx) { s2 = persian_fix_sound_alikes(s, words, idx); free(s); s = s2; }
  }
  s2 = persian_itn(s); free(s); s = s2;
  s2 = persian_itn_digits(s); free(s); s = s2;
  s2 = persian_to_latin(s); free(s); s = s2;
  s2 = persian_fix(s); free(s); s = s2;
  s2 = persian_punctuate(s, 1); free(s); s = s2;
  s2 = persian_restore_symbols(s); free(s); s = s2;
  return s;
}

static int fails;
static void check(const char *label, const char *got, const char *want) {
  int ok = strcmp(got, want) == 0;
  if (!ok) fails++;
  printf("%-14s got=[%s] want=[%s] %s\n", label, got, want, ok ? "OK" : "FAIL");
}

int main(void) {
  PersianWords *words = persian_words_load("../..");
  if (!words) { printf("could not load shim/data/fa-words.txt -- skipping\n"); return 0; }
  SoundIndex *idx = persian_sound_index_build("../..", 30);

  // "گیت هاب" (plain space) sounds identical to the lexicon's "گیت‌هاب" (ZWNJ)
  // and gets corrected to the ZWNJ spelling; toLatin then turns that single
  // ZWNJ-joined token into "GitHub"; the spelled-out number becomes a digit;
  // the sentence gets a final "."
  char *r = postprocess(
      "\xDA\xAF\xDB\x8C\xD8\xAA \xD9\x87\xD8\xA7\xD8\xA8 \xD8\xB1\xD9\x88 \xDA\x86\xDA\xA9 \xDA\xA9\xD9\x86 \xD9\x88 \xD8\xA8\xDB\x8C\xD8\xB3\xD8\xAA \xD9\x88 \xD9\xBE\xD9\x86\xD8\xAC \xD8\xAA\xD8\xA7 \xD9\x81\xD8\xA7\xDB\x8C\xD9\x84 \xD8\xB3\xD8\xA7\xD8\xAE\xD8\xAA\xD9\x85",
      words, idx);  // گیت هاب رو چک کن و بیست و پنج تا فایل ساختم
  check("github-to-latin", r,
        "GitHub \xD8\xB1\xD9\x88 \xDA\x86\xDA\xA9 \xDA\xA9\xD9\x86 \xD9\x88 \xDB\xB2\xDB\xB5 \xD8\xAA\xD8\xA7 \xD9\x81\xD8\xA7\xDB\x8C\xD9\x84 \xD8\xB3\xD8\xA7\xD8\xAE\xD8\xAA\xD9\x85.");
  free(r);

  // "دات کام" with lexicon/number/toLatin steps around it should still
  // roundtrip to ".com" -- and, now that toLatin runs too, "گیت هاب" itself
  // becomes "GitHub" rather than just the ZWNJ-corrected Persian spelling.
  r = postprocess(
      "\xDA\xAF\xDB\x8C\xD8\xAA \xD9\x87\xD8\xA7\xD8\xA8 \xD8\xAF\xD8\xA7\xD8\xAA \xDA\xA9\xD8\xA7\xD9\x85",
      words, idx);  // گیت هاب دات کام
  check("github-dot-com", r, "GitHub.com.");
  free(r);

  persian_sound_index_free(idx);
  persian_words_free(words);
  if (!fails) printf("all passed\n");
  return fails != 0;
}
