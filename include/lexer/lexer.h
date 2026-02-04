/**
 * @file lexer.h
 * @author Abdellah
 * @brief Lexer header
 */
#ifndef LEXER_H
#define LEXER_H

#include <generic/list.h> 

/* lexer function (Sujet 4.3) :
    lex_defs : path to the file containing lexem definitions
    source_file : path to the source file to analyze
    returns a list of lexems found in the source file
*/
list_t lex(char *lex_defs, char *source_file);

/* lex_rule deletion callback */
int lex_rule_delete(void *ptr);

#endif