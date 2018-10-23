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

struct node *head = NULL;

struct disk *disk;

int nframes;
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

void popi(struct node ** head, int index){
	struct node * current = *head;
	struct node * temp = NULL;
	if (index ==0){
		popf(head);
		return;
	}
	for(int i = 0; i < index-1; i++){
		current = current->next;
	}
	temp = current->next;
	current->next = temp->next;
	free(temp);
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

void push_lfr(struct node * head, int page, int frame){
	struct node * current = head;
	while(current->next != NULL){
		current = current->next;
	}
	current->next = malloc(sizeof(struct node));
	current->next->page = page;
	current->next->frame = frame;
	current->next->next = NULL;
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
	struct node *node = head;
	int using_frame = -1;
	while(node != NULL && using_frame == -1){
		if(node->page == -1){
			using_frame = node->frame;
			node->page = page;
		}
		node = node->next;
	}
	char * physical_pointer;
	physical_pointer = page_table_get_physmem(pt);

	if(using_frame == -1){
		node = head;
		int old_page = node->page;
		using_frame = node->frame;
		disk_write(disk, old_page, &physical_pointer[using_frame * PAGE_SIZE]);
		page_table_set_entry(pt, old_page, using_frame, 0);
		push_lfr(head, page, using_frame);
		popf(&head);
	}
	if(using_frame != -1){
		page_table_set_entry(pt, page, using_frame, PROT_READ|PROT_WRITE);
		disk_read(disk, page, &physical_pointer[using_frame * PAGE_SIZE]);
	}
}
void page_fault_handler_RAND( struct page_table *pt, int page )
{
	struct node *node = head;
	int using_frame = -1;
	char * physical_pointer;
	physical_pointer = page_table_get_physmem(pt);

	if(using_frame == -1){
		int ran_num = lrand48()%nframes;
		int ran_num2 = ran_num;
		while(ran_num > 0){
			node = node->next;
			ran_num--;
		}
		using_frame = node->frame;
		int old_page = node->page;
		if(old_page != -1){
			disk_write(disk, old_page, &physical_pointer[using_frame * PAGE_SIZE]);
			page_table_set_entry(pt, old_page, using_frame, 0);
		}
		popi(&head, ran_num2);
		push_lfr(head, page, using_frame);
	}
	if(using_frame != -1){
		page_table_set_entry(pt, page, using_frame, PROT_READ|PROT_WRITE);
		disk_read(disk, page, &physical_pointer[using_frame * PAGE_SIZE]);
	}
		page_table_print(pt);
}

void page_fault_handler_CUSTOM( struct page_table *pt, int page )
{
	printf("page fault on page #%d\n",page);
	exit(1);
}



int main( int argc, char *argv[] )
{
	int npages;
	int nframes;
	char * algorithm;
	const char *program;

	if(argc==5) {
		npages = atoi(argv[1]);
		nframes = atoi(argv[2]);
		algorithm = argv[3];
		program = argv[4];
	}
	else if(argc==9) {
		for (int i = 1; i < argc; i++) {
			if(i+1 != argc) {
				if(strcmp(argv[i], "-n") == 0) {
					npages = atoi(argv[i+1]);
					i++;
				}
				else if(strcmp(argv[i], "-f") == 0) {
					nframes = atoi(argv[i+1]);
					i++;
				}
				else if(strcmp(argv[i], "-a") == 0) {
					algorithm = argv[i+1];
					i++;
				}
				else if(strcmp(argv[i], "-p") == 0) {
					program = argv[i+1];
					i++;
				}
			}
		}
	}
	else {
		/* Add 'random' replacement algorithm if the size of your group is 3 */
		printf("use: virtmem <npages> <nframes> <lru|fifo> <sort|scan|focus>\n");
		return 1;
	}

	disk = disk_open("myvirtualdisk",npages);
	if(!disk) {
		fprintf(stderr,"couldn't create virtual disk: %s\n",strerror(errno));
		return 1;
	}

	head = malloc(sizeof(struct node));
	head->frame = 0;
	head->page = -1;
	head->next = NULL;

	for(int i = 1; i<nframes; i++){
		push_lfr(head, -1, i);
	}
	struct page_table *pt;
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
		fprintf(stderr,"unknown program: %s\n",program);

	}

	page_table_delete(pt);
	disk_close(disk);

	return 0;
}
