// Solar Hijri (Jalali) calendar conversion and the date line prepended to
// Persian summaries. See persian_date.h.

#include "persian_date.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Gregorian day-of-year at the start of each month (non-leap year).
static const int kGDaysInMonth[12] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};

void jalali_from_gregorian(int gy, int gm, int gd, int *jy, int *jm, int *jd) {
  int base_jy, base_gy;
  if (gy <= 1600) {
    base_jy = 0;
    base_gy = gy - 621;
  } else {
    base_jy = 979;
    base_gy = gy - 1600;
  }
  int gy2 = gm > 2 ? base_gy + 1 : base_gy;
  long days = 365L * base_gy + (gy2 + 3) / 4 - (gy2 + 99) / 100 + (gy2 + 399) / 400 - 80 + gd +
              kGDaysInMonth[gm - 1];
  base_jy += 33 * (days / 12053);
  days %= 12053;
  base_jy += 4 * (days / 1461);
  days %= 1461;
  if (days > 365) {
    base_jy += (days - 1) / 365;
    days = (days - 1) % 365;
  }
  if (days < 186) {
    *jm = 1 + days / 31;
    *jd = 1 + days % 31;
  } else {
    *jm = 7 + (days - 186) / 30;
    *jd = 1 + (days - 186) % 30;
  }
  *jy = base_jy;
}

static const char *const kJalaliMonths[12] = {"فروردین", "اردیبهشت", "خرداد",  "تیر",   "مرداد", "شهریور",
                                               "مهر",      "آبان",     "آذر",    "دی",    "بهمن",  "اسفند"};

// ASCII digits -> Persian digits (U+06F0..U+06F9, 2-byte UTF-8 each). Returns
// a malloc'ed string.
static char *persian_digits(int n) {
  char buf[16];
  snprintf(buf, sizeof buf, "%d", n);
  size_t len = strlen(buf);
  char *out = malloc(len * 2 + 1);
  size_t o = 0;
  for (size_t i = 0; i < len; i++) {
    unsigned cp = 0x06F0 + (unsigned)(buf[i] - '0');
    out[o++] = (char)(0xC0 | (cp >> 6));
    out[o++] = (char)(0x80 | (cp & 0x3F));
  }
  out[o] = 0;
  return out;
}

char *jalali_date_string(time_t t) {
  struct tm tmv;
  localtime_r(&t, &tmv);
  int jy, jm, jd;
  jalali_from_gregorian(tmv.tm_year + 1900, tmv.tm_mon + 1, tmv.tm_mday, &jy, &jm, &jd);
  char *dd = persian_digits(jd);
  char *yy = persian_digits(jy);
  size_t n = strlen(dd) + 1 + strlen(kJalaliMonths[jm - 1]) + 1 + strlen(yy) + 1;
  char *out = malloc(n);
  snprintf(out, n, "%s %s %s", dd, kJalaliMonths[jm - 1], yy);
  free(dd);
  free(yy);
  return out;
}

char *persian_summary_note(int en_headings, time_t t) {
  static const char fa_fmt[] =
      "\n\nLanguage: the transcript is in Persian (Farsi). Write all of your output in Persian (Farsi), "
      "including the markdown headings: write \"# مرور کلی\" instead of \"# Overview\" and "
      "\"# جزئیات\" instead of \"# Details\", and translate any other heading the same way. "
      "Keep any required JSON keys or field names exactly as specified, in English. "
      "Start your reply with today's date on its own line, written exactly as given here, then a "
      "blank line, before any heading: %s";
  static const char en_fmt[] =
      "\n\nLanguage: the transcript is in Persian (Farsi). Write all of your output in Persian (Farsi). "
      "Keep the template's markdown headings (such as \"# Overview\" and \"# Details\") and any "
      "required JSON keys or field names exactly as specified, in English. "
      "Start your reply with today's date on its own line, written exactly as given here, then a "
      "blank line, before any heading: %s";
  char *date = jalali_date_string(t);
  const char *fmt = en_headings ? en_fmt : fa_fmt;
  size_t n = strlen(fmt) + strlen(date) + 1;
  char *note = malloc(n);
  snprintf(note, n, fmt, date);
  free(date);
  return note;
}
