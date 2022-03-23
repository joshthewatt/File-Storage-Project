#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "cache.h"

static cache_entry_t *cache = NULL;
static int cache_size = 0;
static int clock = 0;
static int num_queries = 0;
static int num_hits = 0;

/* Returns 1 on success and -1 on failure. Should allocate a space for
 * |num_entries| cache entries, each of type cache_entry_t. Calling it again
 * without first calling cache_destroy (see below) should fail. */
int cache_create(int num_entries) { // num_entries is 2 - 4096
  // if called twice and cache_destroy not called
  if (cache != NULL || cache_size != 0){
    return -1;
  }
  // allocation
  cache = (*cache_entry_t)malloc(num_entries * sizeof(cache_entry_t));
  cache_size = num_entries;
  // if allocation unsuccessful
  if (cache == NULL){
    return -1;
  }
  else{
    return 1;
  }
}

int cache_destroy(void) {
  // if called twice and cache_create not called
  if (cache == NULL || cache_size == 0){
    return -1;
  }
  free(cache);
  cache_size = 0;
  
  return 1;
}

int cache_lookup(int disk_num, int block_num, uint8_t *buf) {
  return -10;
}

void cache_update(int disk_num, int block_num, const uint8_t *buf) {
}

int cache_insert(int disk_num, int block_num, const uint8_t *buf) {
  return -10;
}

bool cache_enabled(void) {
  return false;
}

void cache_print_hit_rate(void) {
  fprintf(stderr, "Hit rate: %5.1f%%\n", 100 * (float) num_hits / num_queries);
}
