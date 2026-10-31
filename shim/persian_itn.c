// Persian inverse text normalization for the Shenava ASR output.
//
// Shenava writes numbers as words ("بیست و پنج", "دو هزار و چهارصد"). This turns
// well-formed runs of number words into Persian digits ("۲۵", "۲۴۰۰") and leaves
// everything else as it is. A run follows Persian number grammar: components
// joined by "و" in falling order, optionally scaled by هزار / میلیون / میلیارد.
// Adjacent components without "و" ("دو سه", "a couple") end the run, so they are
// not summed. A lone «یک» («a») or «نه» («not») stays a word.

#include "persian_itn.h"

#include <stdlib.h>
#include <string.h>

typedef struct {
  const char *w;
  long long v;
} Word;

// Additive components. The order of a value is the size of the next smaller
// component that may follow it after "و" (e.g. after 300 a tens or units word).
static const Word kAdd[] = {
    {"صفر", 0},      {"یک", 1},       {"یه", 1},        {"دو", 2},        {"سه", 3},
    {"چهار", 4},     {"پنج", 5},      {"شش", 6},        {"شیش", 6},       {"هفت", 7},
    {"هشت", 8},      {"نه", 9},       {"ده", 10},       {"یازده", 11},    {"دوازده", 12},
    {"سیزده", 13},   {"چهارده", 14},  {"پانزده", 15},   {"پونزده", 15},   {"شانزده", 16},
    {"شونزده", 16},  {"هفده", 17},    {"هیفده", 17},    {"هجده", 18},     {"هیجده", 18},
    {"نوزده", 19},   {"بیست", 20},    {"سی", 30},       {"چهل", 40},      {"پنجاه", 50},
    {"شصت", 60},     {"هفتاد", 70},   {"هشتاد", 80},    {"نود", 90},      {"صد", 100},
    {"یکصد", 100},   {"دویست", 200},  {"سیصد", 300},    {"چهارصد", 400},  {"پانصد", 500},
    {"پونصد", 500},  {"ششصد", 600},   {"شیشصد", 600},   {"هفتصد", 700},   {"هشتصد", 800},
    {"نهصد", 900},
};
static const Word kScale[] = {{"هزار", 1000LL}, {"میلیون", 1000000LL}, {"میلیارد", 1000000000LL}};

static int lookup(const Word *t, int n, const char *s, size_t len, long long *v) {
  for (int i = 0; i < n; i++) {
    if (strlen(t[i].w) == len && memcmp(t[i].w, s, len) == 0) {
      *v = t[i].v;
      return 1;
    }
  }
  return 0;
}

// 0 = not a number word, 1 = additive, 2 = scale.
static int classify(const char *s, size_t len, long long *v) {
  if (lookup(kAdd, sizeof kAdd / sizeof *kAdd, s, len, v)) return 1;
  if (lookup(kScale, sizeof kScale / sizeof *kScale, s, len, v)) return 2;
  return 0;
}

// Largest component allowed after this one: hundreds -> 99, tens -> 9, else none.
static long long room_after(long long v) {
  if (v >= 100) return 99;
  if (v >= 20 && v % 10 == 0) return 9;
  return -1;
}

typedef struct {
  const char *s;
  size_t len;
} Tok;

static int is_and(Tok t) { return t.len == 2 && memcmp(t.s, "و", 2) == 0; }

static size_t put_digits(char *o, long long v) {
  char tmp[32];
  int n = 0;
  do {
    tmp[n++] = (char)('0' + v % 10);
    v /= 10;
  } while (v);
  size_t w = 0;
  while (n--) {
    // U+06F0 .. U+06F9 (EXTENDED ARABIC-INDIC DIGIT), UTF-8 DB B0..DB B9
    o[w++] = (char)0xDB;
    o[w++] = (char)(0xB0 + (tmp[n] - '0'));
  }
  return w;
}

// Parses the longest well-formed number starting at tokens[i]. Returns the
// number of tokens used (0 if none) and the value.
static int parse_number(const Tok *t, int n, int i, long long *value) {
  long long total = 0, cur = 0, last = -1, last_scale = 0;
  int used = 0, j = i, have_cur = 0, expect_add = 1;
  while (j < n) {
    long long v;
    int k = classify(t[j].s, t[j].len, &v);
    if (k == 1) {
      if (!expect_add) break;  // two components without "و"
      if (have_cur && (room_after(last) < 0 || v > room_after(last))) break;
      cur += v;
      last = v;
      have_cur = 1;
      expect_add = 0;
      used = j - i + 1;
      j++;
    } else if (k == 2) {
      if (last_scale && v >= last_scale) break;
      if (!have_cur && !expect_add) break;
      if (!have_cur && used) break;  // "و هزار"
      total += (have_cur ? cur : 1) * v;
      cur = 0;
      have_cur = 0;
      last = -1;
      last_scale = v;
      expect_add = 0;
      used = j - i + 1;
      j++;
    } else if (is_and(t[j]) && !expect_add && j + 1 < n) {
      long long nv;
      int nk = classify(t[j + 1].s, t[j + 1].len, &nv);
      if (nk == 0) break;
      expect_add = 1;
      if (nk == 1 && have_cur && (room_after(last) < 0 || nv > room_after(last))) break;
      if (nk == 1 && !have_cur && last_scale && nv >= last_scale) break;
      if (nk == 2 && !have_cur) break;
      j++;  // consume "و"; the next word is consumed by the loop
    } else {
      break;
    }
  }
  *value = total + cur;
  return used;
}

