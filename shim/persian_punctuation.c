// Spoken punctuation <-> characters. Ported from Avan's punctuation.js.
//
// Only phrases that mean nothing else in a normal sentence are converted.
// Ambiguous words stay words («نقطه ضعف», «دو نقطه», «نقطه نظر») unless said
// with «علامت» in front. Longer/more specific phrases are tried before
// shorter ones at each position so «دات کام» wins over the bare «دات» rule.

#include "persian_punctuation.h"

#include <stdlib.h>
#include <string.h>

// Private-use placeholders for symbols the rest of the polish chain would
// otherwise treat specially. Restored to the real character at the end.
#define BACK "\xEE\x80\x80"  // U+E000, for \ (backslash)
#define FWD "\xEE\x80\x81"   // U+E001, for /
#define UND "\xEE\x80\x82"   // U+E002, for _
#define HASH "\xEE\x80\x83"  // U+E003, for #
#define DOT "\xEE\x80\x84"   // U+E004, for . inside an address

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

static int tin(Tok t, const char **list, int n) {
  for (int i = 0; i < n; i++)
    if (teq(t, list[i])) return 1;
  return 0;
}

typedef struct {
  const char *buf;
  size_t len, cap;
} Out;

static void out_init(Out *o, size_t hint) {
  o->cap = hint < 32 ? 32 : hint;
  o->buf = malloc(o->cap);
  o->len = 0;
}
static void out_put(Out *o, const char *s, size_t n) {
  if (o->len + n + 1 > o->cap) {
    while (o->len + n + 1 > o->cap) o->cap *= 2;
    o->buf = realloc((void *)o->buf, o->cap);
  }
  memcpy((void *)(o->buf + o->len), s, n);
  o->len += n;
}
static void out_str(Out *o, const char *s) { out_put(o, s, strlen(s)); }
static void out_tok(Out *o, Tok t) { out_put(o, t.s, t.len); }

// --- spoken -> symbol --------------------------------------------------------

