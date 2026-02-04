/**
 * @file serialiser.c
 * @author Abdellah
 * @brief Serialisation of python objects to binary format .pyc
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include <parser/pyobj.h>
#include <lexer/lexem.h> 
#include <generic/list.h> 



static int write_byte(FILE *fp, int val) {
    if (fputc(val & 0xFF, fp) == EOF) return -1;
    return 0;
}

static int write_int32(FILE *fp, int32_t val) {
    if (fputc(val & 0xFF, fp) == EOF) return -1;
    if (fputc((val >> 8) & 0xFF, fp) == EOF) return -1;
    if (fputc((val >> 16) & 0xFF, fp) == EOF) return -1;
    if (fputc((val >> 24) & 0xFF, fp) == EOF) return -1;
    return 0;
}

static int write_int64(FILE *fp, int64_t val) {
    uint32_t low = (uint32_t)(val & 0xFFFFFFFF);
    uint32_t high = (uint32_t)((val >> 32) & 0xFFFFFFFF);
    if (write_int32(fp, (int32_t)low) < 0) return -1;
    if (write_int32(fp, (int32_t)high) < 0) return -1;
    return 0;
}

static int write_bytes_string(FILE *fp, char *str, int size) {
    if (write_int32(fp, size) < 0) return -1;
    if (size > 0 && str != NULL) {
        if (fwrite(str, 1, size, fp) != (size_t)size) return -1;
    }
    return 0;
}

// Forward declaration
int pyobj_write(FILE *fp, pyobj_t obj); 

//more helpers written after debugging

static int write_string_or_empty(FILE *fp, pyobj_t obj) {
    if (obj == NULL || obj->type != PYOBJ_STRING) {
        if (write_byte(fp, 's') < 0) return -1;
        if (write_int32(fp, 0) < 0) return -1;
        return 0;
    }
    
    if (write_byte(fp, 's') < 0) return -1;
    return write_bytes_string(fp, obj->py._string.buffer, obj->py._string.length);
}


static int write_as_tuple(FILE *fp, pyobj_t obj) {
    
    if (obj == NULL) {
        if (write_byte(fp, '(') < 0) return -1;
        if (write_int32(fp, 0) < 0) return -1;
        return 0;
    }

    // if it's a list or tuple we write it as a tuple 
    if (obj->type == PYOBJ_LIST || obj->type == PYOBJ_TUPLE) {
        //so we force the ()
        if (write_byte(fp, '(') < 0) return -1;

        list_t l = obj->py._list;
        int size = list_length(l);
        if (write_int32(fp, size) < 0) return -1;

        list_t cursor = l;
        while (!list_is_empty(cursor)) {
            
            if (pyobj_write(fp, list_first(cursor)) < 0) return -1;
            cursor = list_next(cursor);
        }
        return 0;
    }
    
    //if none we write empty tuple
    if (write_byte(fp, '(') < 0) return -1;
    if (write_int32(fp, 0) < 0) return -1;
    return 0;
}



int pyobj_write(FILE *fp, pyobj_t obj) {
    if (!fp) return -1;
    if (!obj) return write_byte(fp, 'N');

    switch (obj->type) {
        case PYOBJ_NULL: return 0;

        case PYOBJ_NONE:  return write_byte(fp, 'N');
        case PYOBJ_FALSE: return write_byte(fp, 'F');
        case PYOBJ_TRUE:  return write_byte(fp, 'T');

        case PYOBJ_INT:
            if (write_byte(fp, 'i') < 0) return -1;
            return write_int32(fp, obj->py._int);

        case PYOBJ_INT64:
            if (write_byte(fp, 'I') < 0) return -1; 
            return write_int64(fp, obj->py._int64);

        case PYOBJ_FLOAT:
            if (write_byte(fp, 'g') < 0) return -1;
            if (fwrite(&(obj->py._float), sizeof(double), 1, fp) != 1) return -1;
            break;

        case PYOBJ_STRING:
            if (write_byte(fp, 's') < 0) return -1;
            return write_bytes_string(fp, obj->py._string.buffer, obj->py._string.length);

        case PYOBJ_LIST:
        case PYOBJ_TUPLE: {
            char marker = (obj->type == PYOBJ_LIST) ? '[' : '(';
            if (write_byte(fp, marker) < 0) return -1;
            
            list_t l = obj->py._list;
            int size = list_length(l);
            if (write_int32(fp, size) < 0) return -1;

            list_t cursor = l;
            while ( !list_is_empty(cursor) ) {
                if (pyobj_write(fp, list_first(cursor)) < 0) return -1;
                cursor = list_next(cursor);
            }
            break;
        }

        case PYOBJ_CODE:
            if (write_byte(fp, 'c') < 0) return -1;

            // header
            if (write_int32(fp, obj->py._code.header.arg_count) < 0) return -1;
            if (write_int32(fp, obj->py._code.header.local_count) < 0) return -1;
            if (write_int32(fp, obj->py._code.header.stack_size) < 0) return -1;
            if (write_int32(fp, obj->py._code.header.flags) < 0) return -1;

            // content 
            // Bytecode -> forced String with the defined struct 
            if (write_string_or_empty(fp, obj->py._code.binary.content.bytecode) < 0) return -1;
            
            // Tables -> Tuples
            if (write_as_tuple(fp, obj->py._code.binary.content.consts) < 0) return -1;
            if (write_as_tuple(fp, obj->py._code.binary.content.names) < 0) return -1;
            if (write_as_tuple(fp, obj->py._code.binary.content.varnames) < 0) return -1;
            if (write_as_tuple(fp, obj->py._code.binary.content.freevars) < 0) return -1;
            if (write_as_tuple(fp, obj->py._code.binary.content.cellvars) < 0) return -1;
            
           
            if (write_string_or_empty(fp, obj->py._code.binary.trailer.filename) < 0) return -1;
            if (write_string_or_empty(fp, obj->py._code.binary.trailer.name) < 0) return -1;
            
            if (write_int32(fp, obj->py._code.binary.trailer.firstlineno) < 0) return -1;
            
            // Lnotab -> forced String
            if (write_string_or_empty(fp, obj->py._code.binary.trailer.lnotab) < 0) return -1;
            break;

        default:
            return -1;
    }

    return 0;
}