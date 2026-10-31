// Lexicon correction and sound-alike respelling, ported from Avan's
// extension/asr/lexicon.js.

#include "persian_lexicon.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "persian_sound.h"
#include "strmap.h"

// Names and terms that should always come out spelled right: boosted during
// decoding (not yet wired up in Farsight) and used here for phonetic
// correction.
static const char *const kLexicon[] = {
    // companies, products, people's tools
    "\xd8\xb5\xd8\xa8\xd8\xa7\xd8\xa7\xdb\x8c\xd8\xaf\xd9\x87",           // صباایده
    "\xd8\xb5\xd8\xa8\xd8\xa7 \xd8\xa7\xdb\x8c\xd8\xaf\xd9\x87",          // صبا ایده
    "\xd9\x81\xdb\x8c\xd9\x84\xdb\x8c\xd9\x85\xd9\x88",                  // فیلیمو
    "\xd8\xa2\xd9\xbe\xd8\xa7\xd8\xb1\xd8\xa7\xd8\xaa",                  // آپارات
    "\xd9\x81\xdb\x8c\xd9\x84\xdb\x8c\xd9\x85\xd9\x88 \xd9\x85\xd8\xaf\xd8\xb1\xd8\xb3\xd9\x87",  // فیلیمو مدرسه
    "\xda\xa9\xd9\x84\xd9\x88\xd8\xaf",                                  // کلود
    "\xd8\xa2\xd9\x86\xd8\xaa\xd8\xb1\xd9\x88\xd9\xbe\xdb\x8c\xda\xa9",   // آنتروپیک
    "\xd8\xb4\xd9\x86\xd9\x88\xd8\xa7",                                  // شنوا
    "\xd8\xa2\xd9\x88\xd8\xa7\xd9\x86",                                  // آوان
    "\xd9\x88\xdb\x8c\xd8\xb1\xda\xaf\xd9\x88\xd9\x84",                  // ویرگول
    "\xda\xaf\xd9\x88\xda\xaf\xd9\x84",                                  // گوگل
    "\xd8\xa7\xd9\xbe\xd9\x84",                                          // اپل
    "\xd9\x85\xd8\xa7\xdb\x8c\xda\xa9\xd8\xb1\xd9\x88\xd8\xb3\xd8\xa7\xd9\x81\xd8\xaa",  // مایکروسافت
    "\xd9\x85\xd8\xaa\xd8\xa7",                                          // متا
    "\xd8\xa7\xd9\x88\xd9\xbe\xd9\x86\xe2\x80\x8c\xd8\xa7\xdb\x8c\xe2\x80\x8c\xd8\xa2\xdb\x8c",  // اوپن‌ای‌آی
    "\xd8\xac\xd9\x85\xd9\x86\xd8\xa7\xdb\x8c",                          // جمنای
    "\xda\x86\xd8\xaa\xe2\x80\x8c\xd8\xac\xdb\x8c\xe2\x80\x8c\xd9\xbe\xdb\x8c\xe2\x80\x8c\xd8\xaa\xdb\x8c",  // چت‌جی‌پی‌تی
    "\xd9\x87\xd8\xa7\xda\xaf\xdb\x8c\xd9\x86\xda\xaf\xe2\x80\x8c\xd9\x81\xdb\x8c\xd8\xb3",  // هاگینگ‌فیس
    "\xd9\x81\xdb\x8c\xda\xaf\xd9\x85\xd8\xa7",                          // فیگما
    "\xda\xaf\xdb\x8c\xd8\xaa\xe2\x80\x8c\xd9\x87\xd8\xa7\xd8\xa8",       // گیت‌هاب
    "\xd8\xa7\xdb\x8c\xd9\x86\xd8\xb3\xd8\xaa\xd8\xa7\xda\xaf\xd8\xb1\xd8\xa7\xd9\x85",  // اینستاگرام
    "\xd8\xaa\xd9\x84\xda\xaf\xd8\xb1\xd8\xa7\xd9\x85",                  // تلگرام
    "\xd9\x88\xd8\xa7\xd8\xaa\xd8\xb3\xd8\xa7\xd9\xbe",                  // واتساپ
    "\xdb\x8c\xd9\x88\xd8\xaa\xdb\x8c\xd9\x88\xd8\xa8",                  // یوتیوب
    "\xd9\x84\xdb\x8c\xd9\x86\xda\xa9\xd8\xaf\xdb\x8c\xd9\x86",          // لینکدین
    "\xd8\xa7\xd8\xb3\xd9\x84\xda\xa9",                                  // اسلک
    "\xd9\x86\xd9\x88\xd8\xb4\xd9\x86",                                  // نوشن
    "\xd8\xa7\xd9\x86\xd8\xaf\xd8\xb1\xd9\x88\xdb\x8c\xd8\xaf",          // اندروید
    "\xd8\xa2\xdb\x8c\xd9\x81\xd9\x88\xd9\x86",                          // آیفون
    "\xd9\x85\xda\xa9\xe2\x80\x8c\xd8\xa8\xd9\x88\xda\xa9",              // مک‌بوک
    "\xd9\x88\xdb\x8c\xd9\x86\xd8\xaf\xd9\x88\xd8\xb2",                  // ویندوز
    "\xda\xa9\xd8\xb1\xd9\x88\xd9\x85",                                  // کروم
    // design & tech vocabulary
    "\xd9\x87\xd9\x88\xd8\xb4 \xd9\x85\xd8\xb5\xd9\x86\xd9\x88\xd8\xb9\xdb\x8c",  // هوش مصنوعی
    "\xdb\x8c\xd8\xa7\xd8\xaf\xda\xaf\xdb\x8c\xd8\xb1\xdb\x8c \xd9\x85\xd8\xa7\xd8\xb4\xdb\x8c\xd9\x86",  // یادگیری ماشین
    "\xd9\x85\xd8\xaf\xd9\x84 \xd8\xb2\xd8\xa8\xd8\xa7\xd9\x86\xdb\x8c",  // مدل زبانی
    "\xd8\xaf\xdb\x8c\xd8\xb2\xd8\xa7\xdb\x8c\xd9\x86 \xd8\xb3\xdb\x8c\xd8\xb3\xd8\xaa\xd9\x85",  // دیزاین سیستم
    "\xd8\xb1\xd8\xa7\xd8\xa8\xd8\xb7 \xda\xa9\xd8\xa7\xd8\xb1\xd8\xa8\xd8\xb1\xdb\x8c",  // رابط کاربری
    "\xd8\xaa\xd8\xac\xd8\xb1\xd8\xa8\xd9\x87 \xda\xa9\xd8\xa7\xd8\xb1\xd8\xa8\xd8\xb1\xdb\x8c",  // تجربه کاربری
    "\xd9\xbe\xd8\xb1\xd9\x88\xd8\xaf\xd8\xa7\xda\xa9\xd8\xaa",          // پروداکت
    "\xd9\xbe\xd8\xb1\xd9\x88\xd8\xaf\xd8\xa7\xda\xa9\xd8\xaa \xd8\xaf\xdb\x8c\xd8\xb2\xd8\xa7\xdb\x8c\xd9\x86",  // پروداکت دیزاین
    "\xda\xa9\xd8\xa7\xd9\x85\xd9\xbe\xd9\x88\xd9\x86\xd9\x86\xd8\xaa",  // کامپوننت
    "\xd8\xaa\xd9\x88\xda\xa9\xd9\x86",                                  // توکن
    "\xd9\x81\xd8\xb1\xdb\x8c\xd9\x85\xe2\x80\x8c\xd9\x88\xd8\xb1\xda\xa9",  // فریم‌ورک
    "\xd9\xbe\xd8\xb1\xd8\xa7\xd9\x85\xd9\xbe\xd8\xaa",                  // پرامپت
    "\xda\xa9\xd8\xa7\xd9\x86\xd8\xaa\xda\xa9\xd8\xb3\xd8\xaa",          // کانتکست
    "\xd8\xa7\xda\xa9\xd8\xb3\xd8\xaa\xd9\x86\xd8\xb4\xd9\x86",          // اکستنشن
    "\xd8\xa7\xd9\xbe\xd9\x84\xdb\x8c\xda\xa9\xdb\x8c\xd8\xb4\xd9\x86",  // اپلیکیشن
    "\xd9\x88\xd8\xa8\xe2\x80\x8c\xd8\xb3\xd8\xa7\xdb\x8c\xd8\xaa",      // وب‌سایت
    "\xd8\xaf\xdb\x8c\xd8\xaa\xd8\xa7\xd8\xa8\xdb\x8c\xd8\xb3",          // دیتابیس
    "\xd8\xb3\xd8\xb1\xd9\x88\xd8\xb1",                                  // سرور
    "\xda\xa9\xd9\x84\xd8\xa7\xdb\x8c\xd9\x86\xd8\xaa",                  // کلاینت
    "\xd9\x81\xd8\xb1\xd8\xa7\xd9\x86\xd8\xaa\xe2\x80\x8c\xd8\xa7\xd9\x86\xd8\xaf",  // فرانت‌اند
    "\xd8\xa8\xda\xa9\xe2\x80\x8c\xd8\xa7\xd9\x86\xd8\xaf",              // بک‌اند
    "\xd8\xac\xd8\xa7\xd9\x88\xd8\xa7\xd8\xa7\xd8\xb3\xda\xa9\xd8\xb1\xdb\x8c\xd9\xbe\xd8\xaa",  // جاوااسکریپت
    "\xd8\xaa\xd8\xa7\xdb\x8c\xd9\xbe\xe2\x80\x8c\xd8\xa7\xd8\xb3\xda\xa9\xd8\xb1\xdb\x8c\xd9\xbe\xd8\xaa",  // تایپ‌اسکریپت
    "\xd9\xbe\xd8\xa7\xdb\x8c\xd8\xaa\xd9\x88\xd9\x86",                  // پایتون
    "\xd8\xb1\xdb\x8c\xe2\x80\x8c\xd8\xa7\xda\xa9\xd8\xaa",              // ری‌اکت
    "\xd8\xb1\xdb\x8c\xd9\xbe\xd8\xa7\xd8\xb2\xdb\x8c\xd8\xaa\xd9\x88\xd8\xb1\xdb\x8c",  // ریپازیتوری
    "\xda\xa9\xd8\xa7\xd9\x85\xdb\x8c\xd8\xaa",                          // کامیت
    "\xd8\xaf\xdb\x8c\xd9\xbe\xd9\x84\xd9\x88\xdb\x8c",                  // دیپلوی
    "\xd8\xaf\xdb\x8c\xda\xa9\xd8\xaa\xd9\x87",                          // دیکته
    "\xd8\xb1\xd8\xa7\xd8\xb3\xd8\xaa\xe2\x80\x8c\xda\x86\xdb\x8c\xd9\x86",  // راست‌چین
};
#define N_LEXICON (sizeof kLexicon / sizeof *kLexicon)

