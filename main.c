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

int nframes;
int writesdisk = 0;
int readdisk = 0;
char *physmem;
struct page_table *pt;
struct disk *disk;
char *algorithm;
int npages;
struct list *head;
int *frame_tables;
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

void pop_first(){
	head->node = head->node->next;
}
//End linked lists

struct node *lPage(int page){
	struct node * node = head->node;
	while (node != NULL){
		if(node->page == page){
			return node;
		}
		node = node->next;
	}
	return NULL;
}


//Page_Fault_Handlers

void page_fault_handlerFIFO( struct page_table *pt, int page )
{
	int frame = head->node->value;
	int vpage = head->node->page;
	pop_first(head);
	struct node *next = head->node;
	append(frame, page, head->node, next);

	disk_write(disk, vpage, &physmem[frame * PAGE_SIZE]);
	writesdisk++;
	disk_read(disk, page, &physmem[frame * PAGE_SIZE]);
	readdisk++;

	page_table_set_entry(pt, vpage, frame, 0);
	page_table_set_entry(pt, page, frame, PROT_READ);

	page_table_print(pt);
	printf("page fault on page #%d\n",page);
	exit(1);
}
void page_fault_handlerLRU( struct page_table *pt, int page )
{
	printf("page fault on page #%d\n",page);
	exit(1);

}
void page_fault_handlerOUR( struct page_table *pt, int page )
{
	printf("page fault on page #%d\n",page);
	exit(1);
}


void page_fault_handler(struct page_table *pt, int page){
	struct node *loadedp = lPage(page);
	if(loadedp != NULL){
		page_table_set_entry(pt, page, loadedp->value, PROT_READ|PROT_WRITE);
	}
	int permission = 0;
	for (int i = 0 ; i <nframes; i++){
		if(frame_tables[i] != -1){
			disk_read(disk, page, &physmem[i * PAGE_SIZE]);
			readdisk++;
			page_table_set_entry(pt, page, i, PROT_READ);
			frame_tables[i] = -1;
			struct node *next = head->node;
			if(i == 0){
				head->node->value = 0;
				head->node->page = page;
				permission = 1;
				break;
			}
			append(i, page, head->node, next);
			permission = 1;
			break;
		}
	}
	if(permission != 1){
		if (strcmp(algorithm, "rand") == 0){
			pt = page_table_create( npages, nframes, page_fault_handlerLRU );
		}
		else if (strcmp(algorithm, "fifo") == 0){
			pt = page_table_create( npages, nframes, page_fault_handlerFIFO );
		}
		else if (strcmp(algorithm, "our") == 0){
			pt = page_table_create( npages, nframes, page_fault_handlerOUR );
		}
	}
}

int main( int argc, char *argv[] )
{
	if(argc!=5) {
		/* Add 'random' replacement algorithm if the size of your group is 3 */
		printf("use: virtmem <npages> <nframes> <lru|fifo> <sort|scan|focus>\n");
		return 1;
	}

	head = malloc(sizeof(struct list));
	head->node = malloc(sizeof(struct node));

	int npages = atoi(argv[1]);
	int nframes = atoi(argv[2]);
	algorithm = argv[3];
	const char *program = argv[4];

	struct disk *disk = disk_open("myvirtualdisk",npages);

	if(!disk) {
		fprintf(stderr,"couldn't create virtual disk: %s\n",strerror(errno));
		return 1;
	}
	//if the argv[3] it's the tipe of the handler we make
	if (!strcmp(algorithm, "rand")){
		fprintf(stderr, "Using rand algorithm");
		return 1;
	}
	else if (!strcmp(algorithm, "FIFO")){
		fprintf(stderr, "Using FIFO algorithm");
		return 1;

	}
	else if (!strcmp(algorithm, "custom")){
		fprintf(stderr, "Using custom algorithm");
		return 1;

	}

	else{
		fprintf(stderr,"algorithm error");
		exit(1);
	}

	if(!pt) {
		fprintf(stderr,"couldn't create page table: %s\n",strerror(errno));
		return 1;
	}
	char *virtmem = page_table_get_virtmem(pt);
	physmem = page_table_get_physmem(pt);

	//Frame table


	for (int i = 0; i < nframes; ++i){
		frame_tables[i] = i;
		disk_write(disk, i, &physmem[i * BLOCK_SIZE]);
	}

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
