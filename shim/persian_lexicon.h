#pragma once

// Lexicon correction and sound-alike respelling, ported from Avan's
// extension/asr/lexicon.js. Backed by shim/data/fa-words.txt.

typedef struct PersianWords PersianWords;  // fa-words.txt: word -> frequency count
typedef struct SoundIndex SoundIndex;      // sound key -> most frequent known word

// Loads shim/data/fa-words.txt from <farsight_repo>/shim/data/fa-words.txt.
// Returns NULL on failure (file missing); callers should keep working without
// lexicon correction in that case rather than fail the whole transcript.
PersianWords *persian_words_load(const char *repo_dir);
void persian_words_free(PersianWords *w);
// Is `word` a known Persian word (by frequency-list membership)?
int persian_words_has(const PersianWords *w, const char *word);

// sound key -> most frequent word with at least `min_count` occurrences and
// 3+ characters, built once from shim/data/fa-words.txt. NULL on failure.
SoundIndex *persian_sound_index_build(const char *repo_dir, long min_count);
void persian_sound_index_free(SoundIndex *idx);

// Replaces 1-3 word spans that *sound* like a curated lexicon entry (names,
// products, tech terms — see the LEXICON array in persian_lexicon.c) with its
// correct spelling. Conservative: only entries of 5+ sound-letters, matching
// first sound, within one weighted edit per 5 letters. A span made only of
// words `words` already knows is left alone unless it sounds identical to an
// entry. Returns a malloc'ed string.
char *persian_correct_with_lexicon(const char *text, const PersianWords *words);

// Replaces unknown words (3+ letters, pure Arabic-script, not in `words`)
// with the most common known word that sounds identical ("متمئن" ->
// "مطمئن"). Returns a malloc'ed string.
char *persian_fix_sound_alikes(const char *text, const PersianWords *words, const SoundIndex *idx);