typedef struct {
  const char *w;
  CpString k;
  int n_words;  // word count, for span-length checks elsewhere
} Entry;

static Entry g_entries[N_LEXICON];
static int g_n_entries;
static int g_entries_built;

static int count_words(const char *s) {
  int n = 0;
  int in_word = 0;
  for (const char *p = s; *p; p++) {
    if (*p == ' ') { in_word = 0; }
    else if (!in_word) { n++; in_word = 1; }
  }
  return n;
}

static void build_entries(void) {
  if (g_entries_built) return;
  g_entries_built = 1;
  g_n_entries = 0;
  for (size_t i = 0; i < N_LEXICON; i++) {
    CpString k = persian_sound_key(kLexicon[i]);
    if (k.n < 5) { cp_free(&k); continue; }  // only 5+ sound-letter entries
    g_entries[g_n_entries].w = kLexicon[i];
    g_entries[g_n_entries].k = k;
    g_entries[g_n_entries].n_words = count_words(kLexicon[i]);
    g_n_entries++;
  }
}

// --- word list ---------------------------------------------------------------

struct PersianWords {
  StrMap *counts;
};

PersianWords *persian_words_load(const char *repo_dir) {
  char path[1024];
  snprintf(path, sizeof path, "%s/shim/data/fa-words.txt", repo_dir);
  FILE *f = fopen(path, "r");
  if (!f) return NULL;
  PersianWords *w = malloc(sizeof *w);
  w->counts = strmap_new(1 << 17);
  char *line = NULL;
  size_t cap = 0;
  ssize_t len;
  while ((len = getline(&line, &cap, f)) > 0) {
    while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) len--;
    if (len == 0) continue;
    if (len >= 7 && memcmp(line, "# total", 7) == 0) continue;
    // split on the LAST space (word may itself not contain a space in this file)
    ssize_t sp = -1;
    for (ssize_t i = len - 1; i >= 0; i--)
      if (line[i] == ' ') { sp = i; break; }
    if (sp <= 0) continue;
    long count = strtol(line + sp + 1, NULL, 10);
    CpString k = persian_lm_key_n(line, (size_t)sp);
    char *kenc = cp_encode(k.cp, k.n);
    strmap_set(w->counts, kenc, strlen(kenc), count);
    free(kenc);
    cp_free(&k);
  }
  free(line);
  fclose(f);
  return w;
}

