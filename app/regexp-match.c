/**
 * @file regexp.c
 * @author François Cayre <francois.cayre@grenoble-inp.fr>
 * @date Fri Jul  28 09:56:03 2022
 * @brief Regexp parsing and matching.
 *
 * Program for regexp parsing, and regexp matching.
 */

#include <stdlib.h>
#include <stdio.h>

#include <regexp/regexp.h>
#include <regexp/chargroup.h>

int main ( int argc, char *argv[] ) {
  char     *end = NULL;
  int  is_match;

  if ( argc < 3 ) {
    fprintf( stderr, "Usage :\n\t%s regexp text\n", argv[ 0 ] );
    exit( EXIT_FAILURE );
  }

  is_match = re_match( argv[ 1 ], argv[ 2 ], &end );

  if ( is_match ) {
    printf( "The start of '%s' is %s, %s: '%s'.\n", argv[2], argv[ 1 ], *end ? "next" : "END", end );
  }
  else {
    list_t parsed = re_read( argv[ 1 ] );

    if ( NULL == parsed ) {
      fprintf( stderr, "Invalid regexp: '%s'.\n", argv[ 1 ] );
      exit( EXIT_FAILURE );
    }

    list_delete( parsed, chargroup_delete_cb );
    printf( "The start of '%s' is *NOT* %s.\n", argv[2], argv[ 1 ] );
  }

  exit( EXIT_SUCCESS );
}
