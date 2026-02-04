/**
 * @file pyasm.c
 * @author Abdellah & Ninon
 * @brief Parse lexems into python objects
 *
 */

#include "generic/list.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <parser/parser.h>
#include <parser/lexem_helpers.h>
#include <parser/pyobj.h> 
#include <lexer/lexem.h> 
#include <lexer/lexer.h>

static pyobj_t parse_constant(list_t *lexems);
static lexem_t lexem_clone(lexem_t lex) {
    if (NULL == lex) return NULL;
    return lexem_new((char *)lexem_type(lex), (char *)lexem_value(lex), lexem_line(lex), lexem_column(lex));
}

//a pys code may start with useless structure::blanks or random newlines
static void skip_eol(list_t *lexems) {
    while (next_lexem_is(lexems, "structure::newline") || next_lexem_is(lexems, "structure::blank") || next_lexem_is(lexems, "structure::comment")) {
        lexem_advance(lexems);
    }
}



static pyobj_t parse_collection(list_t *lexems, char *end_type, int is_list) {
    // stock list/tuple elements
    // CORRECTION: create_list -> list_new
    pyobj_t collection = is_list ? pyobj_list_new() : pyobj_list_new(); 
    
    lexem_advance(lexems); 

    // the end_type is th ) or ]
    while (!next_lexem_is(lexems, end_type)) {
        
        //ignore blanks between elements
        if (next_lexem_is(lexems, "structure::blank") || next_lexem_is(lexems, "structure::newline")) {
            lexem_advance(lexems);
            continue;
        }

        pyobj_t element = parse_constant(lexems);
        if (!element) {
            pyobj_delete(collection);
            return NULL; 
        }

        // Once inserted, the list owns the element.
        if (0 != pyobj_list_prepend(collection, element)) {
            pyobj_delete(element);
            pyobj_delete(collection);
            return NULL;
        }
    }

    // closing bracket
    lexem_advance(lexems);
	pyobj_list_reverse(collection);
    return collection;
}




static pyobj_t parse_constant(list_t *lexems) {
    
    lexem_t lex = lexem_peek(lexems);

    if (next_lexem_is(lexems, "number::int")) {
        lexem_advance(lexems);
        return pyobj_int_new(atoi(lexem_value(lex)));
    }
    else if (next_lexem_is(lexems, "number::uint")) {
        lexem_advance(lexems);
        return pyobj_int_new((int32_t)strtol(lexem_value(lex), NULL, 10));
    }
    else if (next_lexem_is(lexems, "number::hex")) {
        lexem_advance(lexems);
        return pyobj_int_new((int)strtol(lexem_value(lex), NULL, 16));
    }
    else if (next_lexem_is(lexems, "number::oct")) {
        lexem_advance(lexems);
        const char *s = lexem_value(lex);
        if (s && 0 == strncmp(s, "0o", 2)) s += 2;
        return pyobj_int_new((int32_t)strtol(s ? s : "0", NULL, 8));
    }
    else if (next_lexem_is(lexems, "number::bin")) {
        lexem_advance(lexems);
        const char *s = lexem_value(lex);
        if (s && 0 == strncmp(s, "0b", 2)) s += 2;
        return pyobj_int_new((int32_t)strtol(s ? s : "0", NULL, 2));
    }
    else if (next_lexem_is(lexems, "number::float")) {
        lexem_advance(lexems);
    
        return pyobj_float_new(atof(lexem_value(lex)));
    }
    else if (next_lexem_is(lexems, "number::floatexp")) {
        lexem_advance(lexems);
        return pyobj_float_new(atof(lexem_value(lex)));
    }
    else if (next_lexem_is(lexems, "string::*") || next_lexem_is(lexems, "String")){
        lexem_advance(lexems);
        // PS we might need to remove the "" ?
        return pyobj_string_new(lexem_value(lex)); 
    }
    else if (next_lexem_is(lexems, "pycst::None")) {
        lexem_advance(lexems);
        
        return pyobj_none_new();
    }
    else if (next_lexem_is(lexems, "pycst::True")) {
        lexem_advance(lexems);
        
        return pyobj_true_new();
    }
    else if (next_lexem_is(lexems, "pycst::False")) {
        lexem_advance(lexems);
        
        return pyobj_false_new();
    }
    
    else if (next_lexem_is(lexems, "bracket::left")) {
        return parse_collection(lexems, "bracket::right", 1); // 1 = Liste
    }
    else if (next_lexem_is(lexems, "paren::left")) {
        return parse_collection(lexems, "paren::right", 0); // 0 = Tuple
    }

    print_parse_error("Expected constant", lexems);
    return NULL;
}