char *persian_spoken_symbols(const char *text) {
  if (!text) return NULL;
  // Fast exit: none of the trigger words appear at all.
  if (!strstr(text, "\xd9\x86\xd9\x82\xd8\xb7\xd9\x87") &&     // نقطه
      !strstr(text, "\xd8\xb9\xd9\x84\xd8\xa7\xd9\x85\xd8\xaa") &&  // علامت
      !strstr(text, "\xd9\xbe\xd8\xb1\xd8\xa7\xd9\x86\xd8\xaa\xd8\xb2") &&  // پرانتز
      !strstr(text, "\xd8\xa7\xd8\xb3\xd9\x84\xd8\xb4") &&     // اسلش
      !strstr(text, "\xd9\x84\xd8\xa7\xd9\x8a\xd9\x86") &&     // لاین (also matches لاین inside آندرلاین)
      !strstr(text, "\xd8\xa7\xd8\xb3\xda\xa9\xd9\x88\xd8\xb1") &&  // اسکور
      !strstr(text, "\xd9\x87\xd8\xb4\xd8\xaa") &&             // هشت (هشتگ/هشتک)
      !strstr(text, "\xd8\xaf\xd8\xa7\xd8\xaa"))                // دات
    return strdup(text);

  Tok t[512];
  int n = tokenize(text, t, 512);
  Out o;
  out_init(&o, strlen(text) + 32);

  for (int i = 0; i < n; i++) {
    int consumed = 0;
    const char *repl = NULL;

    if (i + 1 < n && teq(t[i], "\xd9\x86\xd9\x82\xd8\xb7\xd9\x87") && teq(t[i + 1], "\xd9\x88\xdb\x8c\xd8\xb1\xda\xaf\xd9\x88\xd9\x84")) {
      repl = "\xd8\x9b";  // نقطه ویرگول -> ؛
      consumed = 2;
    } else if (i + 1 < n && teq(t[i], "\xd8\xb9\xd9\x84\xd8\xa7\xd9\x85\xd8\xaa")) {  // علامت ...
      Tok b = t[i + 1];
      static const char *QM[] = {"\xd8\xb3\xd9\x88\xd8\xa7\xd9\x84", "\xd8\xb3\xd8\xa4\xd8\xa7\xd9\x84", "\xd9\xbe\xd8\xb1\xd8\xb3\xd8\xb4"};  // سوال سؤال پرسش
      if (tin(b, QM, 3)) { repl = "\xd8\x9f"; consumed = 2; }  // ؟
      else if (teq(b, "\xd8\xaa\xd8\xb9\xd8\xac\xd8\xa8")) { repl = "!"; consumed = 2; }  // تعجب
      else if (teq(b, "\xd9\x88\xdb\x8c\xd8\xb1\xda\xaf\xd9\x88\xd9\x84")) { repl = "\xd8\x8c"; consumed = 2; }  // ویرگول -> ،
      else if (i + 2 < n && teq(b, "\xd8\xaf\xd9\x88") && teq(t[i + 2], "\xd9\x86\xd9\x82\xd8\xb7\xd9\x87")) { repl = ":"; consumed = 3; }  // دو نقطه
      else if (teq(b, "\xd9\x86\xd9\x82\xd8\xb7\xd9\x87")) { repl = "."; consumed = 2; }  // نقطه
    } else if (i + 1 < n && teq(t[i], "\xd9\xbe\xd8\xb1\xd8\xa7\xd9\x86\xd8\xaa\xd8\xb2")) {  // پرانتز ...
      if (teq(t[i + 1], "\xd8\xa8\xd8\xa7\xd8\xb2")) { repl = "("; consumed = 2; }  // باز
      else if (teq(t[i + 1], "\xd8\xa8\xd8\xb3\xd8\xaa\xd9\x87")) { repl = ")"; consumed = 2; }  // بسته
    } else if (i + 1 < n && teq(t[i], "\xd8\xa8\xda\xa9") && teq(t[i + 1], "\xd8\xa7\xd8\xb3\xd9\x84\xd8\xb4")) {  // بک اسلش
      repl = BACK;
      consumed = 2;
    } else if (teq(t[i], "\xd8\xa7\xd8\xb3\xd9\x84\xd8\xb4")) {  // اسلش
      repl = FWD;
      consumed = 1;
    } else if (teq(t[i], "\xd9\x87\xd8\xb4\xd8\xaa\xda\xaf") || teq(t[i], "\xd9\x87\xd8\xb4\xd8\xaa\xda\xa9")) {  // هشتگ هشتک
      repl = HASH;
      consumed = 1;
    } else if (teq(t[i], "\xd8\xa2\xd9\x86\xd8\xaf\xd8\xb1") || teq(t[i], "\xd8\xa7\xd9\x86\xd8\xaf\xd8\xb1")) {  // آندر اندر
      if (i + 1 < n && (teq(t[i + 1], "\xd9\x84\xd8\xa7\xdb\x8c\xd9\x86") || teq(t[i + 1], "\xd8\xa7\xd8\xb3\xda\xa9\xd9\x88\xd8\xb1"))) {  // لاین اسکور
        repl = UND;
        consumed = 2;
      }
    } else if (i + 1 < n && teq(t[i], "\xd8\xaf\xd8\xa7\xd8\xaa")) {  // دات ...
      Tok b = t[i + 1];
      if (teq(b, "\xda\xa9\xd8\xa7\xd9\x85")) { repl = DOT "com"; consumed = 2; }  // کام
      else if (teq(b, "\xd8\xa7\xd8\xb1\xda\xaf") || teq(b, "\xd8\xa7\xd9\x88\xd8\xb1\xda\xaf")) { repl = DOT "org"; consumed = 2; }  // ارگ اورگ
      else if (teq(b, "\xd9\x86\xd8\xaa")) { repl = DOT "net"; consumed = 2; }  // نت
      else if (i + 2 < n && (teq(b, "\xd8\xa2\xdb\x8c") || teq(b, "\xd8\xa7\xdb\x8c")) && teq(t[i + 2], "\xd8\xa7\xd9\x88")) { repl = DOT "io"; consumed = 3; }  // آی/ای او
      else if (i + 2 < n && (teq(b, "\xd8\xa2\xdb\x8c") || teq(b, "\xd8\xa7\xdb\x8c")) && teq(t[i + 2], "\xd8\xa2\xd8\xb1")) { repl = DOT "ir"; consumed = 3; }  // آی/ای آر
      else { repl = DOT; consumed = 1; }  // still growing ("دات" alone, more word follows)
    }

    if (repl) {
      if (o.len) out_str(&o, " ");
      out_str(&o, repl);
      i += consumed - 1;
    } else {
      if (o.len) out_str(&o, " ");
      out_tok(&o, t[i]);
    }
  }
  out_put(&o, "", 1);  // NUL
  return (char *)o.buf;
}

// --- placeholder -> symbol ---------------------------------------------------

char *persian_restore_symbols(const char *text) {
  if (!text) return NULL;
  if (!strstr(text, BACK) && !strstr(text, FWD) && !strstr(text, UND) && !strstr(text, HASH) && !strstr(text, DOT))
    return strdup(text);

  size_t L = strlen(text);
  Out o;
  out_init(&o, L + 16);
  for (size_t i = 0; i < L;) {
    if (i + 3 <= L && (unsigned char)text[i] == 0xEE && (unsigned char)text[i + 1] == 0x80) {
      unsigned char c = (unsigned char)text[i + 2];
      const char *sym = c == 0x80 ? "\\" : c == 0x81 ? "/" : c == 0x82 ? "_" : c == 0x83 ? "#" : c == 0x84 ? "." : NULL;
      if (sym) {
        // Drop a surrounding single space on either side, same as the JS version.
        if (o.len && o.buf[o.len - 1] == ' ') o.len--;
        out_str(&o, sym);
        i += 3;
        if (i < L && text[i] == ' ') i++;
        continue;
      }
    }
    out_put(&o, text + i, 1);
    i++;
  }
  out_put(&o, "", 1);
  return (char *)o.buf;
}
