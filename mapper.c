#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/mtd/mtd.h>
#include <linux/vmalloc.h>
#include <linux/string.h>
#include "mapper.h"
#include "core.h"
#include "device.h"
#define PRINT_PREF KERN_INFO "[LKP_KV]: "

uint64_t *page_mapping;
char *page_metadata;
extern lkp_kv_cfg meta_config;
extern lkp_kv_cfg data_config;
struct freelist myfreelist;
struct freelist *freelist_top=NULL;
struct freelist *freelist_end=NULL;
/*
 *This function takes in virtual page and provides a new mapping,
 this should be called when a new page mapping is required.
 *
 * */
uint64_t get_mapping(uint64_t vpage)
{
	struct freelist *currenttop;
	int pagenumber=0;
	if(freelist_top==NULL)
		return -1;
	currenttop=freelist_top; 
	if(freelist_top!= freelist_end)
	{
		pagenumber = freelist_top -> pagenumber;
		freelist_top=freelist_top ->next;
	//	vfree(currenttop);/*Currently giving page fault if this is enabled*/
		update_pagemetadata(PAGE_VALID, pagenumber);
		page_mapping[vpage]=pagenumber;
		return pagenumber;
	}
	else{
		return -1;
	}
}
/*
 *Takes in virtual page and gives current vpage to actual
 page mapping. Returns -1 if mapping doesn't exist,
 if so call getmapping.
 *
 */
uint64_t map_page(uint64_t vpage)
{
	uint64_t actualpage=page_mapping[vpage];
	if(actualpage == 0xFFFFFFFFFFFFFFFF)
		return -1;
	else
		return actualpage;

}
/*
 *Takes in virtal page, and marks the corresponding
 pagemetadata as invalid.
 */
void mark_invalid(uint64_t vpage, int numpage)
{
	int i=0;
	uint64_t virtualpage =vpage;
	uint64_t actualpage =0;
	for(i=0;i<numpage;i++)
	{
		actualpage=page_mapping[virtualpage];
		update_pagemetadata(PAGE_INVALID,actualpage);
		virtualpage++;
	}
}

/*
 *This function takes in real page 
 number and updates its metadata to given val.
 Possible val should be PAGE_VALID, PAGE_INVALID,
 PAGE_FREE, PAGE_RECLAIMED.
 *
 * */
void update_pagemetadata(int val, int pagenumber)
{
	int indexoffset=pagenumber %4;
	int index= 4+ (pagenumber+1)%4;
	char currentdata;
	char mask=0xFF;
	char newmetadata= 0xFF;
	switch(indexoffset)
	{
		case 0: mask= 0xFC;
			break;
		case 1: mask = 0xF3;
			break;
		case 2: mask = 0xCF;
			break;
		case 3: mask = 0x3F;
			break;
	
	}
	switch(val)
	{
		case 0:	newmetadata=PAGE_FREE << (2* indexoffset);
			break;

		case 1: newmetadata=PAGE_VALID << (2* indexoffset);
			break;
		case 2: newmetadata=PAGE_INVALID << (2* indexoffset);
			break;
		case 3: newmetadata=PAGE_RECLAIMED << (2* indexoffset);
			break;
		default : break;
	
	}
	currentdata=page_metadata[index];
	newmetadata =mask | newmetadata;
	page_metadata[index] = currentdata & newmetadata;
}

/*
 *This function can be used to get
 the current state of any page from the metadata.
 *Takes in page number and gives page status
 */
int getcurrent_pagemetadata(int pagenumber){
	int indexoffset=pagenumber %4;
	int index= 4+ (pagenumber+1)%4;
	char currentdata=page_metadata[index];
	return ((currentdata >> (2*indexoffset))& 0x03);
}



/*
 *This function should be called after 
 garbage collection. This scans through entire
 metadata and adds RECLAIMED nodes back to list.
 */

