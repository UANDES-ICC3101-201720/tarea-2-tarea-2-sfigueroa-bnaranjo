/*
Main program for the virtual memory project.
Make all of your modifications to this file.
You may add or rearrange any code or data as you need.
The header files page_table.h and disk.h explain
how to use the page table and disk interfaces.
*/

#include "page_table.h"
#include "disk.h"
#include "program.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
//Create the structure disk that we will need
struct disk *disk;

//Create the linked list for poping and append easily
struct node{
	int value;
	int page;
	struct node *next;
};

struct list{
	struct node *node;
};

void pop(struct node *initial){
	struct node *next = initial;
	struct node *before;
	while(next->next != NULL){
		before = next;
		next = next->next;
	}
	before->next = NULL;
}

void append(int nvalue, int npage, struct node * initial, struct node * next){
	next = initial;
	if (next != 0){
		while (next -> next != 0){
			next = next -> next;
		}
	}
	next->next = malloc(sizeof(struct node));
	next = next->next;
	next->value = nvalue;
	next->page = npage;
}

void pop_first(struct node *initial){
	struct node *next = initial;
	initial->value = next->value;
	initial->page = next->page;
	initial->next = next->next;
}

//Page_Fault_Handlers
void page_fault_handlerFIFO( struct page_table *pt, int page )
{
	printf("page fault on page #%d\n",page);
	exit(1);
}
void page_fault_handlerLRU( struct page_table *pt, int page )
{

	page_table_set_entry(pt, page, page, PROT_READ|PROT_WRITE);
	page_table_print(pt);
	page_table_get_entry(pt, 0, 0, "wr");
	printf("page fault on page #%d\n",page);
	exit(1);

}
void page_fault_handlerOUR( struct page_table *pt, int page )
{
	printf("page fault on page #%d\n",page);
	exit(1);
}

int main( int argc, char *argv[] )
{
	if(argc!=5) {
		/* Add 'random' replacement algorithm if the size of your group is 3 */
		printf("use: virtmem <npages> <nframes> <lru|fifo> <sort|scan|focus>\n");
		return 1;
	}

	int npages = atoi(argv[1]);
	int nframes = atoi(argv[2]);
	char *algorithm = argv[3];
	const char *program = argv[4];

	struct disk *disk = disk_open("myvirtualdisk",npages);
	if(!disk) {
		fprintf(stderr,"couldn't create virtual disk: %s\n",strerror(errno));
		return 1;
	}
	//if the argc[3] it's the tipe of the handler we Make
	struct page_table *pt;
	if (strcmp(algorithm, "rand") == 0){
		pt = page_table_create( npages, nframes, page_fault_handlerLRU );
	}
	else if (strcmp(algorithm, "fifo") == 0){
		pt = page_table_create( npages, nframes, page_fault_handlerFIFO );
	}
	else if (strcmp(algorithm, "our") == 0){
		pt = page_table_create( npages, nframes, page_fault_handlerOUR );
	}

	else{
		fprintf(stderr,"algorithm error");
		exit(1);
	}
	//Frame table
	int frame_tables[nframes];
	for (int i = 0; i < nframes; ++i){
		frame_tables[i] = i;
	}


	if(!pt) {
		fprintf(stderr,"couldn't create page table: %s\n",strerror(errno));
		return 1;
	}
	char *virtmem = page_table_get_virtmem(pt);

	char *physmem = page_table_get_physmem(pt);

	if(!strcmp(program,"sort")) {
		sort_program(virtmem,npages*PAGE_SIZE);

	} else if(!strcmp(program,"scan")) {
		scan_program(virtmem,npages*PAGE_SIZE);

	} else if(!strcmp(program,"focus")) {
		focus_program(virtmem,npages*PAGE_SIZE);

	} else {
		fprintf(stderr,"unknown program: %s\n",argv[3]);

	}

	page_table_delete(pt);
	disk_close(disk);

	return 0;
}
