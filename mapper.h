#define PAGE_FREE 0x00
#define PAGE_VALID 0x01
#define PAGE_INVALID 0x02
#define PAGE_RECLAIMED 0x03

struct freelist{

	uint64_t pagenumber;
	struct freelist * next; 	
}; 
uint64_t get_mapping(uint64_t vpage);
uint64_t map_page(uint64_t vpage);
void mark_invalid(uint64_t vpage, int numpage);
void update_pagemetadata(int val, int pagenumber);
int getcurrent_pagemetadata(int pagenumber);
void update_freelist(void);
void init_freelist(void);
int init_pagemetadata(void);
int mount_pagemetadata(void);
int init_pagemapping(void);
int mount_pagemapping(void);
void destroy_pagemapping(void);
void destroy_pagemetadata(void);
void store_pagemetadata(void);
void store_pagemapping(void);
