// Tests for persian_itn and persian_itn_digits. Build and run: shim/test/itn_test.sh
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../persian_itn.h"

static int fails;

static void check(const char *in, const char *want) {
  char *got = persian_itn(in);
  if (strcmp(got, want) != 0) {
    printf("FAIL\n  in:   %s\n  want: %s\n  got:  %s\n", in, want, got);
    fails++;
  }
  free(got);
}

static void check_digits(const char *in, const char *want) {
  char *got = persian_itn_digits(in);
  if (strcmp(got, want) != 0) {
    printf("FAIL (digits)\n  in:   %s\n  want: %s\n  got:  %s\n", in, want, got);
    fails++;
  }
  free(got);
}

int main(void) {
  check("بیست و پنج", "۲۵");
  check("ساعت سه و نیم", "ساعت ۳ و نیم");
  check("دو هزار و چهارصد و پنجاه و شش تومان", "۲۴۵۶ تومان");
  check("سال هزار و چهارصد و سه", "سال ۱۴۰۳");
  check("دو میلیون و سیصد هزار", "۲۳۰۰۰۰۰");
  check("یک روز خوب", "یک روز خوب");
  check("یک میلیارد", "۱۰۰۰۰۰۰۰۰۰");
  check("دو سه نفر", "۲ ۳ نفر");
  check("صد و بیست", "۱۲۰");
  check("پنج و بیست", "۵ و ۲۰");
  check("سیصد و دوازده", "۳۱۲");
  check("هزار", "۱۰۰۰");
  check("من و تو", "من و تو");
  check("ده و", "۱۰ و");
  check("جلسه ی شماره ی هفت", "جلسه ی شماره ی ۷");
  check("یک اثر هنری نه به عنوان یک شیء", "یک اثر هنری نه به عنوان یک شیء");
  check("بیست و نه", "۲۹");
  check("نه هزار", "۹۰۰۰");
  check("", "");
  check("hello world", "hello world");

  // A figure Shenava already wrote as digits only joins a scale word right
  // next to it ("200 میلیون", "3 هزار") -- never as a free-standing "number
  // word" on its own.
  check("200 میلیون", "۲۰۰۰۰۰۰۰۰");
  check("۲۰۰ میلیون", "۲۰۰۰۰۰۰۰۰");
  check("3 هزار", "۳۰۰۰");
  check("200 نفر اومدن", "200 نفر اومدن");

  check_digits("3 تا سیب و 10 تا پرتقال", "۳ تا سیب و ۱۰ تا پرتقال");
  check_digits("ساعت 10:30 قرار داریم", "ساعت ۱۰:۳۰ قرار داریم");
  check_digits("Python 3 و بعد v2 رو هم تست کن", "Python 3 و بعد v2 رو هم تست کن");
  check_digits("نسخه v2.5 منتشر شد", "نسخه v2.5 منتشر شد");
  // No English token right before it, so it converts (unlike "v0.74" or "version 0.74").
  check_digits("قیمتش 0.74 دلاره", "قیمتش \xDB\xB0\xD9\xAB\xDB\xB7\xDB\xB4 دلاره");
  check_digits("version 0.74 منتشر شد", "version 0.74 منتشر شد");
  check_digits("", "");

  if (!fails) printf("all passed\n");
  return fails != 0;
}