void persian_words_free(PersianWords *w) {
  if (!w) return;
  strmap_free(w->counts);
  free(w);
}

int persian_words_has(const PersianWords *w, const char *word) {
  if (!w) return 0;
  CpString k = persian_lm_key(word);
  char *enc = cp_encode(k.cp, k.n);
  int found = strmap_get(w->counts, enc, strlen(enc), NULL);
  free(enc);
  cp_free(&k);
  return found;
}

// --- sound index ---------------------------------------------------------

// PersianWords' underlying map has no iteration API, so the index is built
// by re-reading fa-words.txt directly (same source) rather than walking the
// already-built StrMap.
struct SoundIndex {
  StrMap *best;  // sound key (encoded) -> packed (count << 32 | word index)
  char **words;
  size_t n_words, cap_words;
};

SoundIndex *persian_sound_index_build(const char *repo_dir, long min_count) {
  char path[1024];
  snprintf(path, sizeof path, "%s/shim/data/fa-words.txt", repo_dir);
  FILE *f = fopen(path, "r");
  if (!f) return NULL;
  SoundIndex *idx = malloc(sizeof *idx);
  idx->best = strmap_new(1 << 16);
  idx->cap_words = 1024;
  idx->words = malloc(sizeof *idx->words * idx->cap_words);
  idx->n_words = 0;

  char *line = NULL;
  size_t cap = 0;
  ssize_t len;
  while ((len = getline(&line, &cap, f)) > 0) {
    while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) len--;
    if (len == 0) continue;
    if (len >= 7 && memcmp(line, "# total", 7) == 0) continue;
    ssize_t sp = -1;
    for (ssize_t i = len - 1; i >= 0; i--)
      if (line[i] == ' ') { sp = i; break; }
    if (sp <= 0) continue;
    long count = strtol(line + sp + 1, NULL, 10);
    CpString wcp = cp_decode_n(line, (size_t)sp);
    if (count < min_count || wcp.n < 3) { cp_free(&wcp); continue; }
    cp_free(&wcp);
    CpString k = persian_sound_key_n(line, (size_t)sp);
    char *kenc = cp_encode(k.cp, k.n);
    if (idx->n_words == idx->cap_words) {
      idx->cap_words *= 2;
      idx->words = realloc(idx->words, sizeof *idx->words * idx->cap_words);
    }
    char *word_copy = malloc((size_t)sp + 1);
    memcpy(word_copy, line, (size_t)sp);
    word_copy[sp] = 0;
    idx->words[idx->n_words] = word_copy;

    // Keep the highest-count word per sound key: strmap stores count directly
    // (safe: counts fit in `long`), with a second pass tracking the word index.
    long prev_count;
    int had = strmap_get(idx->best, kenc, strlen(kenc), &prev_count);
    if (!had || count > (prev_count >> 32)) {
      long packed = (count << 32) | (long)idx->n_words;
      strmap_set(idx->best, kenc, strlen(kenc), packed);
    }
    idx->n_words++;
    free(kenc);
    cp_free(&k);
  }
  free(line);
  fclose(f);
  return idx;
}

