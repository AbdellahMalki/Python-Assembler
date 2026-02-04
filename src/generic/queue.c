/**
 * @file queue.c
 * @author François Cayre <francois.cayre@grenoble-inp.fr>
 * @brief Queue.
 *
 * Queue.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h> /* NULL */

#include <generic/queue.h>

struct link_t {
  void          *content;
  struct link_t *next;
};

queue_t queue_new( void ) {
  return NULL;
}

int     queue_empty( queue_t q ) {
  return queue_new() == q;
}

queue_t enqueue( queue_t q, void* object ) {
  struct link_t *new_node = malloc(sizeof(struct link_t));
  new_node->content = object;
  new_node->next = NULL;
  if (!q) {
    return new_node;
  }
  struct link_t *tail = q;
  while (tail->next) tail = tail->next;
  tail->next = new_node;
  return q;
}

list_t  queue_to_list( queue_t q ) {
  list_t l = list_new();
  struct link_t *cur = q;
  while (cur) {
    l = list_add_last(cur->content, l);
    struct link_t *temp = cur;
    cur = cur->next;
    free(temp);
  }
  return l;
}
