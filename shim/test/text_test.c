// Tests for persian_fix. Build and run: shim/test/itn_test.sh
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../persian_text.h"

#define Z "\xE2\x80\x8C"
static int fails;

static void check(const char *in, const char *want) {
  char *got = persian_fix(in);
  if (strcmp(got, want) != 0) {
    printf("FAIL\n  in:   %s\n  want: %s\n  got:  %s\n", in, want, got);
    fails++;
  }
  free(got);
}

int main(void) {
  check("می شه", "می" Z "شه");
  check("نمی تونیم", "نمی" Z "تونیم");
  check("طرح های پیشین", "طرح" Z "های پیشین");
  check("فیک ها", "فیک" Z "ها");
  check("خانه ای بزرگ", "خانه" Z "ای بزرگ");
  check("ای خدا", "ای خدا");
  check("می", "می");
  check("میز کار", "میز کار");
  check("هادی ها", "هادی" Z "ها");
  check("علي كتاب", "علی کتاب");
  check("سال ١٤٠٣", "سال ۱۴۰۳");
  check("ما به نتیجه رسیدیم که می تونیم", "ما به نتیجه رسیدیم که می" Z "تونیم");
  check("API ها", "API ها");
  check("", "");
  if (!fails) printf("text: all passed\n");
  return fails != 0;
}
