// Persian orthography cleanup for transcripts.
//
//  1. Characters: Arabic yeh/alef maksura -> Persian yeh (ی), Arabic kaf -> Persian
//     kaf (ک), Arabic-Indic digits -> Persian digits.
//  2. Zero-width non-joiner (ZWNJ, U+200C) where Persian writing puts one but
//     speech recognition writes a space:
//       می / نمی + verb        «می شه»       -> «می‌شه»
//       word + ها / های / ...  «طرح های»     -> «طرح‌های»
//       word ending in ه + ای  «خانه ای»     -> «خانه‌ای»
//     Only whole space-separated tokens are joined, so «میز» or «هادی» are
//     never touched.

#include "persian_text.h"

#include <stdlib.h>
#include <string.h>

#define ZWNJ "\xE2\x80\x8C"

// --- step 1: character mapping ---------------------------------------------

static char *map_chars(const char *in) {
  size_t n = strlen(in);
  char *out = malloc(n + 1);  // every mapping keeps the UTF-8 length (2 bytes)
  size_t o = 0;
  for (size_t i = 0; i < n;) {
    unsigned char a = (unsigned char)in[i];
    if (i + 1 < n && (a == 0xD9 || a == 0xDA || a == 0xDB)) {
      unsigned char b = (unsigned char)in[i + 1];
      unsigned cp = ((a & 0x1F) << 6) | (b & 0x3F);
      unsigned m = cp;
      if (cp == 0x064A || cp == 0x0649) m = 0x06CC;           // ي ى -> ی
      else if (cp == 0x0643) m = 0x06A9;                       // ك -> ک
      else if (cp >= 0x0660 && cp <= 0x0669) m = cp - 0x0660 + 0x06F0;  // ٠-٩ -> ۰-۹
      out[o++] = (char)(0xC0 | (m >> 6));
      out[o++] = (char)(0x80 | (m & 0x3F));
      i += 2;
    } else {
      out[o++] = in[i++];
    }
  }
  out[o] = 0;
  return out;
}

// --- step 2: ZWNJ joins -----------------------------------------------------

typedef struct {
  const char *s;
  size_t len;
} Tok;

static int tok_is(Tok t, const char *w) { return t.len == strlen(w) && memcmp(t.s, w, t.len) == 0; }

// First code point of a token is an Arabic-script letter (U+0620..U+06FF, not a digit).
static int starts_persian_letter(Tok t) {
  if (t.len < 2) return 0;
  unsigned char a = (unsigned char)t.s[0], b = (unsigned char)t.s[1];
  if (a < 0xD8 || a > 0xDB) return 0;
  unsigned cp = ((a & 0x1F) << 6) | (b & 0x3F);
  if (cp >= 0x0660 && cp <= 0x0669) return 0;
  if (cp >= 0x06F0 && cp <= 0x06F9) return 0;
  return cp >= 0x0620;
}

// Last code point is a Persian letter.
static int ends_persian_letter(Tok t) {
  if (t.len < 2) return 0;
  Tok last = {t.s + t.len - 2, 2};
  return starts_persian_letter(last);
}

static int ends_with_heh(Tok t) {
  return t.len >= 2 && (unsigned char)t.s[t.len - 2] == 0xD9 && (unsigned char)t.s[t.len - 1] == 0x87;  // ه
}

static const char *kPlural[] = {"ها", "های", "هایی", "هایم", "هایت", "هایش", "هایمان", "هایتان", "هایشان", "هاست", "هامون", "هاتون", "هاشون"};

static int is_plural_suffix(Tok t) {
  for (size_t i = 0; i < sizeof kPlural / sizeof *kPlural; i++)
    if (tok_is(t, kPlural[i])) return 1;
  return 0;
}

static char *join_zwnj(const char *in) {
  size_t L = strlen(in);
  int cap = 16, n = 0;
  Tok *t = malloc(sizeof *t * cap);
  for (size_t p = 0; p < L;) {
    while (p < L && in[p] == ' ') p++;
    if (p >= L) break;
    size_t q = p;
    while (q < L && in[q] != ' ') q++;
    if (n == cap) t = realloc(t, sizeof *t * (cap *= 2));
    t[n].s = in + p;
    t[n].len = q - p;
    n++;
    p = q;
  }
  char *out = malloc(L * 2 + 16);
  size_t o = 0;
  for (int i = 0; i < n; i++) {
    int glue = 0;  // join this token to the previous one with ZWNJ instead of a space
    if (i > 0) {
      Tok prev = t[i - 1], cur = t[i];
      if ((tok_is(prev, "می") || tok_is(prev, "نمی")) && starts_persian_letter(cur)) glue = 1;
      else if (is_plural_suffix(cur) && ends_persian_letter(prev)) glue = 1;
      else if (tok_is(cur, "ای") && ends_with_heh(prev)) glue = 1;
    }
    if (i > 0) {
      if (glue) {
        memcpy(out + o, ZWNJ, 3);
        o += 3;
      } else {
        out[o++] = ' ';
      }
    }
    memcpy(out + o, t[i].s, t[i].len);
    o += t[i].len;
  }
  out[o] = 0;
  free(t);
  return out;
}

char *persian_fix(const char *text) {
  if (!text) return NULL;
  char *a = map_chars(text);
  char *b = join_zwnj(a);
  free(a);
  return b;
}
