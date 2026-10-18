#pragma once

#include <time.h>

// Converts a Gregorian calendar date to the Solar Hijri (Jalali/Persian) calendar.
// Public-domain algorithm (Kazimierz Borkowski), valid for any Gregorian date
// after year 1.
void jalali_from_gregorian(int gy, int gm, int gd, int *jy, int *jm, int *jd);

// Formats a point in time as a Jalali date in Persian ("<day> <month name> <year>",
// Persian digits, e.g. "۲۶ مهر ۱۴۰۴"). Returns a malloc'ed string.
char *jalali_date_string(time_t t);

// Builds the "write in Persian" instruction appended to a request whose
// transcript is Persian, prepending today's Jalali date (computed from t, in
// local time) to the summary. en_headings keeps the template's English
// headings (FARSIGHT_HEADINGS=en); otherwise the headings are translated to
// Persian. Returns a malloc'ed string starting with "\n\n" (use note+2 for a
// fresh message's own content). JSON keys stay in English either way.
char *persian_summary_note(int en_headings, time_t t);
