// English-term substitution in Persian dictation output, ported from Avan's
// extension/asr/latin.js.

#include "persian_latin.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "persian_sound.h"

// [English, Persian spelling]. Several rows share an English term when
// Shenava has more than one way of spelling it. Generated (and round-trip
// verified against the original Persian text) from Avan's LATIN array, not
// hand-transcribed.
typedef struct {
  const char *en;
  const char *fa;
} LatinRow;

static const LatinRow kLatin[] = {
    {"Claude", "\xda\xa9\xd9\x84\xd9\x88\xd8\xaf"},  // کلود
    {"Anthropic", "\xd8\xa2\xd9\x86\xd8\xaa\xd8\xb1\xd9\x88\xd9\xbe\xdb\x8c\xda\xa9"},  // آنتروپیک
    {"Anthropic", "\xd8\xa7\xd9\x86\xd8\xaa\xd8\xb1\xd9\x88\xd9\xbe\xdb\x8c\xda\xa9"},  // انتروپیک
    {"OpenAI", "\xd8\xa7\xd9\x88\xd9\xbe\xd9\x86\xe2\x80\x8c\xd8\xa7\xdb\x8c\xe2\x80\x8c\xd8\xa2\xdb\x8c"},  // اوپن‌ای‌آی
    {"OpenAI", "\xd8\xa7\xd9\x88\xd9\xbe\xd9\x86\x20\xd8\xa7\xdb\x8c\x20\xd8\xa2\xdb\x8c"},  // اوپن ای آی
    {"ChatGPT", "\xda\x86\xd8\xaa\xe2\x80\x8c\xd8\xac\xdb\x8c\xe2\x80\x8c\xd9\xbe\xdb\x8c\xe2\x80\x8c\xd8\xaa\xdb\x8c"},  // چت‌جی‌پی‌تی
    {"ChatGPT", "\xda\x86\xd8\xaa\x20\xd8\xac\xdb\x8c\x20\xd9\xbe\xdb\x8c\x20\xd8\xaa\xdb\x8c"},  // چت جی پی تی
    {"Gemini", "\xd8\xac\xd9\x85\xd9\x86\xd8\xa7\xdb\x8c"},  // جمنای
    {"Gemini", "\xd8\xac\xd9\x85\xdb\x8c\xd9\x86\xd8\xa7\xdb\x8c"},  // جمینای
    {"Hugging Face", "\xd9\x87\xd8\xa7\xda\xaf\xdb\x8c\xd9\x86\xda\xaf\xe2\x80\x8c\xd9\x81\xdb\x8c\xd8\xb3"},  // هاگینگ‌فیس
    {"Hugging Face", "\xd9\x87\xd8\xa7\xda\xaf\xdb\x8c\xd9\x86\xda\xaf\x20\xd9\x81\xdb\x8c\xd8\xb3"},  // هاگینگ فیس
    {"Figma", "\xd9\x81\xdb\x8c\xda\xaf\xd9\x85\xd8\xa7"},  // فیگما
    {"GitHub", "\xda\xaf\xdb\x8c\xd8\xaa\xe2\x80\x8c\xd9\x87\xd8\xa7\xd8\xa8"},  // گیت‌هاب
    {"GitHub", "\xda\xaf\xdb\x8c\xd8\xaa\x20\xd9\x87\xd8\xa7\xd8\xa8"},  // گیت هاب
    {"Notion", "\xd9\x86\xd9\x88\xd8\xb4\xd9\x86"},  // نوشن
    {"Slack", "\xd8\xa7\xd8\xb3\xd9\x84\xda\xa9"},  // اسلک
    {"Cursor", "\xda\xa9\xd8\xb1\xd8\xb3\xd8\xb1"},  // کرسر
    {"VS Code", "\xd9\x88\xdb\x8c\xe2\x80\x8c\xd8\xa7\xd8\xb3\xe2\x80\x8c\xda\xa9\xd8\xaf"},  // وی‌اس‌کد
    {"VS Code", "\xd9\x88\xdb\x8c\x20\xd8\xa7\xd8\xb3\x20\xda\xa9\xd8\xaf"},  // وی اس کد
    {"Vercel", "\xd9\x88\xd8\xb1\xd8\xb3\xd9\x84"},  // ورسل
    {"Cloudflare", "\xda\xa9\xd9\x84\xd8\xa7\xd8\xaf\xd9\x81\xd9\x84\xd8\xb1"},  // کلادفلر
    {"Supabase", "\xd8\xb3\xd9\x88\xd9\xbe\xd8\xa7\xd8\xa8\xdb\x8c\xd8\xb3"},  // سوپابیس
    {"Docker", "\xd8\xaf\xd8\xa7\xda\xa9\xd8\xb1"},  // داکر
    {"macOS", "\xd9\x85\xda\xa9\xe2\x80\x8c\xd8\xa7\xd9\x88\xd8\xa7\xd8\xb3"},  // مک‌اواس
    {"macOS", "\xd9\x85\xda\xa9\x20\xd8\xa7\xd9\x88\x20\xd8\xa7\xd8\xb3"},  // مک او اس
    {"JavaScript", "\xd8\xac\xd8\xa7\xd9\x88\xd8\xa7\xd8\xa7\xd8\xb3\xda\xa9\xd8\xb1\xdb\x8c\xd9\xbe\xd8\xaa"},  // جاوااسکریپت
    {"JavaScript", "\xd8\xac\xd8\xa7\xd9\x88\xd8\xa7\x20\xd8\xa7\xd8\xb3\xda\xa9\xd8\xb1\xdb\x8c\xd9\xbe\xd8\xaa"},  // جاوا اسکریپت
    {"TypeScript", "\xd8\xaa\xd8\xa7\xdb\x8c\xd9\xbe\xe2\x80\x8c\xd8\xa7\xd8\xb3\xda\xa9\xd8\xb1\xdb\x8c\xd9\xbe\xd8\xaa"},  // تایپ‌اسکریپت
    {"TypeScript", "\xd8\xaa\xd8\xa7\xdb\x8c\xd9\xbe\x20\xd8\xa7\xd8\xb3\xda\xa9\xd8\xb1\xdb\x8c\xd9\xbe\xd8\xaa"},  // تایپ اسکریپت
    {"React Native", "\xd8\xb1\xdb\x8c\xe2\x80\x8c\xd8\xa7\xda\xa9\xd8\xaa\x20\xd9\x86\xdb\x8c\xd8\xaa\xdb\x8c\xd9\x88"},  // ری‌اکت نیتیو
    {"React Native", "\xd8\xb1\xdb\x8c\x20\xd8\xa7\xda\xa9\xd8\xaa\x20\xd9\x86\xdb\x8c\xd8\xaa\xdb\x8c\xd9\x88"},  // ری اکت نیتیو
    {"React Native", "\xd8\xb1\xdb\x8c\xd8\xa7\xda\xa9\xd8\xaa\x20\xd9\x86\xdb\x8c\xd8\xaa\xdb\x8c\xd9\x88"},  // ریاکت نیتیو
    {"React Native", "\xd8\xb1\xdb\x8c\xe2\x80\x8c\xd8\xa7\xda\xa9\xd8\xaa\x20\xd9\x86\xdb\x8c\xd8\xaa\xdb\x8c\xd9\x80\xd9\x88"},  // ری‌اکت نیتیـو
    {"React", "\xd8\xb1\xdb\x8c\xe2\x80\x8c\xd8\xa7\xda\xa9\xd8\xaa"},  // ری‌اکت
    {"React", "\xd8\xb1\xdb\x8c\x20\xd8\xa7\xda\xa9\xd8\xaa"},  // ری اکت
    {"Flutter", "\xd9\x81\xd9\x84\xd8\xa7\xd8\xaa\xd8\xb1"},  // فلاتر
    {"Kotlin", "\xda\xa9\xd8\xa7\xd8\xaa\xd9\x84\xdb\x8c\xd9\x86"},  // کاتلین
    {"Swift", "\xd8\xb3\xd9\x88\xd8\xa6\xdb\x8c\xd9\x81\xd8\xaa"},  // سوئیفت
    {"Vue", "\xd9\x88\xdb\x8c\xd9\x88\x20\xd8\xac\xdb\x8c\x20\xd8\xa7\xd8\xb3"},  // ویو جی اس
    {"Next.js", "\xd9\x86\xda\xa9\xd8\xb3\xd8\xaa\x20\xd8\xac\xdb\x8c\x20\xd8\xa7\xd8\xb3"},  // نکست جی اس
    {"Python", "\xd9\xbe\xd8\xa7\xdb\x8c\xd8\xaa\xd9\x88\xd9\x86"},  // پایتون
    {"Node.js", "\xd9\x86\xd9\x88\xd8\xaf\x20\xd8\xac\xdb\x8c\x20\xd8\xa7\xd8\xb3"},  // نود جی اس
    {"Tailwind", "\xd8\xaa\xdb\x8c\xd9\x84\xd9\x88\xdb\x8c\xd9\x86\xd8\xaf"},  // تیلویند
    {"Tailwind", "\xd8\xaa\xdb\x8c\xd9\x84\xe2\x80\x8c\xd9\x88\xdb\x8c\xd9\x86\xd8\xaf"},  // تیل‌ویند
    {"commit", "\xda\xa9\xd8\xa7\xd9\x85\xdb\x8c\xd8\xaa"},  // کامیت
    {"repo", "\xd8\xb1\xdb\x8c\xd9\xbe\xd9\x88"},  // ریپو
    {"repository", "\xd8\xb1\xdb\x8c\xd9\xbe\xd8\xa7\xd8\xb2\xdb\x8c\xd8\xaa\xd9\x88\xd8\xb1\xdb\x8c"},  // ریپازیتوری
    {"repository", "\xd8\xb1\xdb\x8c\xd9\xbe\xd9\x88\xd8\xb2\xdb\x8c\xd8\xaa\xd9\x88\xd8\xb1\xdb\x8c"},  // ریپوزیتوری
    {"repository", "\xd8\xb1\xdb\x8c\xd9\xbe\xd8\xa7\xd8\xb2\xd9\x88\xd8\xaa\xd9\x88\xd8\xb1\xdb\x8c"},  // ریپازوتوری
    {"reload", "\xd8\xb1\xdb\x8c\xd9\x84\xd9\x88\xd8\xaf"},  // ریلود
    {"refresh", "\xd8\xb1\xd9\x81\xd8\xb1\xd8\xb4"},  // رفرش
    {"deploy", "\xd8\xaf\xdb\x8c\xd9\xbe\xd9\x84\xd9\x88\xdb\x8c"},  // دیپلوی
    {"build", "\xd8\xa8\xdb\x8c\xd9\x84\xd8\xaf"},  // بیلد
    {"branch", "\xd8\xa8\xd8\xb1\xd9\x86\xda\x86"},  // برنچ
    {"release", "\xd8\xb1\xdb\x8c\xd9\x84\xdb\x8c\xd8\xb2"},  // ریلیز
    {"feature", "\xd9\x81\xdb\x8c\xda\x86\xd8\xb1"},  // فیچر
    {"issue", "\xd8\xa7\xdb\x8c\xd8\xb4\xd9\x88"},  // ایشو
    {"pull request", "\xd9\xbe\xd9\x88\xd9\x84\x20\xd8\xb1\xdb\x8c\xda\xa9\xd9\x88\xd8\xa6\xd8\xb3\xd8\xaa"},  // پول ریکوئست
    {"pull request", "\xd9\xbe\xd9\x88\xd9\x84\xe2\x80\x8c\xd8\xb1\xdb\x8c\xda\xa9\xd9\x88\xd8\xa6\xd8\xb3\xd8\xaa"},  // پول‌ریکوئست
    {"request", "\xd8\xb1\xdb\x8c\xda\xa9\xd9\x88\xd8\xa6\xd8\xb3\xd8\xaa"},  // ریکوئست
    {"response", "\xd8\xb1\xdb\x8c\xd8\xb3\xd9\xbe\xd8\xa7\xd9\x86\xd8\xb3"},  // ریسپانس
    {"endpoint", "\xd8\xa7\xd9\x86\xd8\xaf\xd9\xbe\xd9\x88\xdb\x8c\xd9\x86\xd8\xaa"},  // اندپوینت
    {"database", "\xd8\xaf\xdb\x8c\xd8\xaa\xd8\xa7\xd8\xa8\xdb\x8c\xd8\xb3"},  // دیتابیس
    {"prompt", "\xd9\xbe\xd8\xb1\xd8\xa7\xd9\x85\xd9\xbe\xd8\xaa"},  // پرامپت
    {"token", "\xd8\xaa\xd9\x88\xda\xa9\xd9\x86"},  // توکن
    {"context", "\xda\xa9\xd8\xa7\xd9\x86\xd8\xaa\xda\xa9\xd8\xb3\xd8\xaa"},  // کانتکست
    {"extension", "\xd8\xa7\xda\xa9\xd8\xb3\xd8\xaa\xd9\x86\xd8\xb4\xd9\x86"},  // اکستنشن
    {"component", "\xda\xa9\xd8\xa7\xd9\x85\xd9\xbe\xd9\x88\xd9\x86\xd9\x86\xd8\xaa"},  // کامپوننت
    {"framework", "\xd9\x81\xd8\xb1\xdb\x8c\xd9\x85\xe2\x80\x8c\xd9\x88\xd8\xb1\xda\xa9"},  // فریم‌ورک
    {"framework", "\xd9\x81\xd8\xb1\xdb\x8c\xd9\x85\xd9\x88\xd8\xb1\xda\xa9"},  // فریمورک
    {"frontend", "\xd9\x81\xd8\xb1\xd8\xa7\xd9\x86\xd8\xaa\xe2\x80\x8c\xd8\xa7\xd9\x86\xd8\xaf"},  // فرانت‌اند
    {"frontend", "\xd9\x81\xd8\xb1\xd8\xa7\xd9\x86\xd8\xaa\x20\xd8\xa7\xd9\x86\xd8\xaf"},  // فرانت اند
    {"backend", "\xd8\xa8\xda\xa9\xe2\x80\x8c\xd8\xa7\xd9\x86\xd8\xaf"},  // بک‌اند
    {"backend", "\xd8\xa8\xda\xa9\x20\xd8\xa7\xd9\x86\xd8\xaf"},  // بک اند
    {"console", "\xda\xa9\xd9\x86\xd8\xb3\xd9\x88\xd9\x84"},  // کنسول
    {"screenshot", "\xd8\xa7\xd8\xb3\xda\xa9\xd8\xb1\xdb\x8c\xd9\x86\xe2\x80\x8c\xd8\xb4\xd8\xa7\xd8\xaa"},  // اسکرین‌شات
    {"screenshot", "\xd8\xa7\xd8\xb3\xda\xa9\xd8\xb1\xdb\x8c\xd9\x86\x20\xd8\xb4\xd8\xa7\xd8\xaa"},  // اسکرین شات
    {"script", "\xd8\xa7\xd8\xb3\xda\xa9\xd8\xb1\xdb\x8c\xd9\xbe\xd8\xaa"},  // اسکریپت
    {"client", "\xda\xa9\xd9\x84\xd8\xa7\xdb\x8c\xd9\x86\xd8\xaa"},  // کلاینت
    {"workflow", "\xd9\x88\xd8\xb1\xda\xa9\xe2\x80\x8c\xd9\x81\xd9\x84\xd9\x88"},  // ورک‌فلو
    {"workflow", "\xd9\x88\xd8\xb1\xda\xa9\x20\xd9\x81\xd9\x84\xd9\x88"},  // ورک فلو
    {"pipeline", "\xd9\xbe\xd8\xa7\xdb\x8c\xd9\xbe\xe2\x80\x8c\xd9\x84\xd8\xa7\xdb\x8c\xd9\x86"},  // پایپ‌لاین
    {"pipeline", "\xd9\xbe\xd8\xa7\xdb\x8c\xd9\xbe\x20\xd9\x84\xd8\xa7\xdb\x8c\xd9\x86"},  // پایپ لاین
    {"end-to-end", "\xd8\xa7\xd9\x86\xd8\xaf\x20\xd8\xaa\xd9\x88\x20\xd8\xa7\xd9\x86\xd8\xaf"},  // اند تو اند
    {"design system", "\xd8\xaf\xdb\x8c\xd8\xb2\xd8\xa7\xdb\x8c\xd9\x86\x20\xd8\xb3\xdb\x8c\xd8\xb3\xd8\xaa\xd9\x85"},  // دیزاین سیستم
    {"product", "\xd9\xbe\xd8\xb1\xd9\x88\xd8\xaf\xd8\xa7\xda\xa9\xd8\xaa"},  // پروداکت
    {"dark mode", "\xd8\xaf\xd8\xa7\xd8\xb1\xda\xa9\x20\xd9\x85\xd9\x88\xd8\xaf"},  // دارک مود
    {"light mode", "\xd9\x84\xd8\xa7\xdb\x8c\xd8\xaa\x20\xd9\x85\xd9\x88\xd8\xaf"},  // لایت مود
    {"API", "\xd8\xa7\xdb\x8c\xe2\x80\x8c\xd9\xbe\xdb\x8c\xe2\x80\x8c\xd8\xa2\xdb\x8c"},  // ای‌پی‌آی
    {"API", "\xd8\xa7\xdb\x8c\x20\xd9\xbe\xdb\x8c\x20\xd8\xa2\xdb\x8c"},  // ای پی آی
    {"UI", "\xdb\x8c\xd9\x88\xe2\x80\x8c\xd8\xa2\xdb\x8c"},  // یو‌آی
    {"UI", "\xdb\x8c\xd9\x88\x20\xd8\xa2\xdb\x8c"},  // یو آی
    {"UX", "\xdb\x8c\xd9\x88\xe2\x80\x8c\xd8\xa7\xdb\x8c\xda\xa9\xd8\xb3"},  // یو‌ایکس
    {"UX", "\xdb\x8c\xd9\x88\x20\xd8\xa7\xdb\x8c\xda\xa9\xd8\xb3"},  // یو ایکس
    {"MVP", "\xd8\xa7\xd9\x85\xe2\x80\x8c\xd9\x88\xdb\x8c\xe2\x80\x8c\xd9\xbe\xdb\x8c"},  // ام‌وی‌پی
    {"MVP", "\xd8\xa7\xd9\x85\x20\xd9\x88\xdb\x8c\x20\xd9\xbe\xdb\x8c"},  // ام وی پی
    {"RTL", "\xd8\xa2\xd8\xb1\xe2\x80\x8c\xd8\xaa\xdb\x8c\xe2\x80\x8c\xd8\xa7\xd9\x84"},  // آر‌تی‌ال
    {"RTL", "\xd8\xa2\xd8\xb1\x20\xd8\xaa\xdb\x8c\x20\xd8\xa7\xd9\x84"},  // آر تی ال
    {"LLM", "\xd8\xa7\xd9\x84\xe2\x80\x8c\xd8\xa7\xd9\x84\xe2\x80\x8c\xd8\xa7\xd9\x85"},  // ال‌ال‌ام
    {"LLM", "\xd8\xa7\xd9\x84\x20\xd8\xa7\xd9\x84\x20\xd8\xa7\xd9\x85"},  // ال ال ام
    {"MCP", "\xd8\xa7\xd9\x85\xe2\x80\x8c\xd8\xb3\xdb\x8c\xe2\x80\x8c\xd9\xbe\xdb\x8c"},  // ام‌سی‌پی
    {"MCP", "\xd8\xa7\xd9\x85\x20\xd8\xb3\xdb\x8c\x20\xd9\xbe\xdb\x8c"},  // ام سی پی
    {"CSS", "\xd8\xb3\xdb\x8c\xe2\x80\x8c\xd8\xa7\xd8\xb3\xe2\x80\x8c\xd8\xa7\xd8\xb3"},  // سی‌اس‌اس
    {"CSS", "\xd8\xb3\xdb\x8c\x20\xd8\xa7\xd8\xb3\x20\xd8\xa7\xd8\xb3"},  // سی اس اس
    {"HTML", "\xd8\xa7\xda\x86\xe2\x80\x8c\xd8\xaa\xdb\x8c\xe2\x80\x8c\xd8\xa7\xd9\x85\xe2\x80\x8c\xd8\xa7\xd9\x84"},  // اچ‌تی‌ام‌ال
    {"HTML", "\xd8\xa7\xda\x86\x20\xd8\xaa\xdb\x8c\x20\xd8\xa7\xd9\x85\x20\xd8\xa7\xd9\x84"},  // اچ تی ام ال
    {"PR", "\xd9\xbe\xdb\x8c\xe2\x80\x8c\xd8\xa2\xd8\xb1"},  // پی‌آر
    {"PR", "\xd9\xbe\xdb\x8c\x20\xd8\xa2\xd8\xb1"},  // پی آر
    {"AI", "\xd8\xa7\xdb\x8c\xe2\x80\x8c\xd8\xa2\xdb\x8c"},  // ای‌آی
    {"AI", "\xd8\xa7\xdb\x8c\x20\xd8\xa2\xdb\x8c"},  // ای آی
    {"URL", "\xdb\x8c\xd9\x88\xe2\x80\x8c\xd8\xa2\xd8\xb1\xe2\x80\x8c\xd8\xa7\xd9\x84"},  // یو‌آر‌ال
    {"URL", "\xdb\x8c\xd9\x88\x20\xd8\xa2\xd8\xb1\x20\xd8\xa7\xd9\x84"},  // یو آر ال
    {"PDF", "\xd9\xbe\xdb\x8c\xe2\x80\x8c\xd8\xaf\xdb\x8c\xe2\x80\x8c\xd8\xa7\xd9\x81"},  // پی‌دی‌اف
    {"PDF", "\xd9\xbe\xdb\x8c\x20\xd8\xaf\xdb\x8c\x20\xd8\xa7\xd9\x81"},  // پی دی اف
};
#define N_LATIN (sizeof kLatin / sizeof *kLatin)