void persian_sound_index_free(SoundIndex *idx) {
  if (!idx) return;
  strmap_free(idx->best);
  for (size_t i = 0; i < idx->n_words; i++) free(idx->words[i]);
  free(idx->words);
  free(idx);
}

static const char *sound_index_lookup(const SoundIndex *idx, const CpString *k) {
  char *kenc = cp_encode(k->cp, k->n);
  long packed;
  int found = strmap_get(idx->best, kenc, strlen(kenc), &packed);
  free(kenc);
  if (!found) return NULL;
  size_t wi = (size_t)(packed & 0xFFFFFFFFL);
  return idx->words[wi];
}

// --- correctWithLexicon ------------------------------------------------------

typedef struct {
  const char *s;
  size_t len;
} Tok;

static int tokenize(const char *in, Tok *t, int cap) {
  size_t L = strlen(in);
  int n = 0;
  for (size_t p = 0; p < L && n < cap;) {
    while (p < L && in[p] == ' ') p++;
    if (p >= L) break;
    size_t q = p;
    while (q < L && in[q] != ' ') q++;
    t[n].s = in + p;
    t[n].len = q - p;
    n++;
    p = q;
  }
  return n;
}

static int teq(Tok t, const char *w) { return t.len == strlen(w) && memcmp(t.s, w, t.len) == 0; }
static int contains_and(const Tok *t, int from, int span) {
  for (int i = from; i < from + span; i++)
    if (teq(t[i], "\xd9\x88")) return 1;  // و
  return 0;
}

