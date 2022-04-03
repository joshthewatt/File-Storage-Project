#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <inttypes.h>

#include "cache.h"

static cache_entry_t *cache = NULL;
static int cache_size = 0;
static int clock = 0;
static int num_queries = 0;
static int num_hits = 0;

static int cache_items = 0;// number of inserts
static bool cache_initialized = false;

/* Returns 1 on success and -1 on failure. Should allocate a space for
 * |num_entries| cache entries, each of type cache_entry_t. Calling it again
 * without first calling cache_destroy (see below) should fail. */
int cache_create(int num_entries) { // num_entries is 2 - 4096
  // if called twice and cache_destroy not called

  if (cache_initialized == true || num_entries < 2 || num_entries > 4096){
    return -1;
  }

  // allocation
  cache = (cache_entry_t*)malloc(num_entries * sizeof(cache_entry_t));
  cache_size = num_entries;
  cache_initialized = true;

  // if allocation unsuccessful
  if (cache == NULL){
    return -1;
  }
  else{
    cache_initialized = true;
    return 1;
  }
}

int cache_destroy(void) {
  // if called twice and cache_create not called
  if (cache_initialized == false){
    return -1;
  }
  free(cache);
  cache_size = 0;
  cache_items = 0;
  cache_initialized = false;
  return 1;
}

int cache_lookup(int disk_num, int block_num, uint8_t *buf) {
  if (cache_initialized == false || cache_size == 0 || buf == NULL){
    return -1;
  }

  cache_entry_t temp;
  num_queries += 1;
  for (int k = 0; k < cache_items; k++){ // Loop through cache
    temp = cache[k];
    if (temp.disk_num == disk_num && temp.block_num == block_num){ // If found in cache

      if (temp.block == NULL){
        return -1;
      }
      memcpy(buf, temp.block, 256); // copy block
      // Increments
      num_hits += 1;
      clock += 1;
      cache[k].access_time = clock;
      return 1;
    }
  }
  return -1;
}

void cache_update(int disk_num, int block_num, const uint8_t *buf) {
  cache_entry_t temp;
  for (int k = 0; k < cache_items; k++){ // Loop through to find the right entry
    temp = cache[k];
    if (temp.disk_num == disk_num && temp.block_num == block_num){ // Finding the disk and block
      memcpy(cache[k].block, buf, 256); // Copy in new block contents
      clock += 1;
      temp.access_time = clock;
      break;

    }
  }

}

int cache_insert(int disk_num, int block_num, const uint8_t *buf) {
  // Parameters
  if (cache_initialized == false || disk_num < 0 || disk_num > 16 || block_num < 0|| block_num > 256 || buf == NULL){
    return -1;
  }

  clock += 1;
  // Make new cache insert
  cache_entry_t insert;
  insert.valid = true;
  insert.disk_num = disk_num;
  insert.block_num = block_num;
  memcpy(insert.block, buf, 256);
  insert.access_time = clock;

  // Check if insert already exists, if so Update
  cache_entry_t temp;
  for (int k = 0; k < cache_items; k++){
    temp = cache[k];
    if (temp.disk_num == disk_num && temp.block_num == block_num ){
      cache_update(disk_num, block_num, buf);
      return -1;
    }
  }
  // LRU alg
  if (cache_size == cache_items){ //where cache is filled LRU
    int lru_time = cache[0].access_time;
    for (int k = 1; k < cache_size; k++){
      if (cache[k].access_time < lru_time){
        lru_time = cache[k].access_time;
      }
    }
    // Find place to insert
    for (int k = 0; k < cache_size; k++){
      if (cache[k].access_time == lru_time){
        cache[k] = insert;
      }
    }
    return 1;
  }
  // Regular insert if not filled
  else{ // Cache not filled, just put in
    cache[cache_items] = insert;
    cache_items += 1;
    return 1;
  }
  return -1;
}

bool cache_enabled(void) {
  if (cache_size >= 2){
    return true;
  }
  return false;
}

void cache_print_hit_rate(void) {
  fprintf(stderr, "Hit rate: %5.1f%%\n", 100 * (float) num_hits / num_queries);
}