// Precomputed per-row sound key + word count (of the Persian spelling, split
// on ASCII space or ZWNJ, same as latin.js's `f.split(/[\s‌]+/).length`).
typedef struct {
  const char *en;
  CpString k;
  int n_words;
} Entry;

static Entry g_entries[N_LATIN];
static int g_entries_built;
static int g_max_words;

static int count_words_sep(const char *s) {
  int n = 0, in_word = 0;
  for (size_t i = 0; s[i];) {
    if (s[i] == ' ') { in_word = 0; i++; continue; }
    if ((unsigned char)s[i] == 0xE2 && (unsigned char)s[i + 1] == 0x80 && (unsigned char)s[i + 2] == 0x8C) {
      in_word = 0;
      i += 3;
      continue;
    }
    if (!in_word) { n++; in_word = 1; }
    // advance one UTF-8 codepoint
    unsigned char c = (unsigned char)s[i];
    i += (c < 0x80) ? 1 : (c >> 5) == 6 ? 2 : (c >> 4) == 14 ? 3 : (c >> 3) == 30 ? 4 : 1;
  }
  return n;
}

static void build_entries(void) {
  if (g_entries_built) return;
  g_entries_built = 1;
  for (size_t i = 0; i < N_LATIN; i++) {
    g_entries[i].en = kLatin[i].en;
    g_entries[i].k = persian_sound_key(kLatin[i].fa);
    g_entries[i].n_words = count_words_sep(kLatin[i].fa);
    if (g_entries[i].n_words > g_max_words) g_max_words = g_entries[i].n_words;
  }
}