static void join_tokens(const Tok *t, int from, int span, char *out, size_t cap) {
  size_t o = 0;
  for (int i = 0; i < span; i++) {
    if (i) out[o++] = ' ';
    memcpy(out + o, t[from + i].s, t[from + i].len);
    o += t[from + i].len;
  }
  out[o] = 0;
  (void)cap;
}

// Suffixes tried longest-first; the matched one must leave >=1 byte of prefix.
static const char *const kSuffixes[] = {
    "\xd9\x87\xd8\xa7\xdb\x8c\xdb\x8c",  // هایی (4)
    "\xd9\x87\xd8\xa7\xdb\x8c",          // های (3)
    "\xd9\x87\xd8\xa7\xd9\x85",          // هام (3)
    "\xd9\x87\xd8\xa7\xd8\xaa",          // هات (3)
    "\xd9\x87\xd8\xa7\xd8\xb4",          // هاش (3)
    "\xd9\x85\xd9\x88\xd9\x86",          // مون (3)
    "\xd8\xaa\xd9\x88\xd9\x86",          // تون (3)
    "\xd8\xb4\xd9\x88\xd9\x86",          // شون (3)
    "\xd9\x85\xd8\xa7\xd9\x86",          // مان (3)
    "\xd8\xaa\xd8\xa7\xd9\x86",          // تان (3)
    "\xd8\xb4\xd8\xa7\xd9\x86",          // شان (3)
    "\xd9\x87\xd8\xa7",                  // ها (2)
    "\xd8\xb1\xd9\x88",                  // رو (2)
    "\xd8\xb4",                          // ش (1)
    "\xd9\x85",                          // م (1)
    "\xd8\xaa",                          // ت (1)
    "\xd9\x88",                          // و (1)
};
#define N_SUFFIX (sizeof kSuffixes / sizeof *kSuffixes)

