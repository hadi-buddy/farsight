#pragma once

// Rewrites spoken Persian numbers ("بیست و پنج") as Persian digits ("۲۵").
// Returns a malloc'ed string; the caller frees it. Words that are not part of a
// number pass through unchanged. Runs of spaces collapse to one.
char *persian_itn(const char *text);

// Latin digit characters -> Persian digits ("3" -> "۳", "10:30" -> "۱۰:۳۰"),
// except a run glued to or right after an English-looking token ("Python 3",
// "v2" stay as they are). Returns a malloc'ed string.
char *persian_itn_digits(const char *text);
