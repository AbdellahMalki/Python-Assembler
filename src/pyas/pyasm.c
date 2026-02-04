/**
 * @file pyasm.c
 * @author Abdellah 
 * @brief Python object assembler.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pyas/lnotab.h>
#include <parser/pyobj.h>
#include <lexer/lexem.h>

//labels 
// i create a structure to store labels name+adress (octet)
typedef struct label_s {
    char *name;         
    int addr;           
    struct label_s *next; //pointer to the next (very useful parlasuite)
} label_t;


static void label_add(label_t **table, char *name, int addr) {
    label_t *new = malloc(sizeof(label_t));
    assert(new != NULL);

    new->name = strdup(name);
    
//removz the : 
    size_t len = strlen(new->name);
    if (len > 0 && new->name[len-1] == ':') {
        new->name[len-1] = '\0';
    }

    new->addr = addr;
    new->next = *table;
    *table = new;
}

// find label address with name returns -1 if fail=
static int label_get_addr(label_t *table, char *name) {
    label_t *curr = table;
    while (curr) {
        if (strcmp(curr->name, name) == 0) {
            return curr->addr;
        }
        curr = curr->next;
    }
    return -1; // unknown labl
}


static void label_free(label_t *table) {
    label_t *curr = table;
    while (curr) {
        label_t *next = curr->next;
        free(curr->name);
        free(curr);
        curr = next;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////
//Adress calc
//this will be the first passage 1 : it should return the total size of the bytecode

static int pyasm_pass1(pyobj_t code_obj, label_t **label_table) {
    int current_offset = 0;
    
    //the code section parsed we put it in list lexem
    list_t lexems = code_obj->py._code.instructions;
    
    //traveling through lexems ...... ^-^
    list_t cursor = lexems; 

    while ( !list_is_empty(cursor) ) {
        lexem_t lex = list_first(cursor); // capture the current lexem
        const char *type = lexem_type(lex);
        
        //  case 1 : Labels
        if (strstr(type, "label") != NULL || strchr(lexem_value(lex), ':')) {
            
            label_add(label_table, (char*)lexem_value(lex), current_offset);
        }
        
        // case 2 : Iinsn::0 no argument (1 octet)

        else if (strstr(type, "insn::0") != NULL) {
            current_offset += 1;
        }
        
        // case 3 : insn::1 with args(3 octets)
        // Opcode (1 octet) + Argument (2 octets) = 3 octets
        else if (strstr(type, "insn::1") != NULL) {
            current_offset += 3;
            // consume the argument
            cursor = list_next(cursor); 
        }

        // case 4 : skip directive line and the num  that follows
        else if (strstr(type, "directive::line") != NULL) {
             cursor = list_next(cursor); // skip line numbr
        }
        
        // case 5 : Directives n Commentaires (0 octet)
        
        cursor = list_next(cursor);
    }
    
    return current_offset;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////


//extract the opcode
static int get_opcode_from_type(const char *type) {
    // look for "0x"
    char *hex_str = strstr(type, "0x");
    if (hex_str) {
        return (int)strtol(hex_str, NULL, 16);
    }
    
    return 0; 
}

//if it's a jump
static int is_relative_jump(int opcode) {
    
    switch (opcode) {
        case 0x5d: // FOR_ITER
        case 0x6e: // JUMP_FORWARD
        case 0x78: // SETUP_LOOP
        case 0x79: // SETUP_EXCEPT
        case 0x7a: // SETUP_FINALLY
        case 0x8f: // SETUP_WITH
            return 1;
        default:
            return 0;
    }
}
///////////////////////////////////////////////////////////////////////////////////////////
//this will be 2 passage should generate the bytecode

static void pyasm_pass2(pyobj_t code_obj, label_t *labels, unsigned char *bytecode) {
    int offset = 0;
    list_t cursor = code_obj->py._code.instructions;

    while ( !list_is_empty(cursor) ) {
        lexem_t lex = list_first(cursor);
        const char *type = lexem_type(lex);
        char *val = (char*)lexem_value(lex);

        //  case 1 : Label cintinue already dealt with in pass1
        if (strstr(type, "label") || strchr(val, ':')) {
            cursor = list_next(cursor);
            continue;
        }

        // Ignore directives line here too!
        
        if (strstr(type, "directive::line")) {
            cursor = list_next(cursor); // skip the number token
            cursor = list_next(cursor); 
            continue; 
            // cursor is at .line. list_next moves to number. 
            // Loop bottom does list_next -> moves to next instr. Correct.
        }

        // case 2
        if (strstr(type, "insn::0")) {
            int opcode = get_opcode_from_type(type);
            bytecode[offset] = (unsigned char)opcode;
            offset += 1;
        }
        
        // case 3
        else if (strstr(type, "insn::1")) {
            // write opcode
            int opcode = get_opcode_from_type(type);
            bytecode[offset] = (unsigned char)opcode;
            offset += 1;

            // then the argument
            cursor = list_next(cursor); 
            // verify if there is an argument
            if (list_is_empty(cursor)) {
                fprintf(stderr, "Erreur: Argument manquant pour l'instruction %s\n", val);
                exit(EXIT_FAILURE);
            }
            
            lexem_t arg_lex = list_first(cursor);
            char *arg_val = (char*)lexem_value(arg_lex);
            const char *arg_type = lexem_type(arg_lex);
            
            int arg_value = 0;

            // analyse arg (Entier, Hex or another Label ?)
            
            // if number dec or hex
            if (strstr(arg_type, "number") || (arg_val[0] >= '0' && arg_val[0] <= '9') || arg_val[0] == '-') {
                
                if (strstr(arg_val, "0x")) {
                    arg_value = (int)strtol(arg_val, NULL, 16);
                } else {
                    arg_value = atoi(arg_val);
                }
            }
            // Si c'est un symbole -> C'est un Label (Saut)
            else {
                int target_addr = label_get_addr(labels, arg_val);
                
                if (target_addr == -1) {
                    fprintf(stderr, "Erreur: Label indéfini '%s' (utilisé ligne %d)\n", 
                            arg_val, lexem_line(lex));
                    exit(EXIT_FAILURE);
                }

                // calculate the jump
                if (is_relative_jump(opcode)) {
                    // relative jump : Cible - (Instruction_Courante + 3)
                    // offset pointe actuellement sur l'argument (opcode déjà passé +1), 
                    // donc l'instruction complète finit à offset + 2.
                    arg_value = target_addr - (offset + 2);
                } else {
                    // saut absolute
                    arg_value = target_addr;
                }
            }

            // lil endian writing
            bytecode[offset] = arg_value & 0xFF;        // LSB
            bytecode[offset+1] = (arg_value >> 8) & 0xFF; // MSB
            
            offset += 2;
        }

        // P ext lexem
        cursor = list_next(cursor);
    }
}

//
////////////////////////////////////////////////////////////////////////////
//implementing lnotable

static int pyasm_generate_lnotab(pyobj_t code) {
    // take the number of first line
    int first_line = code->py._code.binary.trailer.firstlineno;
    if (first_line == 0) first_line = 1;

    lnotab_t *table = create_lnotab(first_line);
    if (!table) return -1;

    int current_offset = 0;
    int current_line = first_line;

    list_t cursor = code->py._code.instructions;
    while (!list_is_empty(cursor)) {
        lexem_t lex = list_first(cursor);
        const char *type = lexem_type(lex);

        // when we find a line we need to store the offset (byte) and that line number
        if (strstr(type, "directive::line")) {
            cursor = list_next(cursor); 
            if (!list_is_empty(cursor)) {
                lexem_t num_lex = list_first(cursor);
                int new_line = atoi(lexem_value(num_lex));
                
                lnotab_append(table, new_line, current_offset);
                current_line = new_line;
            }
        }
        
        // we continue incrementing the counter
        else if (strstr(type, "insn::0")) {
            current_offset += 1;
        }
        else if (strstr(type, "insn::1")) {
            current_offset += 3;
            cursor = list_next(cursor); 
        }

        cursor = list_next(cursor);
    }

    //we have a buffer with the lnotab i prefer to put it in the struct of pyobjstring
    pyobj_t lnotab_obj = malloc(sizeof(struct pyobj));
    lnotab_obj->refcount = 1;
    lnotab_obj->type = PYOBJ_STRING;
    
    lnotab_obj->py._string.length = table->lnotab_size;
    lnotab_obj->py._string.buffer = malloc(table->lnotab_size);
    memcpy(lnotab_obj->py._string.buffer, table->buffer, table->lnotab_size);

    //attach to our code
    code->py._code.binary.trailer.lnotab = lnotab_obj;

    free_lnotab(table);
    return 0;
}

////////////////////////////////////////////////////////////////

int pyasm(pyobj_t code) {
    label_t *labels = NULL;

    // first passage label marking and total size
    // printf("[PYASM] -- Passe 1 : Analyse des labels --\n");
    
    int bytecode_size = pyasm_pass1(code, &labels);
    
    if (bytecode_size < 0) {
        return -1; 
    }

    unsigned char *buffer = calloc(bytecode_size, sizeof(unsigned char));
    if (!buffer && bytecode_size > 0) {
        perror("Erreur allocation bytecode");
        return -1;
    }
    // printf("[PYASM] -- Passe 2 : Géneration du Bytecode--\n");
    // second passage bytecode and jumps
    pyasm_pass2(code, labels, buffer);

    //we create a pyobj string type and we allocate manually size (problem liked to 0x00 STOP_code)

    pyobj_t bytecode_pystr = malloc(sizeof(struct pyobj));
    if (!bytecode_pystr) return -1;

    bytecode_pystr->refcount = 1;
    bytecode_pystr->type = PYOBJ_STRING;
    
    bytecode_pystr->py._string.length = bytecode_size; 
    bytecode_pystr->py._string.buffer = malloc(bytecode_size);

    memcpy(bytecode_pystr->py._string.buffer, buffer, bytecode_size);

    
    //we write the bytecode
    code->py._code.binary.content.bytecode = bytecode_pystr;

    //generate the lnotab 
    if (pyasm_generate_lnotab(code) < 0) {
        fprintf(stderr, "[PYASM] Attention: Echec generation lnotab (non fatal)\n");
        
    }


    free(buffer);
    label_free(labels);
    
    printf("[PYASM] Assemblage termine avec succes.\n");
    return 0;
}