#pragma once

// English-term substitution in Persian dictation output, ported from Avan's
// extension/asr/latin.js: specialised terms, product names and acronyms are
// written in English ("ریپو" -> repo, "جاوااسکریپت" -> JavaScript), and a
// handful of words that are ALSO ordinary Persian switch only inside a light-
// verb phrase ("پوش کن" -> "push کن", but "پول" alone stays Persian). Returns
// a malloc'ed string.
char *persian_to_latin(const char *text);
