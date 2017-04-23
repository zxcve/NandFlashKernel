#include "cache.h"
#include "core.h"

#include <linux/list.h>
#include <linux/bitmap.h>
#include <linux/slab.h>

/* Configure the number of cache pages here */
#define NUM_CACHE_PAGES 64

/* Stores the address of cache in-memory */
static void *start_address = NULL;

/* Initialize the linked list to store struct cache_page. Each
 * entry represents a cache page
 */
LIST_HEAD(cache_pages);

/* Bitmap to store which in-memory cache pages are free/used
 */
static DECLARE_BITMAP(used_cache_map, NUM_CACHE_PAGES);

/* This is used to store information about a cache page
 */
struct cache_page
{
    uint64_t page_idx;		        /* Disk page this cache page
                                       represents */
    void *address;                  /* Address in memory for this
                                       cache page */
    bool is_valid;                   /* If the contents in the address
                                       corresponds to disk page */
    struct list_head elem;
};

static struct cache_page* cache_lookup (uint64_t);
static struct cache_page* cache_get (uint64_t);
static struct cache_page* cache_alloc (void);
static struct cache_page* cache_get_evict_page (void);

/* Initializes the cache memory.*/
void cache_init (void)
{
	/* Allocate the memory required to store all the cache pages
	 */
	start_address = vmalloc (NUM_CACHE_PAGES * config.page_size);
}

/* reserve a page in cache dedicated to hold this page_idx
   possibly evicting some other LRU page */
struct cache_page*
cache_get_page (uint64_t page_idx)
{
  struct cache_page *cp; 
  cp = cache_get (page_idx);
  return cp;
}

/* This function reads page from disk, returns pointer to data */
void*
cache_read_page (struct cache_page *cp)
{
  uint64_t addr;
  size_t retlen;
  if(!cp->is_valid)
    {
	  addr = ((uint64_t) cp->page_idx) * ((uint64_t) config.page_size);
      config.mtd->_read(config.mtd, addr, config.page_size, &retlen,
				 cp->address);
      cp->is_valid = true;
    } 
  return cp->address;
}

/* Upon shutdown, cleanup  */

void
cache_shutdown ()
{
	
}

/* Returns the cache pages which matches with the page_idx by looking up all
   the entries in the cache. */

static struct cache_page*
cache_lookup (uint64_t page_idx)
{
  struct list_head *e;
  struct cache_page *cp = NULL;

  /* look through cache_pages linked list and find the page_idx. If found,
   * great. Otherwise, return NULL
   */
  list_for_each(e, &cache_pages) {
	  cp = list_entry(e, struct cache_page, elem);
	  if(cp->page_idx == page_idx)
        return cp;
  }
  
  return NULL;
}

/* Returns the cache page by looking up the entries in the buffer
   and matching with the sector. */

static struct cache_page*
cache_get (uint64_t page_idx)
{
	struct cache_page *cp = NULL;
	/* check if page_idx is in cache */
	cp = cache_lookup (page_idx);
	if (cp == NULL)
    {
		/* The page is not in cache. so, allocate a page
		 */
		cp = cache_alloc ();
		cp->page_idx = page_idx;
		cp->is_valid = false;
    }
    return cp;
}

/* Allocates a cache page which can be used. If no cache pages are
   free, then evict a page and reuse it. */

static struct cache_page*
cache_alloc ()
{
  struct cache_page *cp;
  void *address;

	/* check the bitmap to see if there is any free cache page */
	size_t cache_idx = find_first_zero_bit (used_cache_map, NUM_CACHE_PAGES);
	if (cache_idx == NUM_CACHE_PAGES)
	{
		/* Nothing free. Evict page */
		cp = cache_get_evict_page ();
		/* Since we are evicting an existing page, we have to store the
		 * in-memory address, clear the cp struct and reassign the address
		 */
		address = cp->address;
		memset (cp, 0, sizeof *cp);
		cp->address = address;
		//printk(KERN_INFO "full alloc %d %p", cache_idx, cp->address);
		/* We add it to the top of the cache_pages list to indicate 
		 * recent use */
		list_add (&cp->elem, &cache_pages);
		return cp;
    }
	else
    {
		/* cache_idx is free, set it to used and use it */
		set_bit(cache_idx, used_cache_map);
		cp = vmalloc (sizeof(struct cache_page));
		/* This is the address in memory where the page will be written */
		cp->address = start_address + (cache_idx * config.page_size);
		//printk(KERN_INFO "alloc %d %p", cache_idx, cp->address);
		/* We add it to the top of the cache_pages list to indicate 
		 * recent use */
		list_add (&cp->elem, &cache_pages);

		return cp;
    }

}

/* Evicts a cache page LRU*/

static struct cache_page*
cache_get_evict_page ()
{
  struct list_head *e, *next;
  struct cache_page *cp;
  
  /* Loop from the last node of the list and remove the first iterated
   * node, remove and return it. No other easy way to access the last element.
   */
  list_for_each_prev_safe(e, next, &cache_pages) 
  {
		cp = list_entry(e, struct cache_page, elem);
		list_del(&cp->elem);
		return cp;
  }
  return NULL;
}
