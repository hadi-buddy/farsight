// Shared sound-matching primitives, ported from Avan's extension/asr/lexicon.js.

#include "persian_sound.h"

#include <stdlib.h>
#include <string.h>

// --- UTF-8 <-> code points --------------------------------------------------

CpString cp_decode_n(const char *utf8, size_t n) {
  unsigned *cp = malloc(sizeof *cp * (n + 1));
  size_t m = 0;
  for (size_t i = 0; i < n;) {
    unsigned char a = (unsigned char)utf8[i];
    unsigned c;
    int len;
    if (a < 0x80) { c = a; len = 1; }
    else if ((a & 0xE0) == 0xC0 && i + 1 < n) { c = ((a & 0x1F) << 6) | ((unsigned char)utf8[i + 1] & 0x3F); len = 2; }
    else if ((a & 0xF0) == 0xE0 && i + 2 < n) {
      c = ((a & 0x0F) << 12) | (((unsigned char)utf8[i + 1] & 0x3F) << 6) | ((unsigned char)utf8[i + 2] & 0x3F);
      len = 3;
    } else if ((a & 0xF8) == 0xF0 && i + 3 < n) {
      c = ((a & 0x07) << 18) | (((unsigned char)utf8[i + 1] & 0x3F) << 12) | (((unsigned char)utf8[i + 2] & 0x3F) << 6) |
          ((unsigned char)utf8[i + 3] & 0x3F);
      len = 4;
    } else { c = a; len = 1; }  // invalid lead byte: pass through as-is
    cp[m++] = c;
    i += (size_t)len;
  }
  CpString s = {cp, m};
  return s;
}

CpString cp_decode(const char *utf8) { return cp_decode_n(utf8, strlen(utf8)); }

void cp_free(CpString *s) {
  free(s->cp);
  s->cp = NULL;
  s->n = 0;
}

static size_t utf8_len(unsigned c) {
  if (c < 0x80) return 1;
  if (c < 0x800) return 2;
  if (c < 0x10000) return 3;
  return 4;
}

static size_t utf8_put(char *o, unsigned c) {
  if (c < 0x80) { o[0] = (char)c; return 1; }
  if (c < 0x800) {
    o[0] = (char)(0xC0 | (c >> 6));
    o[1] = (char)(0x80 | (c & 0x3F));
    return 2;
  }
  if (c < 0x10000) {
    o[0] = (char)(0xE0 | (c >> 12));
    o[1] = (char)(0x80 | ((c >> 6) & 0x3F));
    o[2] = (char)(0x80 | (c & 0x3F));
    return 3;
  }
  o[0] = (char)(0xF0 | (c >> 18));
  o[1] = (char)(0x80 | ((c >> 12) & 0x3F));
  o[2] = (char)(0x80 | ((c >> 6) & 0x3F));
  o[3] = (char)(0x80 | (c & 0x3F));
  return 4;
}

char *cp_encode(const unsigned *cp, size_t n) {
  size_t cap = 1;
  for (size_t i = 0; i < n; i++) cap += utf8_len(cp[i]);
  char *o = malloc(cap);
  size_t w = 0;
  for (size_t i = 0; i < n; i++) w += utf8_put(o + w, cp[i]);
  o[w] = 0;
  return o;
}

// --- lm key (lighter: real alternate spellings only, not sound-alikes) ------

static unsigned map_lm(unsigned c) {
  switch (c) {
    case 0x64A: case 0x649: return 0x6CC;  // ي ى -> ی
    case 0x643: return 0x6A9;              // ك -> ک
    case 0x629: case 0x6C0: return 0x647;  // ة ۀ -> ه
    case 0x623: case 0x625: case 0x671: return 0x627;  // أ إ ٱ -> ا
    default: return c;
  }
}

static int is_lm_dropped(unsigned c) {
  if (c == 0x200C || c == 0x200D) return 1;  // ZWNJ ZWJ
  if (c >= 0x64B && c <= 0x652) return 1;    // diacritics ً-ْ
  if (c == 0x670) return 1;                  // ٰ superscript alef
  return 0;
}

CpString persian_lm_key_n(const char *word, size_t len) {
  CpString in = cp_decode_n(word, len);
  unsigned *out = malloc(sizeof *out * (in.n + 1));
  size_t m = 0;
  for (size_t i = 0; i < in.n; i++) {
    if (is_lm_dropped(in.cp[i])) continue;
    out[m++] = map_lm(in.cp[i]);
  }
  cp_free(&in);
  CpString s = {out, m};
  return s;
}