// --- "push کن" / "پول ریکوئست"-style light-verb phrases ---------------------

typedef struct {
  const char *en;
  const char *fa;
} VerbRow;

static const VerbRow kVerbPhrase[] = {
    {"push", "\xd9\xbe\xd9\x88\xd8\xb4"},    // پوش
    {"pull", "\xd9\xbe\xd9\x88\xd9\x84"},    // پول
    {"merge", "\xd9\x85\xd8\xb1\xd8\xac"},   // مرج
    {"clone", "\xda\xa9\xd9\x84\xd9\x88\xd9\x86"},  // کلون
    {"fork", "\xd9\x81\xd9\x88\xd8\xb1\xda\xa9"},   // فورک
};
#define N_VERB_PHRASE (sizeof kVerbPhrase / sizeof *kVerbPhrase)

#define FA_VA "\xd9\x88"          // و ("and")
#define FA_HARJ "\xd9\x87\xd8\xb1\xd8\xac"  // هرج ("chaos", only in the idiom هرج و مرج)
#define FA_ZWNJ "\xe2\x80\x8c"

// Light-verb stems (کن/کرد/شد/شده/بشه/بشن/زد/زن), each optionally preceded by
// one of the prefixes ن/ب/می(+ZWNJ)/نمی(+ZWNJ) — mirrors latin.js's
// LIGHT_VERB regex, matched as a *prefix* of the next word (it may have more
// letters after, e.g. "کنم", "کرده").
static const char *const kVerbPrefixes[] = {"", "\xd9\x86", "\xd8\xa8", "\xd9\x85\xdb\x8c", "\xd9\x85\xdb\x8c" FA_ZWNJ,
                                             "\xd9\x86\xd9\x85\xdb\x8c", "\xd9\x86\xd9\x85\xdb\x8c" FA_ZWNJ};
