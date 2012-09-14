/*
 * (C)opyright 2007 Nico Golde <nico@ngolde.de>
 * See LICENSE file for license details
 */

/**
 * \file list.h
 * \brief linked list
 */

/**
 * \struct node_t
 * \brief node for linked list
 */
typedef struct node{
	char *name;         /**< node name */
	struct node *next;  /**< pointer to the next node */
} node_t;

/**
 * \struct list_t
 * \brief linked list
 */
typedef struct{
	int length;         /**< list length */
	node_t *top;        /**< pointer to top node */
	node_t *last;       /**< pointer to last node */
} list_t;

/**
 * Lists contents (for libacpi directories) of a directory
 * and return them in a linked list
 * @param dir directory to list
 * @return linked list
 */
list_t *dir_list(char *dir);

/**
 * Delete linked list
 * @param lst list to delete
 */
void delete_list(list_t *lst);