// Finds the longest suffix from kSuffixes at the end of `s` (length `len`
// bytes) leaving at least 1 byte of prefix. Returns suffix byte length, 0 if
// none.
static size_t find_suffix(const char *s, size_t len) {
  for (size_t i = 0; i < N_SUFFIX; i++) {
    size_t sl = strlen(kSuffixes[i]);
    if (sl < len && memcmp(s + len - sl, kSuffixes[i], sl) == 0) return sl;
  }
  return 0;
}

static size_t cp_len_of(const char *s, size_t len) {
  CpString c = cp_decode_n(s, len);
  size_t n = c.n;
  cp_free(&c);
  return n;
}

// Last space-separated word of a byte range.
static void last_word(const char *s, size_t len, const char **ws, size_t *wl) {
  size_t end = len;
  while (end > 0 && s[end - 1] == ' ') end--;
  size_t start = end;
  while (start > 0 && s[start - 1] != ' ') start--;
  *ws = s + start;
  *wl = end - start;
}

char *persian_correct_with_lexicon(const char *text, const PersianWords *words) {
  build_entries();
  if (!text) return NULL;
  Tok t[256];
  int n = tokenize(text, t, 256);
  if (n == 0) return strdup(text);

  size_t out_cap = strlen(text) * 2 + 64;
  char *out = malloc(out_cap);
  size_t o = 0;

  for (int i = 0; i < n;) {
    int best_span = 0;
    const char *best_w = NULL;
    double best_d = 1e18;
    char best_sfx[64] = "";

    for (int span = 1; span <= 3 && i + span <= n; span++) {
      if (span > 1 && contains_and(t, i, span)) continue;
      int all_known = 1;
      char chunk[512];
      {
        char tmp[512];
        join_tokens(t, i, span, tmp, sizeof tmp);
        strcpy(chunk, tmp);
      }
      for (int k = 0; k < span; k++) {
        char w[256];
        size_t wl = t[i + k].len < sizeof w - 1 ? t[i + k].len : sizeof w - 1;
        memcpy(w, t[i + k].s, wl);
        w[wl] = 0;
        if (!persian_words_has(words, w)) all_known = 0;
      }

      // form 1: whole chunk, no suffix. form 2 (if applicable): chunk minus a
      // trailing suffix glued to its last word.
      const char *forms_chunk[2];
      const char *forms_sfx[2];
      int n_forms = 1;
      forms_chunk[0] = chunk;
      forms_sfx[0] = "";
      size_t sl = find_suffix(chunk, strlen(chunk));
      if (sl) {
        size_t prefix_len = strlen(chunk) - sl;
        if (prefix_len > 0 && chunk[prefix_len - 1] != ' ') {
          CpString pk = persian_sound_key_n(chunk, prefix_len);
          const char *lw;
          size_t lwl;
          last_word(chunk, prefix_len, &lw, &lwl);
          if (pk.n >= 3 && cp_len_of(lw, lwl) >= 2) {
            static char prefix_buf[512];
            memcpy(prefix_buf, chunk, prefix_len);
            prefix_buf[prefix_len] = 0;
            forms_chunk[1] = prefix_buf;
            forms_sfx[1] = chunk + prefix_len;
            n_forms = 2;
          }
          cp_free(&pk);
        }
      }

      for (int fi = 0; fi < n_forms; fi++) {
        CpString k = persian_sound_key(forms_chunk[fi]);
        for (int e = 0; e < g_n_entries; e++) {
          if (k.n == 0 || g_entries[e].k.n == 0 || k.cp[0] != g_entries[e].k.cp[0]) continue;
          long diff = (long)k.n - (long)g_entries[e].k.n;
          if (diff < 0) diff = -diff;
          if (diff > 2) continue;
          int exact = strcmp(forms_chunk[fi], g_entries[e].w) == 0;
          if (all_known && !exact && !(k.n == g_entries[e].k.n && memcmp(k.cp, g_entries[e].k.cp, k.n * sizeof *k.cp) == 0)) continue;
          double thresh = (double)g_entries[e].k.n / 5.0;
          double d = exact ? -1 : persian_edit_distance(&k, &g_entries[e].k, thresh);
          if (d <= thresh && (best_span == 0 || d < best_d || (d == best_d && forms_sfx[fi][0] && !best_sfx[0]))) {
            best_span = span;
            best_w = g_entries[e].w;
            best_d = d;
            size_t sfxlen = strlen(forms_sfx[fi]);
            if (sfxlen >= sizeof best_sfx) sfxlen = sizeof best_sfx - 1;
            memcpy(best_sfx, forms_sfx[fi], sfxlen);
            best_sfx[sfxlen] = 0;
          }
        }
        cp_free(&k);
      }
    }

    if (o && o + 1 < out_cap) out[o++] = ' ';
    if (best_span) {
      const char *join = (best_sfx[0] && (unsigned char)best_sfx[0] == 0xD9 /* lead byte of ه = D9 87 */ &&
                           (unsigned char)best_sfx[1] == 0x87)
                              ? "\xE2\x80\x8C"  // ZWNJ
                              : "";
      size_t need = strlen(best_w) + strlen(join) + strlen(best_sfx) + 1;
      while (o + need > out_cap) { out_cap *= 2; out = realloc(out, out_cap); }
      o += (size_t)sprintf(out + o, "%s%s%s", best_w, join, best_sfx);
      i += best_span;
    } else {
      while (o + t[i].len + 1 > out_cap) { out_cap *= 2; out = realloc(out, out_cap); }
      memcpy(out + o, t[i].s, t[i].len);
      o += t[i].len;
      i++;
    }
  }
  out[o] = 0;
  return out;
}

