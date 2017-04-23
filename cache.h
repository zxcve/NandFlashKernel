#ifndef LKP_KV_CACHE_H
#define LKP_KV_CACHE_H

#include <linux/types.h>

void cache_init (void);
struct cache_page* cache_get_page (uint64_t page_addr);
void* cache_read_page (struct cache_page* cp);
void cache_shutdown (void);

#endif
