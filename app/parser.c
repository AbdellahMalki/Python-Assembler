/**
 * @file parser.c
 * @brief Main program for parser (Livrable 3)
 */

#include <stdlib.h>
#include <stdio.h>

#include <generic/list.h>
#include <lexer/lexem.h>
#include <lexer/lexer.h>
#include <parser/parser.h>
#include <parser/pyobj.h>


int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Arguments insuffisants ! \n");
        exit(EXIT_FAILURE);
    }
    char *LEX = argv[1];
    char *source_file = argv[2];

    list_t lexems = lex(LEX, source_file);

    if (NULL == lexems) {
        // The lexer already prints the lexical error
        return EXIT_FAILURE;
    }

    // Keep a copy of the list head so we can free nodes later. 
    list_t lexems_head = lexems;
    pyobj_t code = parse_pys(&lexems);
    if (NULL == code) {
        // The parser already prints the parse error
        list_delete(lexems_head, lexem_delete);
        return EXIT_FAILURE;
    }

    pyobj_print_all_recursif(code);
    printf("\n");

    // Free memory
    pyobj_delete(code);
    printf("parsing reussi \n");
    list_delete(lexems_head, lexem_delete);
    return EXIT_SUCCESS;
}