// structure parser for .set KEYWORD VALUE
// Parse ALL consecutive .set directives and validate them (unknown keys, duplicates, missing keys).
// Returns 0 on success, -1 on error.
static int parse_set_directive(list_t *lexems, pyobj_t code) {
    int seen_version_pyvm = 0;
    int seen_flags = 0;
    int seen_filename = 0;
    int seen_name = 0;
    int seen_stack_size = 0;
    int seen_arg_count = 0;

    while (next_lexem_is(lexems, "directive::set")) {
        // Verify .set
        lexem_advance(lexems);

        if (!next_lexem_is(lexems, "structure::blank")) {
            print_parse_error("Expected space after .set", lexems);
            return -1;
        }
        lexem_advance(lexems);

        // KEYWORD eg version_pyvm ..
        if (!next_lexem_is(lexems, "identifier::symbol")) {
            print_parse_error("Expected identifier after .set", lexems);
            return -1;
        }
        lexem_t key_lex = lexem_peek(lexems);
        const char *key = lexem_value(key_lex);
        lexem_advance(lexems);

        if (!next_lexem_is(lexems, "structure::blank")) {
            print_parse_error("Expected space before the value", lexems);
            return -1;
        }
        lexem_advance(lexems);

        // read the value
        lexem_t val_lex = lexem_peek(lexems);
        if (NULL == val_lex) {
            print_parse_error("Expected value after .set", lexems);
            return -1;
        }
    
        if (key && 0 == strcmp(key, "version_pyvm")) {
            if (seen_version_pyvm) {
                print_parse_error("Duplicate .set directive", lexems);
                return -1;
            }
            if (!next_lexem_is(lexems, "number::uint")) {
                print_parse_error("Expected decimal integer", lexems);
                return -1;
            }
            code->py._code.binary.header.version_pyvm = (uint32_t)atoi(lexem_value(val_lex));
            seen_version_pyvm = 1;
        }

        else if (key && 0 == strcmp(key, "flags")) {
            if (seen_flags) {
                print_parse_error("Duplicate .set directive", lexems);
                return -1;
            }
            if (!next_lexem_is(lexems, "number::hex")) {
                print_parse_error("Expected hexadecimal integer", lexems);
                return -1;
            }
            code->py._code.header.flags = (uint32_t)strtol(lexem_value(val_lex), NULL, 16);
            seen_flags = 1;
        }

        else if (key && 0 == strcmp(key, "filename")) {
            if (seen_filename) {
                print_parse_error("Duplicate .set directive", lexems);
                return -1;
            }
            if (!next_lexem_is(lexems, "string::double")) {
                print_parse_error("Expected string", lexems);
                return -1;
            }
            code->py._code.binary.trailer.filename = pyobj_string_new(lexem_value(val_lex));
            seen_filename = 1;
        }

        else if (key && 0 == strcmp(key, "name")) {
            if (seen_name) {
                print_parse_error("Duplicate .set directive", lexems);
                return -1;
            }
            if (!next_lexem_is(lexems, "string::double")) {
                print_parse_error("Expected string", lexems);
                return -1;
            }
            code->py._code.binary.trailer.name = pyobj_string_new(lexem_value(val_lex));
            seen_name = 1;
        }

        else if (key && 0 == strcmp(key, "stack_size")) {
            if (seen_stack_size) {
                print_parse_error("Duplicate .set directive", lexems);
                return -1;
            }
            if (!next_lexem_is(lexems, "number::uint")) {
                print_parse_error("Expected decimal integer", lexems);
                return -1;
            }
            code->py._code.header.stack_size = (uint32_t)atoi(lexem_value(val_lex));
            seen_stack_size = 1;
        }

        else if (key && 0 == strcmp(key, "arg_count")) {
            if (seen_arg_count) {
                print_parse_error("Duplicate .set directive", lexems);
                return -1;
            }
            if (!next_lexem_is(lexems, "number::uint")) {
                print_parse_error("Expected decimal integer", lexems);
                return -1;
            }
            code->py._code.header.arg_count = (uint32_t)atoi(lexem_value(val_lex));
            seen_arg_count = 1;
        }

        else {
            print_parse_error("Unknown .set key", lexems);
            return -1;
        }
    ///////////////////////////////////////////////////////////////////////////////////////////////////

        lexem_advance(lexems);
        // consume until end-of-line (if there are trailing blanks/comments)
        while (lexem_peek(lexems) != NULL && !next_lexem_is(lexems, "structure::newline")) {
            lexem_advance(lexems);
        }
        skip_eol(lexems);
    }

    // Checking that all flags have been seen
    if (!seen_version_pyvm || !seen_flags || !seen_filename || !seen_name || !seen_stack_size || !seen_arg_count) {
        fprintf(stderr, "[PARSER] Missing .set directive: ");
        if (!seen_version_pyvm) fprintf(stderr, "version_pyvm ");
        if (!seen_flags) fprintf(stderr, "flags ");
        if (!seen_filename) fprintf(stderr, "filename ");
        if (!seen_name) fprintf(stderr, "name ");
        if (!seen_stack_size) fprintf(stderr, "stack_size ");
        if (!seen_arg_count) fprintf(stderr, "arg_count ");
        fprintf(stderr, "\n");
        return -1;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// .const or .names parser

// mode : 0 for names  1 consts
//-1 fail 0 success
static int parse_table(list_t *lexems, pyobj_t *target_list, char *directive, int mode) {
    
    if (!next_lexem_is(lexems, directive)) return 0; 
    lexem_advance(lexems);
    
    // end of line verif
    if (!next_lexem_is(lexems, "structure::newline")) {
        print_parse_error("Expected newline after table directive", lexems);
        return -1;
    }
    skip_eol(lexems);

    *target_list = pyobj_list_new(); 

    // loop till we hit a new directive
    while (lexem_peek(lexems) != NULL && !next_lexem_is(lexems, "directive::*")) {

        if (next_lexem_is(lexems, "structure::blank") ||
            next_lexem_is(lexems, "structure::comment") ||
            next_lexem_is(lexems, "structure::newline")) {
            lexem_advance(lexems);
            continue;
        }

        pyobj_t item = NULL;
        if (mode == 1) {
            // .consts)
            item = parse_constant(lexems);
        } else {
            // Mode .names, .varnames... just strings
            if (next_lexem_is(lexems, "string::double")) { //lil question here is string enough or should i type string::double ..
                
                 item = pyobj_string_new(lexem_value(lexem_peek(lexems)));
                lexem_advance(lexems);
            } else {
                 print_parse_error("Expected chain in the table", lexems);
                 return -1;
            }
        }

        if (!item) {
            pyobj_delete(*target_list);
            return -1;
        }
        // Once inserted, the list owns the item.
        if (0 != pyobj_list_prepend(*target_list, item)) {
            pyobj_delete(item);
            pyobj_delete(*target_list);
            return -1;
        }

        skip_eol(lexems);
    }
    
	pyobj_list_reverse(*target_list);
    return 0;
}

// use the previous function in order
static int parse_all_tables(list_t *lexems, pyobj_t code) {
    
    
    // .interned (String simples)
    if (parse_table(lexems, &code->py._code.binary.content.interned, "directive::interned", 0) == -1) return -1;
    
    // .varnames (String simples)
    if (parse_table(lexems, &code->py._code.binary.content.varnames, "directive::varnames", 0) == -1) return -1; //to add to .lex
    
    // .freevars & .cellvars (Optionnels, strings simples)
    if (parse_table(lexems, &code->py._code.binary.content.freevars, "directive::freevars", 0) == -1) return -1;
    if (parse_table(lexems, &code->py._code.binary.content.cellvars, "directive::cellvars", 0) == -1) return -1;

    // .consts (Constantes complexes !)
    if (parse_table(lexems, &code->py._code.binary.content.consts, "directive::consts", 1) == -1) return -1;

    // .names (String simples)
    if (parse_table(lexems, &code->py._code.binary.content.names, "directive::names", 0) == -1) return -1;

    return 0;
}



/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//parser for the .text
/*static int parse_code_section(list_t *lexems, pyobj_t code) {
    if (!next_lexem_is(lexems, "directive::text")) {
        print_parse_error("Section .text manquante", lexems);
        return -1;
    }
    lexem_advance(lexems);
    skip_eol(lexems);

    code->py._code.instructions = list_new();

    // loop till end of file
    while (lexem_peek(lexems) != NULL) {
        lexem_t lex = lexem_peek(lexems);
        //not sure of order here

        if (next_lexem_is(lexems, "identifier::symbol") || 
            next_lexem_is(lexems, "colon")   || 
            next_lexem_is(lexems, "number::*") ||
            next_lexem_is(lexems, "directive::line")) {

            lexem_t owned = lexem_clone(lex);
            code->py._code.instructions = list_add_last(owned, code->py._code.instructions);
            lexem_advance(lexems);
        }
        else if (next_lexem_is(lexems, "structure::newline") || next_lexem_is(lexems, "structure::blank") || next_lexem_is(lexems, "structure::comment")) {
            lexem_advance(lexems); 
        }
        else {
             print_parse_error("Token inattendu dans .text", lexems);
             return -1;
        }
    }
    return 0;
}*/
//parse_code_section v2 adapted for pyasm

//----------------------------------------------------------------------------------------------------
static int parse_code_section(list_t *lexems, pyobj_t code) {
    if (!next_lexem_is(lexems, "directive::text")) {
        print_parse_error("Section .text manquante", lexems);
        return -1;
    }
    lexem_advance(lexems);
    skip_eol(lexems);

    code->py._code.instructions = list_new();

    list_t list_labels_defined = list_new();
    list_t list_labels_used = list_new();

    while (lexem_peek(lexems) != NULL) {
        
        // skip nl blnks cmnts 
        if (next_lexem_is(lexems, "structure::newline") || 
            next_lexem_is(lexems, "structure::blank") || 
            next_lexem_is(lexems, "structure::comment")) {
            lexem_advance(lexems); 
            continue;
        }
        
        lexem_t lex = lexem_peek(lexems);
        char *lex_type = (char*)lexem_type(lex);


        //we keep the line directives
        if (next_lexem_is(lexems, "directive::line")) {
            
            code->py._code.instructions = list_add_last(lexem_clone(lex), code->py._code.instructions);
            lexem_advance(lexems);
            
            
            while(next_lexem_is(lexems, "structure::blank")) lexem_advance(lexems);

            // store line number
            if (next_lexem_is(lexems, "number::*")) {
                lexem_t num_lex = lexem_peek(lexems);
                code->py._code.instructions = list_add_last(lexem_clone(num_lex), code->py._code.instructions);
                lexem_advance(lexems);
            }
            continue;
        }

        // identifier symbols
        if (next_lexem_is(lexems, "identifier::symbol")) {
            code->py._code.instructions = list_add_last(lexem_clone(lex), code->py._code.instructions);

            lexem_advance(lexems);
            continue;
        }

        //instructions
        
        //printf("\n\nLEX TYPE 1: %s\n", lex_type);

        if (strstr(lex_type, "insn::")) {
            // we add the opcode
            code->py._code.instructions = list_add_last(lexem_clone(lex), code->py._code.instructions);
            lexem_advance(lexems);

            // if insn::1, we look for argument
            if (strstr(lex_type, "insn::1")) {
                while(next_lexem_is(lexems, "structure::blank")) lexem_advance(lexems);
                
                lexem_t arg = lexem_peek(lexems);
                if(next_lexem_is(lexems, "identifier::symbol")){
                    code->py._code.instructions = list_add_last(lexem_clone(arg), code->py._code.instructions);
                    
                    if (strstr(lexem_value(arg), "label")) {
                        list_labels_used = list_add_first(lexem_clone(arg), list_labels_used);
                    }
                    lexem_advance(lexems);
                } else if (arg && (next_lexem_is(lexems, "number::*") || 
                            next_lexem_is(lexems, "string::*"))) {
                    code->py._code.instructions = list_add_last(lexem_clone(arg), code->py._code.instructions);
                    lexem_advance(lexems);
                } else {
                    print_parse_error("Missing argument for instruction", lexems);
                    list_delete(list_labels_defined, lexem_delete);
                    list_delete(list_labels_used, lexem_delete);
                    return -1;
                }
            }
            continue;
        }

        //Labels management
        if (next_lexem_is(lexems, "identifier::label")) {
            lexem_t label_lex = lexem_new("identifier::label", (char*)lexem_value(lex), 
                                          lexem_line(lex), lexem_column(lex));
            code->py._code.instructions = list_add_last(label_lex, code->py._code.instructions);
            list_labels_defined = list_add_first(lexem_clone(label_lex), list_labels_defined);
            lexem_advance(lexems);
            continue;
        }

        // error if we find sth else
        print_parse_error("Unexpected token in .text", lexems);
        list_delete(list_labels_defined, lexem_delete);
        list_delete(list_labels_used, lexem_delete);
        return -1;
    }

    //Check labels used vs defined
    list_t tmp = list_labels_used;
    while (tmp != NULL) {
        lexem_t used = (lexem_t)(list_first(tmp));
        int found = 0;
        list_t tmp2 = list_labels_defined;
        while (tmp2 != NULL) {
            lexem_t defined = (lexem_t)(list_first(tmp2));
            const char *u = lexem_value(used);
            const char *d = lexem_value(defined);
            if (u && d) {
            size_t dlen = strlen(d);
            if (dlen > 0 && d[dlen - 1] == ':') {
                if (strlen(u) == dlen - 1 && 0 == strncmp(u, d, dlen - 1)) {
                found = 1;
                break;
                }
            } else {
                if (0 == strcmp(u, d)) {
                found = 1;
                break;
                }
            }
            }
            tmp2 = list_next(tmp2);
        }
        if (!found) {
            fprintf(stderr, "[PARSER] Label never defined but used: %s (row %d)\n", lexem_value(used), lexem_line(used));

            //Free lists
            list_delete(list_labels_defined, lexem_delete);
            list_delete(list_labels_used, lexem_delete);
            return -1;
        }
        tmp = list_next(tmp);
    }
    //cleanup
        list_delete(list_labels_defined, lexem_delete);
        list_delete(list_labels_used, lexem_delete);

    return 0;
}
// // // // // // // /*--/-*-*-**-*/-----**-*--/-*--*-*/-//--*//*--*-*-*---*-*-*

pyobj_t parse_program(list_t *lexems) {
    //new empty object
    pyobj_t code = pyobj_code_new();
    if (!code) return NULL;

    //skip line in begining
    skip_eol(lexems);

    //Directives .set
    if (parse_set_directive(lexems, code) == -1) {
        pyobj_delete(code); 
        return NULL;
    }

    //Tables
    if (parse_all_tables(lexems, code) == -1) {
         pyobj_delete(code);
        return NULL;
    }

    //Code section
    if (parse_code_section(lexems, code) == -1) {
        pyobj_delete(code);
        return NULL;
    }

    // Succès !
    return code;
}

/* Backward-compatible alias (not in public header). */
pyobj_t parse_pys(list_t *lexems) {
    return parse_program(lexems);
}