CpString persian_lm_key(const char *word) { return persian_lm_key_n(word, strlen(word)); }

// --- sound key ---------------------------------------------------------------

static unsigned map_sound(unsigned c) {
  switch (c) {
    case 0x62B: case 0x635: return 0x633;  // ث ص -> س
    case 0x630: case 0x636: case 0x638: return 0x632;  // ذ ض ظ -> ز
    case 0x637: return 0x62A;  // ط -> ت
    case 0x63A: return 0x642;  // غ -> ق
    case 0x62D: return 0x647;  // ح -> ه
    case 0x622: case 0x623: case 0x625: case 0x639: case 0x621: case 0x626: return 0x627;  // آ أ إ ع ء ئ -> ا
    case 0x64A: return 0x6CC;  // ي -> ی
    case 0x643: return 0x6A9;  // ك -> ک
    case 0x629: return 0x647;  // ة -> ه
    case 0x624: return 0x648;  // ؤ -> و
    default: return c;
  }
}

static int is_dropped(unsigned c) {
  if (c == 0x200C || c == ' ' || c == '\t' || c == '\n' || c == '\r') return 1;  // ZWNJ, whitespace
  if (c >= 0x64B && c <= 0x652) return 1;  // Arabic diacritics ً-ْ
  return 0;
}

CpString persian_sound_key_n(const char *word, size_t len) {
  CpString in = cp_decode_n(word, len);
  unsigned *out = malloc(sizeof *out * (in.n + 1));
  size_t m = 0;
  for (size_t i = 0; i < in.n; i++) {
    if (is_dropped(in.cp[i])) continue;
    out[m++] = map_sound(in.cp[i]);
  }
  cp_free(&in);
  CpString s = {out, m};
  return s;
}

CpString persian_sound_key(const char *word) { return persian_sound_key_n(word, strlen(word)); }

// --- edit distance -------------------------------------------------------

static int is_soft(unsigned c) { return c == 0x627 || c == 0x6CC || c == 0x648 || c == 0x647; }  // ا ی و ه

static int is_near(unsigned a, unsigned b) {
  // بو وب فو وف ای یا او وا سز زس
  static const unsigned pairs[][2] = {
      {0x628, 0x648}, {0x648, 0x628},  // ب و
      {0x641, 0x648}, {0x648, 0x641},  // ف و
      {0x627, 0x6CC}, {0x6CC, 0x627},  // ا ی
      {0x627, 0x648}, {0x648, 0x627},  // ا و
      {0x633, 0x632}, {0x632, 0x633},  // س ز
  };
  for (size_t i = 0; i < sizeof pairs / sizeof *pairs; i++)
    if (pairs[i][0] == a && pairs[i][1] == b) return 1;
  return 0;
}

double persian_edit_distance(const CpString *a, const CpString *b, double max) {
  size_t n = a->n, m = b->n;
  double diff = n > m ? (double)(n - m) : (double)(m - n);
  if (diff * 0.5 > max) return max + 1;

  double *prev = malloc(sizeof *prev * (m + 1));
  double *cur = malloc(sizeof *cur * (m + 1));
  prev[0] = 0;
  for (size_t j = 1; j <= m; j++) prev[j] = prev[j - 1] + (is_soft(b->cp[j - 1]) ? 0.5 : 1.0);
  for (size_t i = 1; i <= n; i++) {
    unsigned ca = a->cp[i - 1];
    double da = is_soft(ca) ? 0.5 : 1.0;
    cur[0] = prev[0] + da;
    double row_min = cur[0];
    for (size_t j = 1; j <= m; j++) {
      unsigned cb = b->cp[j - 1];
      double sub = ca == cb ? 0.0 : is_near(ca, cb) ? 0.5 : 1.0;
      double del = prev[j] + da;
      double ins = cur[j - 1] + (is_soft(cb) ? 0.5 : 1.0);
      double subc = prev[j - 1] + sub;
      double v = del < ins ? del : ins;
      if (subc < v) v = subc;
      cur[j] = v;
      if (v < row_min) row_min = v;
    }
    if (row_min > max) { free(prev); free(cur); return max + 1; }
    double *tmp = prev;
    prev = cur;
    cur = tmp;
  }
  double result = prev[m];
  free(prev);
  free(cur);
  return result;
}
