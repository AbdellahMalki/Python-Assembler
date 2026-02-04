/**
 * @file list.c
 * @author François Cayre <francois.cayre@grenoble-inp.fr>
 * @brief Lists.
 *
 * Lists.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <generic/list.h>

/*

  # Generic data structures in C

  So far, you have learned how to program a list-of-some-objects.
  If you need a list-of-some-other-kind-of-objets, you need to
  copy-paste all the code for lists and adapt it. This is fairly
  error-prone.

  You want to program the *idea* of a list instead, no matter what is
  stored inside. Hence, with the very same set of functions, you get
  lists of any kinds of objects. This is called *genericity*.

  There are two main possible ways to implement a (pretty weak) form of
  genericity in C: macros and generic pointers. We describe the second
  option here.

  A generic pointer in C has type `void*` -- an address-of-anything.
  For instance, `malloc(3)` and friends return generic pointers.

  Technically, the links of a generic list will have generic pointers
  as their contents. The programmer will only access addresses of
  objects, of which s/he must only know how to process it (e.g., for
  deleting, printing.)
 */

struct link_t {
  void          *contents;
  struct link_t *next;
};

/*
  The following functions are independent of the type of objects
  that are stored in links. They only pertain to the sole notion of
  a list. This is our core list interface.
 */
list_t list_new( void ) {
  return NULL;
}

int    list_is_empty( list_t l ) {
  return list_new() == l;
}

void*  list_first( list_t l ) {

  assert( !list_is_empty( l ) );

  return l->contents;
}

list_t list_next( list_t l ) {

  assert( !list_is_empty( l ) );

  return l->next;
}

list_t list_add_first( void* object, list_t l ) {
  struct link_t *new = malloc( sizeof( *new ) );

  assert( new );

  new->contents = object;
  new->next     = l;

  return new;
}

list_t list_add_last(void* object, list_t l) {

  /* If list is empty */
  if (list_is_empty(l)) {
    return list_add_first(object, l);
  }

  /* running through the list until the last link */
  list_t cur = l;
  while (!list_is_empty(list_next(cur))) {
    cur = list_next(cur);
  }

  /* Add after the last */
  cur->next = list_add_first(object, list_next(cur));

  return l;
}

list_t list_reverse( list_t l ) {
  list_t prev = list_new();

  while ( !list_is_empty( l ) ) {
    list_t next = l->next;
    l->next = prev;
    prev = l;
    l = next;
  }

  return prev;
}


/*
  All functions below only use the core interface. They are of a
  higher-level.
 */

size_t list_length( list_t l ) {
  size_t len;

  for ( len = 0 ; !list_is_empty( l ) ; l = list_next( l ), len++ );

  return len;
}

/*
  The following function are dependent on the type of objects that
  we store the addresses of in the links.

  This means we should be able to parameterize the behaviour of
  our code depending on the type of objects in the list.

  The two main behaviours we need to parameterize are:
  - deleting an object,
  - printing an object.

  A function that encodes the behaviour (deleting, printing) of
  a given object type is called a *callback*.

  Callbacks will be implemented using function pointers.

  A function pointer is a variable that may store any function of
  the same type.

  Just like in maths, the type of a function is given by the type
  of its return value and the list of the types of its arguments.

  For instance, the trigonometric functions:

  ```C
  double cos( double );
  double sin( double );
  double tan( double );
  ```

  are of the same type: they take a double and they return also
  one.

  For simplicity, we will use only kind of callback: a function
  that will take a generic pointer as its only argument and that
  returns an integer, see `include/generic/callbacks.h`:

  ```C
  typedef int (*action_t) (void*);
  ```

  Any function that takes a single `void*` argument and that
  returns an `int` qualifies as a valid `action_t`.

  See `test/0-list.c` for the definition of two example callbacks.

  Everytime we need to parameterize the behaviour to a specific
  kind of object, we call back (hence the name) the `action_t`
  that is passed to the function:
 */
list_t list_del_first( list_t l, action_t delete_ ) {
  list_t next;

  assert( !list_is_empty( l ) );

  next = list_next( l );

  /* Execute deletion callback if it exists: */
  if ( delete_ ) delete_( list_first( l ) );

  free( l );

  return next;
}

void   list_delete( list_t l, action_t delete_ ) {
  for ( ; !list_is_empty( l ) ; l = list_del_first( l, delete_ ) );
}

int    list_print( list_t l, action_t print ) {
  /* A printing function should return the number of characters it
     has printed.
  */
  int nchars = printf( "%s", list_is_empty( l ) ? "" : " " );

  for ( ; !list_is_empty( l ) ; l = list_next( l ) ) {
    nchars += print ? print( list_first( l ) ) : printf( "#OBJECT#" );
    nchars += printf( " " );
  }

  return nchars + printf( "" );
}

void*  list_get_at( list_t l, size_t index ) {
  size_t i;

  for ( i = 0 ; i < index ; i++ ) {
    assert( !list_is_empty( l ) );
    l = list_next( l );
  }

  assert( !list_is_empty( l ) );

  return list_first( l );
}
