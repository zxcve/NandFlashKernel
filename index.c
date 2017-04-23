#include "index.h"

#include <linux/slab.h>

#define KV_HASH_BITS 6

/* Fowler-Noll-Vo hash constants, for 32-bit word sizes. */
#define FNV_32_PRIME 16777619u
#define FNV_32_BASIS 2166136261u

DEFINE_HASHTABLE(kv_index_table, KV_HASH_BITS);

struct index_item {
     const char* key;
     uint64_t page_idx;
     struct hlist_node hlist_elem;
};

static unsigned hash_string (const char*, unsigned);

void
index_put(const char* key, uint64_t page_idx)
{
	struct index_item *node;
	node = vmalloc(sizeof(*node));
	if(!node)
		printk(KERN_INFO "Could not put key in index");
		
	node->key = key;
	node->page_idx = page_idx;
	
	hlist_add_head(&node->hlist_elem, &kv_index_table[hash_string(key, HASH_SIZE(kv_index_table))]);
}

uint64_t
index_get(const char* key)
{
	struct index_item *node;
	hlist_for_each_entry(node, &kv_index_table[hash_string(key, HASH_SIZE(kv_index_table))], hlist_elem)
	{
		if(strcmp(node->key, key) == 0)
			return node->page_idx;
	}
	return -1;
}

void
index_clear()
{
	int bkt;
	struct index_item *node;
	struct hlist_node *tmp;
	hash_for_each_safe(kv_index_table, bkt, tmp, node, hlist_elem)
	{
		hash_del(&node->hlist_elem);
		vfree(node);
	}
}

static unsigned
hash_string (const char *s_, unsigned bits)
{
  const unsigned char *s = (const unsigned char *) s_;
  unsigned hash;

  hash = FNV_32_BASIS;
  while (*s != '\0')
    hash = (hash * FNV_32_PRIME) ^ *s++;

  return hash % bits;
}