static const char *const kVerbStems[] = {"\xda\xa9\xd9\x86", "\xda\xa9\xd8\xb1\xd8\xaf", "\xd8\xb4\xd8\xaf",
                                          "\xd8\xb4\xd8\xaf\xd9\x87", "\xd8\xa8\xd8\xb4\xd9\x87", "\xd8\xa8\xd8\xb4\xd9\x86",
                                          "\xd8\xb2\xd8\xaf", "\xd8\xb2\xd9\x86"};

static int is_light_verb(const char *s, size_t len) {
  for (size_t p = 0; p < sizeof kVerbPrefixes / sizeof *kVerbPrefixes; p++) {
    size_t pl = strlen(kVerbPrefixes[p]);
    if (pl > len || memcmp(s, kVerbPrefixes[p], pl) != 0) continue;
    for (size_t v = 0; v < sizeof kVerbStems / sizeof *kVerbStems; v++) {
      size_t vl = strlen(kVerbStems[v]);
      if (pl + vl <= len && memcmp(s + pl, kVerbStems[v], vl) == 0) return 1;
    }
  }
  return 0;
}

// --- tokens -------------------------------------------------------------

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
    if (teq(t[i], FA_VA)) return 1;
  return 0;
}

static void join_tokens(const Tok *t, int from, int span, char *out) {
  size_t o = 0;
  for (int i = 0; i < span; i++) {
    if (i) out[o++] = ' ';
    memcpy(out + o, t[from + i].s, t[from + i].len);
    o += t[from + i].len;
  }
  out[o] = 0;
}

