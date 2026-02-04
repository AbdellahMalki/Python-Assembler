/**
 * @file lexem.c
 * @author FranÃ§ois Cayre <francois.cayre@grenoble-inp.fr>
 * @brief Lexems.
 *
 * Lexems.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <lexer/lexem.h>

struct lexem {
  char *type;
  char *value;
  int   line;    /* Start at line 1   */
  int   column;  /* Start at column 0 */
};

const char *lexem_type( lexem_t lex ) {
  return lex ? lex->type : NULL;
}

const char *lexem_value( lexem_t lex ) {
  return lex ? lex->value : NULL;
}

int lexem_line( lexem_t lex ) {
  return lex ? lex->line : 0;
}

int lexem_column( lexem_t lex ) {
  return lex ? lex->column : 0;
}

/*
  Constructor and callbacks for lists/queues of lexems:
 */
lexem_t lexem_new( char *type, char *value, int line, int column ) {
  lexem_t lex = calloc( 1, sizeof( *lex ) );

  assert( lex );

  if ( type  && *type  ) lex->type  = strdup( type );
  if ( value && *value ) lex->value = strdup( value );

  lex->line   = line;
  lex->column = column;

  return lex;
}

int     lexem_print( void *_lex ) {
  lexem_t lex = _lex; /* Start by casting to actual type */

  return printf( "[%d:%d:%s] %s",
         lex->line,
         lex->column,
         lex->type,
         lex->value );
}

int     lexem_delete( void *_lex ) {
  lexem_t lex = _lex;

  if ( lex ) {
    free( lex->type );
    free( lex->value );
  }

  free( lex );

  return 0;
}

//type detection functions
int lexem_is_type_strict(lexem_t lex, char *type) {
    if (NULL == lex || NULL == type) return 0;
    return !strcmp(lex->type, type);
}

int lexem_is_type(lexem_t lex, char *type) {
    if (NULL == lex || NULL == type) return 0;
    
    return lex->type == strstr(lex->type, type);
}


int lexem_is_egal( lexem_t lex1, lexem_t lex2 ) {
    if (strcmp(lex1->type, lex2->type) != 0)
        return 0;
    if (strcmp(lex1->value, lex2->value) != 0)
        return 0;
    if (lex1->line != lex2->line)
        return 0;
    if (lex1->column != lex2->column)
        return 0;
    return 1;
}
