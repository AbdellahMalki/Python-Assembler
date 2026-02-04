/**
 * @file lexem.h
 * @author François Cayre <francois.cayre@grenoble-inp.fr>
 * @brief Lexems.
 *
 * Lexems.
 */

#ifndef LEXEM_H
#define LEXEM_H

#ifdef __cplusplus
extern "C" {
#endif

  /*
    This is called a 'forward declaration': the actual definition of  a
    'struct lexem' is in lexem.c:16, and we only manipulate pointers to
    such lexem structures (observe the star in the typedef). This makes
    the definition of functions using this type legit, because pointers
    have all the same size.
  */

  /* Opaque type: users see only a pointer to struct lexem. The full
     definition is in src/lexer/lexem.c. This prevents users from
     depending on the structure layout. */
  typedef struct lexem *lexem_t;

  /*
    The end goal is to hide the definition to the user, so as to force
    him/her to use the functions below and s/he cannot tamper with the
    structure contents directly.

    This technique is called setting up an 'opaque type'.

    This is the exactly the same technique that is used in the Standard
    Library with the FILE* type: you do not want to know what's inside,
    you only need using fopen, fclose and friends to manipulate files.
  */

  /* Constructor */
  lexem_t lexem_new( char *type, char *value, int line, int column );


  const char *lexem_type( lexem_t lex );
  const char *lexem_value( lexem_t lex );
  int         lexem_line( lexem_t lex );
  int         lexem_column( lexem_t lex );



  /* Callbacks */
  int     lexem_print( void *_lex );
  int     lexem_delete( void *_lex );

  //type detectors (added)
  int lexem_is_type_strict(lexem_t lex, char *type);
  int lexem_is_type(lexem_t lex, char *type);


  //lexem equality tester (added)
  int lexem_is_egal( lexem_t lex1, lexem_t lex2 );

#ifdef __cplusplus
}
#endif

#endif
