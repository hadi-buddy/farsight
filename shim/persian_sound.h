#pragma once

#include <stddef.h>

// Shared primitives for lexicon correction and English-term matching, ported
// from Avan's extension/asr/lexicon.js.

// UTF-8 decoded as an array of Unicode code points, for the codepoint-level
// (not byte-level) comparisons soundKey/editDistance need.
typedef struct {
  unsigned *cp;
  size_t n;
} CpString;

CpString cp_decode(const char *utf8);
// Same, for a byte range that may not be NUL-terminated (e.g. a token inside
// a larger buffer).
CpString cp_decode_n(const char *utf8, size_t len);
void cp_free(CpString *s);
// Re-encodes a code-point array back to a malloc'ed UTF-8 string.
char *cp_encode(const unsigned *cp, size_t n);

// Lookup key for the word-frequency list: Persian letter forms, diacritics
// and ZWNJ/ZWJ removed ("می‌کنم" == "میکنم"). Lighter than persian_sound_key:
// doesn't collapse letters that merely sound alike, only true alternate
// spellings of the same letter.
CpString persian_lm_key(const char *word);
CpString persian_lm_key_n(const char *word, size_t len);

// Collapses letters that sound alike in Persian to one (ث/ص->س, ذ/ض/ظ->ز,
// ط->ت, غ->ق, ح->ه, vowel-carrier/hamza forms ->ا, ي->ی, ك->ک, ة->ه, ؤ->و),
// drops spaces/ZWNJ and diacritics. "صبا ایده" and "سباییده" get the same key.
CpString persian_sound_key(const char *word);
// Same, for a byte range that may not be NUL-terminated.
CpString persian_sound_key_n(const char *word, size_t len);

// Weighted Levenshtein distance on sound keys: inserting/deleting a soft
// vowel-carrier (ا ی و ه) costs 0.5, substituting a near-sound pair (بو/وب,
// فو/وف, ای/یا, او/وا, سز/زس) costs 0.5, everything else costs 1. `max`
// bounds the search (returns a value > max, not necessarily exact, once the
// true distance is known to exceed it) — most comparisons are against
// far-off words, so this is the hot path and early-exit matters.
double persian_edit_distance(const CpString *a, const CpString *b, double max);