static int is_ascii_digit(unsigned char c) { return c >= '0' && c <= '9'; }
static int is_sword_char(unsigned char c) {  // A-Za-z0-9.+#-, the charset an English-ish token is made of
  return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || is_ascii_digit(c) || c == '.' || c == '+' || c == '#' || c == '-';
}
static int is_sword_letter(unsigned char c) { return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'); }

// Does the run of is_sword_char() bytes immediately before `end` (stopping at
// the first byte that isn't one) contain at least one A-Za-z letter? Mirrors
// JS /[A-Za-z][A-Za-z0-9.+#-]*$/.test(before): a trailing run with no letter
// in it (e.g. a bare "10") can't satisfy that pattern, one with a letter
// anywhere in the run can (the regex just starts its match at that letter).
static int trailing_run_has_letter(const char *text, size_t end) {
  size_t p = end;
  int has_letter = 0;
  while (p > 0 && is_sword_char((unsigned char)text[p - 1])) {
    if (is_sword_letter((unsigned char)text[p - 1])) has_letter = 1;
    p--;
  }
  return has_letter;
}

// Latin digits -> Persian digits ("1 2 3" -> "۱ ۲ ۳"), skipping a run glued to
// or right after an English-looking token ("Python 3", "v2", "0.74" after a
// name stays Latin). Numbers already spelled as words are handled separately
// by persian_itn(); this is for literal digit characters in the transcript.
char *persian_itn_digits(const char *text) {
  if (!text) return NULL;
  size_t L = strlen(text);
  char *out = malloc(L * 2 + 16);
  size_t o = 0;
  for (size_t i = 0; i < L;) {
    unsigned char c = (unsigned char)text[i];
    if (is_ascii_digit(c) && (i == 0 || !is_sword_char((unsigned char)text[i - 1]))) {
      size_t j = i;
      while (j < L && is_ascii_digit((unsigned char)text[j])) j++;
      // allow internal separators . , / : followed by more digits
      while (j < L && strchr(".,/:", text[j]) && j + 1 < L && is_ascii_digit((unsigned char)text[j + 1])) {
        j++;
        while (j < L && is_ascii_digit((unsigned char)text[j])) j++;
      }
      size_t end = j;
      int followed_by_alnum = end < L && (is_sword_letter((unsigned char)text[end]) || is_ascii_digit((unsigned char)text[end]));
      if (!followed_by_alnum) {
        size_t before_end = i;
        while (before_end > 0 && text[before_end - 1] == ' ') before_end--;
        int keep_latin = trailing_run_has_letter(text, before_end);
        if (!keep_latin) {
          for (size_t k = i; k < end; k++) {
            if (text[k] == '.') { out[o++] = (char)0xD9; out[o++] = (char)0xAB; }  // ٫ U+066B
            else if (is_ascii_digit((unsigned char)text[k])) { out[o++] = (char)0xDB; out[o++] = (char)(0xB0 + (text[k] - '0')); }
            else out[o++] = text[k];
          }
          i = end;
          continue;
        }
      }
    }
    out[o++] = text[i++];
  }
  out[o] = 0;
  return out;
}

char *persian_itn(const char *text) {
  if (!text) return NULL;
  size_t L = strlen(text);
  // Tokenize on ASCII spaces; keep the separators by position.
  int cap = 16, n = 0;
  Tok *t = malloc(sizeof *t * cap);
  for (size_t p = 0; p < L;) {
    while (p < L && text[p] == ' ') p++;
    if (p >= L) break;
    size_t q = p;
    while (q < L && text[q] != ' ') q++;
    if (n == cap) t = realloc(t, sizeof *t * (cap *= 2));
    t[n].s = text + p;
    t[n].len = q - p;
    n++;
    p = q;
  }
  // Digits never take more bytes than the words they replace, but leave room.
  char *out = malloc(L * 2 + 64);
  size_t o = 0;
  for (int i = 0; i < n;) {
    long long v = 0;
    int used = parse_number(t, n, i, &v);
    if (o) out[o++] = ' ';
    // A lone «یک» is usually the article "a" and a lone «نه» the word "not/no";
    // leave both as words. Longer runs («نه هزار», «بیست و نه») are numbers.
    int lone_word = used == 1 && (v == 1 || (v == 9 && t[i].len == 4 && memcmp(t[i].s, "نه", 4) == 0));
    if (used > 0 && !lone_word) {
      o += put_digits(out + o, v);
      i += used;
    } else {
      memcpy(out + o, t[i].s, t[i].len);
      o += t[i].len;
      i++;
    }
  }
  out[o] = 0;
  free(t);
  return out;
}
