#pragma once

#include <stddef.h>

// A small chained-hashtable string -> long map. Keys are copied in (not
// retained by reference), so the caller's buffer can be freed after loading.
typedef struct StrMap StrMap;

StrMap *strmap_new(size_t bucket_hint);
void strmap_free(StrMap *m);
void strmap_set(StrMap *m, const char *key, size_t keylen, long value);
// Returns 1 and sets *value if found, 0 otherwise.
int strmap_get(const StrMap *m, const char *key, size_t keylen, long *value);
size_t strmap_count(const StrMap *m);
