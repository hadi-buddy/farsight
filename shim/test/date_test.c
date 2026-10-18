// Tests for persian_date (Jalali conversion, formatting, and the date line
// prepended to Persian summaries). Build and run: shim/test/itn_test.sh
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../persian_date.h"

static int fails;

static void check_jalali(int gy, int gm, int gd, int wy, int wm, int wd) {
  int jy, jm, jd;
  jalali_from_gregorian(gy, gm, gd, &jy, &jm, &jd);
  if (jy != wy || jm != wm || jd != wd) {
    printf("FAIL jalali_from_gregorian(%04d-%02d-%02d)\n  want: %04d-%02d-%02d\n  got:  %04d-%02d-%02d\n", gy, gm,
           gd, wy, wm, wd, jy, jm, jd);
    fails++;
  }
}

// mktime() under TZ=UTC (set in main) makes this the same instant jalali_date_string's
// localtime_r would read back as gy-gm-gd, regardless of the host's real timezone.
static time_t mk(int y, int mo, int d) {
  struct tm tm = {0};
  tm.tm_year = y - 1900;
  tm.tm_mon = mo - 1;
  tm.tm_mday = d;
  tm.tm_hour = 12;
  return mktime(&tm);
}

static void check_string(time_t t, const char *want) {
  char *got = jalali_date_string(t);
  if (strcmp(got, want) != 0) {
    printf("FAIL jalali_date_string\n  want: %s\n  got:  %s\n", want, got);
    fails++;
  }
  free(got);
}

static void check_note(int en_headings, time_t t, const char *want_date) {
  char *note = persian_summary_note(en_headings, t);
  if (!strstr(note, want_date)) {
    printf("FAIL persian_summary_note(en_headings=%d): date %s not found\n  note: %s\n", en_headings, want_date,
           note);
    fails++;
  }
  if (strncmp(note, "\n\n", 2) != 0) {
    printf("FAIL persian_summary_note(en_headings=%d): should start with a blank line\n  note: %s\n", en_headings,
           note);
    fails++;
  }
  int has_fa_headings = strstr(note, "مرور کلی") && strstr(note, "جزئیات");
  int has_en_headings = strstr(note, "# Overview") && strstr(note, "# Details");
  if (en_headings && !has_en_headings) {
    printf("FAIL persian_summary_note(en_headings=1): should keep the English headings\n  note: %s\n", note);
    fails++;
  }
  if (!en_headings && !has_fa_headings) {
    printf("FAIL persian_summary_note(en_headings=0): should translate the headings\n  note: %s\n", note);
    fails++;
  }
  free(note);
}

int main(void) {
  setenv("TZ", "UTC", 1);
  tzset();

  // --- conversion (checked against well-known Gregorian/Jalali pairs) ---
  check_jalali(1970, 1, 1, 1348, 10, 11);   // Unix epoch
  check_jalali(2000, 1, 1, 1378, 10, 11);   // Y2K
  check_jalali(2024, 3, 20, 1403, 1, 1);    // Nowruz 1403
  check_jalali(2025, 3, 21, 1404, 1, 1);    // Nowruz 1404
  check_jalali(2026, 3, 21, 1405, 1, 1);    // Nowruz 1405
  check_jalali(1979, 2, 11, 1357, 11, 22);  // 22 Bahman (Islamic Revolution Day)
  check_jalali(2026, 10, 18, 1405, 7, 26);  // a plain mid-year date

  // --- formatting ---
  check_string(mk(2024, 3, 20), "۱ فروردین ۱۴۰۳");
  check_string(mk(1979, 2, 11), "۲۲ بهمن ۱۳۵۷");
  check_string(mk(2026, 10, 18), "۲۶ مهر ۱۴۰۵");

  // --- summary output: the date must actually be the session's date (not a
  // fixed/previous one), and heading language must stay independent of it ---
  check_note(0, mk(2026, 10, 18), "۲۶ مهر ۱۴۰۵");
  check_note(1, mk(2026, 10, 18), "۲۶ مهر ۱۴۰۵");
  check_note(0, mk(2025, 3, 21), "۱ فروردین ۱۴۰۴");

  if (!fails) printf("date: all passed\n");
  return fails != 0;
}