void update_freelist(void)
{
	int total_data_pages= meta_config.nb_blocks *meta_config.pages_per_block;
	int pagemetadata_size=((total_data_pages*2)/8);
	int i=0;
	char currentdata=0;
	char mask=0x03;
	int pagenumber;
	struct freelist* newnode;
	int j=0;
	for(i=0;i<(pagemetadata_size); i++)
	{
			
		currentdata= page_metadata[i+4];
		for(j=0;j<4;j++){
			if(((mask << (2*j) | currentdata) >> (2* j)) == PAGE_RECLAIMED){
				
				pagenumber=i*4+j;
				newnode= (struct freelist*) vmalloc(sizeof(struct freelist));
				freelist_end->pagenumber=pagenumber;
				freelist_end->next=newnode;
				freelist_end->pagenumber=-1;
			}
		}
	}
}

/*
 *This should be called first time after format.
 This creates a list of  pagenumbers in free state.
 */

void init_freelist(void)
{
 	int total_data_pages= meta_config.nb_blocks *meta_config.pages_per_block;
	int i=0;
	struct freelist *node;
	freelist_top=&myfreelist;
	freelist_end= freelist_top;
	for(i=0;i<(total_data_pages); i++)
	{
		node= (struct freelist*) vmalloc(sizeof(struct freelist));
		freelist_end->pagenumber= i;
		freelist_end->next=node;
		freelist_end=freelist_end->next;
		freelist_end->pagenumber=-1;   

	}

}
/*
 *This function should be called after format.
 It initialized page metadata. Block 2 from partition 0(metadata)
 is used for this.
 *
 */

int init_pagemetadata(void)
{
	int total_data_pages= data_config.nb_blocks *data_config.pages_per_block;
	int pagemetadata_size=((total_data_pages*2)/8);
	int total_metadata_pages= (pagemetadata_size +4)/data_config.page_size;
	char *buffer;
	size_t retlen;
	uint64_t metadata_startpage= (uint64_t)(2* meta_config.pages_per_block);
	uint64_t addr;
	int i=0;
	int j=0;
	uint64_t *temp;
	buffer= (char *) vmalloc(meta_config.page_size * sizeof (char));
	temp=(uint64_t *)buffer;
	for(i=0;i<(total_metadata_pages+1);i++)
	{
		if(i==0){
			buffer[0]=0xef;
			buffer[1]=0xbe;
			buffer[2]=0xad;
			buffer[3]=0xde;
			for(j=4;j<meta_config.page_size;j++){
				buffer[j]=0x0;
			}	
		}else{
			for(j=0;j<meta_config.page_size;j++){
				buffer[j]=0x0;
			}	
		}

		addr= (uint64_t )metadata_startpage* (uint64_t )meta_config .page_size;
		if (meta_config.mtd->
		    _write(meta_config.mtd, addr, meta_config.page_size, &retlen, buffer) != 0)
		return -2;


		metadata_startpage++;
	}
	vfree(buffer);
	return 0;
}

/*
 *This function should be called during mounttime load.
 It loads the pagemetadata from memory.
 */

int mount_pagemetadata(void)
{
	int total_data_pages= data_config.nb_blocks *data_config.pages_per_block;
	int pagemetadata_size=((total_data_pages*2)/8);
	int total_metadata_pages= (pagemetadata_size +4)/data_config.page_size;
	char *buffer;
	uint64_t addr;
	int i=0;
	size_t retlen;
	int metadata_startpage= 2* meta_config.pages_per_block;
	char *currentposition;
	buffer= (char *) vmalloc(meta_config.page_size*sizeof(char));
	page_metadata= (char *) vmalloc((total_data_pages +1)* meta_config.page_size);
	currentposition=(char *)page_metadata;
	for(i=0;i<(total_metadata_pages+1);i++){
		addr= (uint64_t )metadata_startpage* (uint64_t )meta_config .page_size;	
		if(meta_config.mtd->_read(meta_config.mtd, addr, meta_config.page_size, &retlen,
						 buffer)!=0)
			return -2;

		metadata_startpage++;

		memcpy(currentposition, buffer, meta_config.page_size);
		currentposition=currentposition+meta_config.page_size*sizeof(char);

	}

	
	vfree(buffer);
	return 0;
}
/*
 *
 * This function should be called after format. It
 * creates a metadata for pagemapping.
 * Block number 3,4,5 & 6(partially) contain the pagemapping
 * */
