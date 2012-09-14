/*
 * (C)opyright 2007 Nico Golde <nico@ngolde.de>
 * See LICENSE file for license details
 * This is no complete linked list implementation, it is just used
 * to get a list of directory entries and delete the list
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>

#include "list.h"

/* create a new list */
static list_t *
new_list(void){
	list_t *l = malloc(sizeof(list_t));
	if(!l) return NULL;
	l->top = l->last = NULL;
	l->length = 0;
	return l;
}

static void
append_node(list_t *lst, char *name){
	node_t *n;
	if(!lst) return;
	if((n = malloc(sizeof(node_t))) == NULL) return;
	if((n->name = strdup(name)) == NULL) {
		free(n);
		return;
	}
	n->next = NULL;
	if(lst->top){
		lst->last->next = n;
		lst->last = lst->last->next;
	} else {
		lst->top = lst->last = n;
	}
	lst->length++;
}

/* delete the whole list */
void
delete_list(list_t *lst){
	node_t *tmp;
	tmp = lst->top;
	while(tmp != NULL){
		lst->top = tmp->next;
		if(tmp->name) free(tmp->name);
		free(tmp);
		tmp = lst->top;
	}
	lst->top = lst->last = NULL;
	free(lst);
}

/* return a linked list with directory entries or NULL on error */
list_t *
dir_list(char *dir){
	list_t *list = new_list();
	DIR *rddir = NULL;
	struct dirent *rd;

	if((rddir = opendir(dir)) == NULL)
		return NULL;
	while((rd = readdir(rddir))){
		if(!strncmp(".", rd->d_name, 1) || !strncmp("..", rd->d_name, 2))
			continue;

		append_node(list, rd->d_name);
	}
	closedir(rddir);
	return list;
}