// --- fixSoundAlikes ----------------------------------------------------------

static int is_pure_persian_token(const char *s, size_t len) {
  CpString c = cp_decode_n(s, len);
  int ok = c.n > 0;
  for (size_t i = 0; ok && i < c.n; i++) {
    unsigned cp = c.cp[i];
    if (!(cp == 0x200C || (cp >= 0x0600 && cp <= 0x06FF))) ok = 0;
  }
  cp_free(&c);
  return ok;
}

char *persian_fix_sound_alikes(const char *text, const PersianWords *words, const SoundIndex *idx) {
  if (!text) return NULL;
  if (!idx) return strdup(text);
  Tok t[256];
  int n = tokenize(text, t, 256);
  size_t cap = strlen(text) + 64;
  char *out = malloc(cap);
  size_t o = 0;
  for (int i = 0; i < n; i++) {
    const char *rep = NULL;
    char wbuf[256];
    size_t wl = t[i].len < sizeof wbuf - 1 ? t[i].len : sizeof wbuf - 1;
    memcpy(wbuf, t[i].s, wl);
    wbuf[wl] = 0;
    size_t cplen = cp_len_of(t[i].s, t[i].len);
    if (cplen >= 3 && is_pure_persian_token(t[i].s, t[i].len) && !persian_words_has(words, wbuf)) {
      CpString k = persian_sound_key(wbuf);
      rep = sound_index_lookup(idx, &k);
      cp_free(&k);
    }
    if (i && o + 1 < cap) out[o++] = ' ';
    const char *src = rep ? rep : wbuf;
    size_t srclen = strlen(src);
    while (o + srclen + 1 > cap) { cap *= 2; out = realloc(out, cap); }
    memcpy(out + o, src, srclen);
    o += srclen;
  }
  out[o] = 0;
  return out;
}
