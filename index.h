#ifndef LKP_KV_INDEX_H
#define LKP_KV_INDEX_H

#include <linux/hashtable.h>

void index_put(const char* key, uint64_t page_idx);
uint64_t index_get(const char* key);
void index_clear(void);


#endif
