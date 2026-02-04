/**
 * @file pyas.c
 * @brief Point d'entrée de l'assembleur Python.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <lexer/lexem.h>
#include <lexer/lexer.h>
#include <parser/parser.h>
#include <parser/pyobj.h>
#include <generic/list.h>


int pyasm(pyobj_t code); 
int pyobj_write(FILE *fp, pyobj_t obj);

// Magic Number for Python 2.7 : 03 F3 0D 0A

#define PY27_MAGIC_NUMBER 0x0A0DF303 

int main(int argc, char *argv[]) {
    //(prog + source + output + lex)
    if (argc != 4) {
        printf("Arguments insuffisants !\n");
        return EXIT_FAILURE;
    }

    char *source_filename = argv[2];
    char *output_filename = argv[3];
    char *lex_rules_filename = argv[1]; 

//analyse lexicale
    
    list_t lexems = lex(lex_rules_filename,source_filename);

    if (list_is_empty(lexems)) {
        fprintf(stderr, "Erreur Lexer \n");
        return EXIT_FAILURE;
    }

//analyse synthaxique 
    //parser
    list_t lexems_cursor = lexems; 
    pyobj_t code_obj = parse_pys(&lexems_cursor);

    if (!code_obj) {
        fprintf(stderr, "Erreur de syntaxe (Parser failed).\n");
        list_delete(lexems, lexem_delete);
        return EXIT_FAILURE;
    }

    
    // ASSEMBLING !!!¤ (Pyasm + Lnotab)
   
    if (pyasm(code_obj) < 0) {
        fprintf(stderr, "Erreur lors de l'assemblage.\n");
        pyobj_delete(code_obj);
        list_delete(lexems, lexem_delete);
        return EXIT_FAILURE;
    }

    //SERIALISER 

    FILE *dest_fp = fopen(output_filename, "wb");
    if (!dest_fp) {
        perror("Erreur ouverture destination");
        pyobj_delete(code_obj);
        list_delete(lexems, lexem_delete);
        return EXIT_FAILURE;
    }

    //magic number
    fputc(0x03, dest_fp);
    fputc(0xf3, dest_fp);
    fputc(0x0d, dest_fp);
    fputc(0x0a, dest_fp);

    //TIMESTAMP
    uint32_t timestamp = (uint32_t)time(NULL);
    fwrite(&timestamp, 4, 1, dest_fp);

    // code section serialisation 
    if (pyobj_write(dest_fp, code_obj) < 0) {
        fprintf(stderr, "Erreur lors de l'écriture du .pyc\n");
        fclose(dest_fp);
        pyobj_delete(code_obj);
        list_delete(lexems, lexem_delete);
        return EXIT_FAILURE;
    }

    printf("Succès ! '%s' a été généré à partir de '%s'.\n", output_filename, source_filename);

    
    fclose(dest_fp);
    pyobj_delete(code_obj);
    list_delete(lexems, lexem_delete);

    return EXIT_SUCCESS;
}