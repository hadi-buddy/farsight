// Tests for persian_sound (sound key + weighted edit distance).
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../persian_sound.h"

static int fails;

static void check_key_eq(const char *a, const char *b) {
  CpString ka = persian_sound_key(a), kb = persian_sound_key(b);
  char *sa = cp_encode(ka.cp, ka.n), *sb = cp_encode(kb.cp, kb.n);
  if (strcmp(sa, sb) != 0) {
    printf("FAIL key_eq\n  a: %s -> %s\n  b: %s -> %s\n", a, sa, b, sb);
    fails++;
  }
  free(sa); free(sb); cp_free(&ka); cp_free(&kb);
}

static void check_dist(const char *a, const char *b, double max_expected_le) {
  CpString ka = persian_sound_key(a), kb = persian_sound_key(b);
  double d = persian_edit_distance(&ka, &kb, 10);
  if (!(d <= max_expected_le)) {
    printf("FAIL dist  a=%s b=%s got=%.2f want<=%.2f\n", a, b, d, max_expected_le);
    fails++;
  }
  cp_free(&ka); cp_free(&kb);
}

int main(void) {
  // "صبا ایده" ~ "سباییده": space/ZWNJ drop and ص->س collapse get them close,
  // but "اا" (word-join) vs "یی" still differs by one soft-vowel substitution.
  check_dist("صبا ایده", "سباییده", 0.5);

  // ریپازیتوری ~ ریپوزیتوری: one near-sound (ا/و) substitution.
  check_dist("ریپازیتوری", "ریپوزیتوری", 1.0);

  // متمئن -> مطمئن should be a cheap edit (ط/ت collapse via sound key makes these identical).
  check_key_eq("مطمئن", "متمئن");

  // Unrelated words should NOT be considered close.
  check_dist("سلام", "خداحافظ", 100);  // sanity: just confirms it doesn't crash / blow up
  {
    CpString a = persian_sound_key("سلام"), b = persian_sound_key("گیت‌هاب");
    double d = persian_edit_distance(&a, &b, 1.5);
    if (d <= 1.5) { printf("FAIL: unrelated words matched within 1.5 (d=%.2f)\n", d); fails++; }
    cp_free(&a); cp_free(&b);
  }

  if (!fails) printf("all passed\n");
  return fails != 0;
}