int init_pagemapping(void)
{
	int mappingsize=data_config.nb_blocks * data_config.pages_per_block;
       	int blockcount= (mappingsize * sizeof(uint64_t))/(data_config.page_size*data_config.pages_per_block);	
	int i=0;
	int j=0;
	int k=0;
	uint64_t addr;
	int metadata_startpage= (uint64_t)(3* meta_config.pages_per_block);
	char *buffer;
	size_t retlen;
	buffer= (char *) vmalloc(meta_config.page_size);
	for(k=0;k<meta_config.page_size;k++){
		buffer[k]=0xFF;
	}
	for(i=0;i<(blockcount+1);i++)
	{
		for(j=0;j<meta_config.pages_per_block;j++)
		{
			addr= (uint64_t )metadata_startpage* (uint64_t )meta_config .page_size;
		
			if (meta_config.mtd->
		    		_write(meta_config.mtd, addr, meta_config.page_size, &retlen, 
					buffer) != 0)
				return -2;
		
			metadata_startpage++;	
		}	
	
	}
	vfree(buffer);

	return 0;

}
/*
 * Read pagemapping. S
 * should be called during mount time. 
 * Block number 3,4,5 & 6(partially) contain the pagemapping*/
int mount_pagemapping(void)
{
	
	int mappingsize=data_config.nb_blocks * data_config.pages_per_block;
       	int blockcount= (mappingsize * sizeof(uint64_t))/(data_config.page_size*data_config.pages_per_block);	
	int i=0;
	int j=0;
	int metadata_startpage= 3* meta_config.pages_per_block;
	char *buffer;
	uint64_t addr;
	size_t retlen;
	char *currentposition;
	buffer= (char *) vmalloc(meta_config.page_size*sizeof(char));
	page_mapping=(uint64_t *) vmalloc((blockcount+1) * (meta_config.pages_per_block)*(meta_config.page_size));
	currentposition=(char *)page_mapping;	
	for(i=0;i<(blockcount+1);i++)
	{
		for(j=0;j<meta_config.pages_per_block;j++)
		{
			addr= (uint64_t )metadata_startpage* (uint64_t )meta_config .page_size;	
			if (meta_config.mtd->
		    		_read(meta_config.mtd, addr, meta_config.page_size, &retlen, 
					buffer) != 0)
				return -2;
			memcpy(currentposition, buffer, meta_config.page_size);
			currentposition=currentposition+meta_config.page_size*sizeof(char);
			metadata_startpage++;	
		}	
	
	}

	vfree (buffer);

	return 0;

}
/*
 *This function should be called during
 module exit to destroy memory of pagemapping
 THIS FUNCTION NEEDS TO BE FIXED.
 */
void destroy_pagemapping(void)
{
	struct freelist* currenttop;
	currenttop=freelist_top;
	while(freelist_top!=freelist_end)
	{
		currenttop=freelist_top;
		vfree(currenttop);
		freelist_top=freelist_top->next;
	}
	vfree(freelist_top);

}
/*
This function should be called during module exit time
to destroy local pagemetadata. Ofcourse, pagemetadata
should be stored in flash before destroying.

THIS FUNCTION NEEDS TO BE FIXED.

*/

void destroy_pagemetadata(void)
{
	int i=0;
	int j=0;
	int total_data_pages= meta_config.nb_blocks *meta_config.pages_per_block;
	int pagemetadata_size=((total_data_pages*2)/8);
	int total_metadata_pages= (pagemetadata_size +4)/meta_config.page_size;
	char *mypointer=page_metadata;
	char *prevpointer=mypointer;
	for(i=0;i<(total_metadata_pages+1);i++)
	{
		for(j=0;j<(meta_config.page_size);j++){
		
				vfree(prevpointer);
				mypointer++;
				prevpointer=mypointer;
		
		}
	
	}

	
}
/*
 *This function stores the current metadata in to flash
 Should be called before module exit.
 *
 */
void store_pagemetadata(void)
{


}

/*
 *This function stores pagemapping in to flash.
 Should be called before module exit.
 */

void store_pagemapping(void)
{

}

