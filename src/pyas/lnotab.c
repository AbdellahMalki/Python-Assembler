/*
 * @file lnotab.h
 * @author LÃ©o 
 * @brief line number table 
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <generic/list.h>
#include <pyas/lnotab.h>

lnotab_t* create_lnotab(int line){  
    lnotab_t *lnotab = malloc(sizeof(lnotab_t)); 
    if (!lnotab) return NULL;

    lnotab->buffer_size = 255; // as everything is a byte, there is (hopefully) no reason for anything to be bigger than 256
    lnotab->lnotab_size = 0;
    lnotab->buffer = malloc(lnotab->buffer_size);
    lnotab->byte_offset = 0;
    lnotab->last_line = line;
    return lnotab;
}

lnotab_t* lnotab_append (lnotab_t* lnotab, int line_number, int bytecode_address){
    if (!lnotab) {
        return NULL;
    }
    
    int delta_offset = bytecode_address - lnotab->byte_offset;
    int delta_line = line_number - lnotab->last_line;
    
    // No changes
    if (delta_offset == 0 && delta_line == 0) {
        return lnotab;
    }
    
    // Deltas > 255 are a problem we ignore in this project (cf p152)
    if (delta_offset >255 || delta_line > 255) {
        printf("deltas are too large, please use object coded on a signle byte");
        return NULL;
    }
    
    // Deltas SHOULD be >= 0 
    if (delta_offset <0 || delta_line <0 ) {
        printf("deltas are negative, can't process further");
        return NULL;
    }
    
    else if (delta_offset > 0 || delta_line > 0) {

        lnotab->buffer[lnotab->lnotab_size] = delta_offset; 
        lnotab->lnotab_size = lnotab->lnotab_size + 1;

        lnotab->buffer[lnotab->lnotab_size] = delta_line; 
        lnotab->lnotab_size = lnotab->lnotab_size + 1;
    }
    
    // updating the values , it's a "differential" implementation
    lnotab->byte_offset = bytecode_address;
    lnotab->last_line = line_number;
    
    return lnotab;
}
void free_lnotab(lnotab_t *lnotab) {
    if (lnotab) {
        free(lnotab->buffer);
        free(lnotab);
    }
}