static size_t cp_len_of(const char *s, size_t len) {
  CpString c = cp_decode_n(s, len);
  size_t n = c.n;
  cp_free(&c);
  return n;
}

// Pronoun/plural suffixes glued to the end of an English word ("کامیتش"),
// longest-first, mirroring latin.js's SUFFIX regex.
static const char *const kSuffixes[] = {
    "\xd9\x87\xd8\xa7\xd9\x85\xd9\x88\xd9\x86",  // هامون (5)
    "\xd9\x87\xd8\xa7\xd8\xaa\xd9\x88\xd9\x86",  // هاتون (5)
    "\xd9\x87\xd8\xa7\xd8\xb4\xd9\x88\xd9\x86",  // هاشون (5)
    "\xd9\x87\xd8\xa7\xdb\x8c\xdb\x8c",          // هایی (4)
    "\xd9\x87\xd8\xa7\xdb\x8c\xd9\x85",          // هایم (4)
    "\xd9\x87\xd8\xa7\xdb\x8c\xd8\xaa",          // هایت (4)
    "\xd9\x87\xd8\xa7\xdb\x8c\xd8\xb4",          // هایش (4)
    "\xd9\x87\xd8\xa7\xdb\x8c",                  // های (3)
    "\xd9\x85\xd9\x88\xd9\x86",                  // مون (3)
    "\xd8\xaa\xd9\x88\xd9\x86",                  // تون (3)
    "\xd8\xb4\xd9\x88\xd9\x86",                  // شون (3)
    "\xd9\x87\xd8\xa7",                          // ها (2)
    "\xd8\xb4",                                  // ش (1)
    "\xd9\x85",                                  // م (1)
    "\xd8\xaa",                                  // ت (1)
    "\xdb\x8c",                                  // ی (1)
};
#define N_SUFFIX (sizeof kSuffixes / sizeof *kSuffixes)

