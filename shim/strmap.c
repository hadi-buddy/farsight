#include "strmap.h"

#include <stdlib.h>
#include <string.h>

typedef struct Entry {
  char *key;
  size_t keylen;
  long value;
  struct Entry *next;
} Entry;

struct StrMap {
  Entry **buckets;
  size_t nbuckets;
  size_t n;
};

static unsigned long hash_bytes(const char *s, size_t n) {
  unsigned long h = 5381;
  for (size_t i = 0; i < n; i++) h = ((h << 5) + h) + (unsigned char)s[i];  // djb2
  return h;
}

StrMap *strmap_new(size_t bucket_hint) {
  StrMap *m = malloc(sizeof *m);
  m->nbuckets = bucket_hint < 16 ? 16 : bucket_hint;
  m->buckets = calloc(m->nbuckets, sizeof *m->buckets);
  m->n = 0;
  return m;
}

void strmap_free(StrMap *m) {
  if (!m) return;
  for (size_t i = 0; i < m->nbuckets; i++) {
    Entry *e = m->buckets[i];
    while (e) {
      Entry *next = e->next;
      free(e->key);
      free(e);
      e = next;
    }
  }
  free(m->buckets);
  free(m);
}

void strmap_set(StrMap *m, const char *key, size_t keylen, long value) {
  unsigned long h = hash_bytes(key, keylen) % m->nbuckets;
  for (Entry *e = m->buckets[h]; e; e = e->next)
    if (e->keylen == keylen && memcmp(e->key, key, keylen) == 0) {
      e->value = value;
      return;
    }
  Entry *e = malloc(sizeof *e);
  e->key = malloc(keylen + 1);
  memcpy(e->key, key, keylen);
  e->key[keylen] = 0;
  e->keylen = keylen;
  e->value = value;
  e->next = m->buckets[h];
  m->buckets[h] = e;
  m->n++;
}

int strmap_get(const StrMap *m, const char *key, size_t keylen, long *value) {
  unsigned long h = hash_bytes(key, keylen) % m->nbuckets;
  for (Entry *e = m->buckets[h]; e; e = e->next)
    if (e->keylen == keylen && memcmp(e->key, key, keylen) == 0) {
      if (value) *value = e->value;
      return 1;
    }
  return 0;
}

size_t strmap_count(const StrMap *m) { return m->n; }
