/**
 * @file lex.c
 * @brief Main program for Lexical Analysis (Livrable 2)
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <lexer/lexem.h>
#include <generic/list.h>
#include <lexer/lexer.h>




// wrapper function just a callback for lexem_print and goes back to the line for lisibility
void print_lexem_wrapper(void *ptr) {
    lexem_print(ptr);
    printf("\n");
}
void delete_lexem_wrapper(void *ptr) {
    lexem_delete(ptr);
}

int main(int argc, char *argv[]) {
    // arguments verif (2 files)
    if (argc != 3) {
        fprintf(stderr, "Usage:\n\t%s <lex_definitions_file> <source_file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // lesgooo
    list_t lexems = lex(argv[1], argv[2]);

    if (lexems == NULL) {
       
        return EXIT_FAILURE;
    }

    // SHOW TIME
    // we go through the list to show each lexem
    if (list_is_empty(lexems)) {
        printf("Result: No lexems found (empty file or full of comments).\n");
    } else {
        printf("--- Lexems list ---\n");
        
        list_print(lexems, print_lexem_wrapper); 
        
        
    }

    // cleaning
    list_delete(lexems, delete_lexem_wrapper);

    return EXIT_SUCCESS;
}