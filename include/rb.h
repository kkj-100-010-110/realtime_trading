#ifndef _RED_BLACK_TREE_H_
#define _RED_BLACK_TREE_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#include "queue.h"

#define LEFT 0
#define RIGHT 1

/* For print() */
#define TEXT_RED "\033[0;31m"
#define TEXT_RESET "\033[0m"

/* UTILITY */
int comparison_str(const void *p1, const void *p2);
int comparison_int(const void *p1, const void *p2);
void free_key(void *key);
void free_data(void *data);

/* DATA TYPE */
typedef enum color_e { BLACK=0, RED=1 } color_t;
typedef int (*compare_func_t)(const void*, const void*);
typedef void (*key_free_func_t)(void *);
typedef void (*data_free_func_t)(void *);

typedef struct rb_tree_s {
	struct rb_node_s *root;
	compare_func_t compare;
	key_free_func_t key_free;
	data_free_func_t data_free;
} rb_tree_t;

typedef struct rb_node_s {
	void	*key;
	void	*data;
	color_t	color;
	int		size; // subtree count
	struct rb_node_s *link[2];
} rb_node_t;

/* BASIS */
size_t rb_size_t(rb_tree_t *t);
size_t rb_size_n(rb_node_t *n);
rb_tree_t *rb_create(int (*comparison)(const void*, const void*), void (*key_free)(void *), void (*data_free)(void *));
rb_node_t *rb_node_create(void *key, void *data);
bool rb_contain(rb_tree_t *t, void *key);
bool rb_is_red(rb_node_t *n);
void *rb_max_t(rb_tree_t *t);
void *rb_min_t(rb_tree_t *t);
rb_node_t *rb_max_n(rb_node_t *n);
rb_node_t *rb_min_n(rb_node_t *n);
rb_node_t *rb_find(rb_node_t *n, compare_func_t f, void *key);

/* INSERTION */
void rb_insert(rb_tree_t *t, void *key, void *data);
void _rb_insert(rb_node_t **n, compare_func_t f, data_free_func_t df, void *key, void *data);

/* DELETION */
void rb_remove(rb_tree_t *t, void *key);
void _rb_remove(rb_node_t **n, compare_func_t f, key_free_func_t kf, data_free_func_t df, void *key);
void rb_remove_tree(rb_tree_t *t);

void rb_remove_min(rb_tree_t *t);
void rb_remove_max(rb_tree_t *t);
void _rb_remove_min(key_free_func_t kf, data_free_func_t df, rb_node_t **n);
void _rb_remove_max(key_free_func_t kf, data_free_func_t df, rb_node_t **n);

void rb_move_red_left(rb_node_t **n);
void rb_move_red_right(rb_node_t **n);

void fix_up(rb_node_t **n);

void free_key_and_data(key_free_func_t kf, data_free_func_t df, rb_node_t **n);

/* ROTATIONS */
void rb_rotate_left(rb_node_t **n);
void rb_rotate_right(rb_node_t **n);

/* COLOR FLIP */
void rb_color_flip(rb_node_t **n);
color_t rb_color(rb_node_t *n);
color_t rb_color_change(color_t c);

/* PRINT */
void rb_print(rb_tree_t *t, int order);
void rb_print_node(rb_node_t *n, int order);
void _print_preorder(rb_node_t *n);
void _print_inorder(rb_node_t *n);
void _print_postorder(rb_node_t *n);
void _print_level_i(rb_node_t *n);
void _print_level_s(rb_node_t *n);
void _print_inorder_string(rb_node_t *n);

#endif//_RED_BLACK_TREE_H_
