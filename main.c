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

struct node *head;
struct node *node;
struct page_table *pt;
struct disk *disk;
//Linked lists
struct node{
	int frame;
	int page;
	struct node *next;
};

void popf(struct node **head){
	struct node * next_node = NULL;
	next_node = (*head)->next;
	free(*head);
	*head = next_node;
}


void popl(struct node *head){
	if(head->next == NULL){
			free(head);
	}
	struct node* current = head;
	while(current->next->next != NULL){
		current = current->next;
	}
	free(current->next);
	current->next = NULL;
}

void append(int nvalue, int npage, struct node * initial){
	struct node * next = initial;
	while (next -> next != NULL){
		next = next -> next;
	}
	next->next = malloc(sizeof(struct node));
	next = next->next;
	next->frame = nvalue;
	next->page = npage;
}


void print_list(struct node * list){
    struct node * current = list;
    while (current != NULL) {
        printf("%d\n", current->page);
				printf("marco %i\n", current->frame);
        current = current->next;
    }
}

void page_fault_handler_FIFO( struct page_table *pt, int page )
{
	printf("page fault on page #%d\n",page);
	node = head;
	int using_frame = -1;
	while(node != NULL && using_frame == -1){
		if(node->page == -1){
			using_frame = node->frame;
			node->page = page;
		}
		node = node->next;
	}
	char * physical_pointer;
	physical_pointer = page_rable_get_physmem(pt);

	if(using_frame == -1){
		node = head;
		int old_page = node->page;
		using_frame = node->frame;
		disk_write(disk, old_page, &physical_pointer[using_frame * PAGE_SIZE]);
		page_table_set_entry(pt, old_page, using_frame, 0);

		popl(head, page, using_frame);
		popf(&head);
	}
	if(using_frame != -1){
		page_table_set_entry(pt, page, using_frame, PROT_READ|PROT_WRITE);
		disk_read(disk, page, &physical_pointer[using_frame * PAGE_SIZE]);
	}
}
void page_fault_handler_RAND( struct page_table *pt, int page )
{
	printf("page fault on page #%d\n",page);
	exit(1);
}
void page_fault_handler_CUSTOM( struct page_table *pt, int page )
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
	char * algorithm = argv[4];
	const char *program = argv[4];

	disk = disk_open("myvirtualdisk",npages);
	if(!disk) {
		fprintf(stderr,"couldn't create virtual disk: %s\n",strerror(errno));
		return 1;
	}
	head->frame = 0;
	head->page = -1;
	head->next = NULL;

	for(int i = 0; i<nframes; i++){
		append(i+1,-1,head);
	}
	if (strcmp(algorithm, "rand") == 0){
			pt = page_table_create( npages, nframes, page_fault_handler_RAND );
		}
		else if (strcmp(algorithm, "fifo") == 0){
			pt = page_table_create( npages, nframes, page_fault_handler_FIFO );
		}
		else if (strcmp(algorithm, "custom") == 0){
			pt = page_table_create( npages, nframes, page_fault_handler_CUSTOM );
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