// Longest suffix from kSuffixes at the end of `s` (len bytes), leaving at
// least 1 byte of prefix. 0 if none.
static size_t find_suffix(const char *s, size_t len) {
  for (size_t i = 0; i < N_SUFFIX; i++) {
    size_t sl = strlen(kSuffixes[i]);
    if (sl < len && memcmp(s + len - sl, kSuffixes[i], sl) == 0) return sl;
  }
  return 0;
}

// Strips ZWNJ bytes (E2 80 8C) out of a buffer in place; returns new length.
static size_t strip_zwnj(char *s, size_t len) {
  size_t o = 0;
  for (size_t i = 0; i < len;) {
    if (i + 3 <= len && (unsigned char)s[i] == 0xE2 && (unsigned char)s[i + 1] == 0x80 && (unsigned char)s[i + 2] == 0x8C) {
      i += 3;
      continue;
    }
    s[o++] = s[i++];
  }
  return o;
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

// Best English match for a chunk of exactly `span` original tokens (sound-
// equal, or one near-sound edit off for a 5+ sound-letter entry).
static const char *match(const char *chunk, int span) {
  CpString k = persian_sound_key(chunk);
  const char *best = NULL;
  double best_d = 1e18;
  for (size_t e = 0; e < N_LATIN; e++) {
    if (span > g_entries[e].n_words) continue;
    long diff = (long)k.n - (long)g_entries[e].k.n;
    if (diff < 0) diff = -diff;
    if (diff > 1) continue;
    int exact = k.n == g_entries[e].k.n && memcmp(k.cp, g_entries[e].k.cp, k.n * sizeof *k.cp) == 0;
    double d = exact ? 0 : (g_entries[e].k.n >= 5 && k.n > 0 && k.cp[0] == g_entries[e].k.cp[0])
                               ? persian_edit_distance(&k, &g_entries[e].k, 0.5)
                               : 9;
    if (d <= 0.5 && (!best || d < best_d)) { best = g_entries[e].en; best_d = d; }
  }
  cp_free(&k);
  return best;
}

char *persian_to_latin(const char *text) {
  build_entries();
  if (!text) return NULL;
  Tok t[256];
  int n = tokenize(text, t, 256);
  if (n == 0) return strdup(text);

  size_t out_cap = strlen(text) * 2 + 64;
  char *out = malloc(out_cap);
  size_t o = 0;

  for (int i = 0; i < n;) {
    // Light-verb phrase: "پوش کن" -> "push کن".
    const char *verb_en = NULL;
    int is_idiom = 0;
    for (size_t v = 0; v < N_VERB_PHRASE; v++) {
      if (teq(t[i], kVerbPhrase[v].fa)) {
        verb_en = kVerbPhrase[v].en;
        is_idiom = strcmp(kVerbPhrase[v].en, "merge") == 0 && i >= 2 && teq(t[i - 1], FA_VA) && teq(t[i - 2], FA_HARJ);
        break;
      }
    }
    if (verb_en && !is_idiom && i + 1 < n && is_light_verb(t[i + 1].s, t[i + 1].len)) {
      if (o) out[o++] = ' ';
      size_t need = strlen(verb_en);
      while (o + need + 1 > out_cap) { out_cap *= 2; out = realloc(out, out_cap); }
      memcpy(out + o, verb_en, need);
      o += need;
      i++;
      continue;
    }

    // Longest span first, so "پول ریکوئست" wins over lone "پول".
    const char *hit_text = NULL;
    int hit_span = 0;
    int max_span = n - i < g_max_words ? n - i : g_max_words;
    for (int span = max_span; span >= 1 && !hit_text; span--) {
      if (span > 1 && contains_and(t, i, span)) continue;
      char chunk[1024];
      join_tokens(t, i, span, chunk);
      const char *m = match(chunk, span);
      if (m) { hit_text = m; hit_span = span; break; }

      // English term + Persian suffix: "کامیت‌ها" -> "commit‌ها".
      size_t clen = strlen(chunk);
      size_t stripped_len = strip_zwnj(chunk, clen);
      chunk[stripped_len] = 0;
      size_t sl = find_suffix(chunk, stripped_len);
      if (!sl) continue;
      size_t prefix_len = stripped_len - sl;
      CpString pk = persian_sound_key_n(chunk, prefix_len);
      const char *lw;
      size_t lwl;
      last_word(chunk, prefix_len, &lw, &lwl);
      if (pk.n >= 3 && cp_len_of(lw, lwl) >= 2) {
        char prefix_buf[1024];
        memcpy(prefix_buf, chunk, prefix_len);
        prefix_buf[prefix_len] = 0;
        const char *pm = match(prefix_buf, span);
        if (pm) {
          static char combined[1100];
          snprintf(combined, sizeof combined, "%s" FA_ZWNJ "%s", pm, chunk + prefix_len);
          hit_text = combined;
          hit_span = span;
        }
      }
      cp_free(&pk);
    }

    if (o) out[o++] = ' ';
    if (hit_text) {
      size_t need = strlen(hit_text);
      while (o + need + 1 > out_cap) { out_cap *= 2; out = realloc(out, out_cap); }
      memcpy(out + o, hit_text, need);
      o += need;
      i += hit_span;
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
