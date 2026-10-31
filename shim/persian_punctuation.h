#pragma once

// Spoken punctuation -> the character: «بک اسلش» -> \, «علامت سوال» -> ؟, «دات
// کام» -> .com, … Ported from Avan's extension/asr/punctuation.js.
//
// Call persian_spoken_symbols() on the raw transcript before the rest of the
// polish chain, and persian_restore_symbols() on the final result. Symbols the
// chain in between would otherwise treat specially (\ / _ # and the dot inside
// an address) travel as private-use placeholders so "گیت هاب دات کام" can still
// become "GitHub.com" after lexicon/Latin-term correction runs. Both return a
// malloc'ed string; NULL in is NULL out.
char *persian_spoken_symbols(const char *text);
char *persian_restore_symbols(const char *text);
