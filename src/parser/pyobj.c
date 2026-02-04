/**
 * @file pyobj.c
 * @brief Implémentation des objets Python (pyobj_t)
 *
 * Helper functions to manipulate Python objects (pyobj_t).
 */

#include <assert.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include <lexer/lexem.h>

#include <parser/pyobj.h>


static pyobj_t pyobj_alloc(pyobj_type type) {
	// Allocation + init (calloc met tout à 0)
	pyobj_t obj = calloc(1, sizeof(*obj));
	assert(obj);
	obj->refcount = 1;
	obj->type = type;
	return obj;
}


pyobj_t pyobj_int_new(int32_t value) {
	// Create int32
	pyobj_t obj = pyobj_alloc(PYOBJ_INT);
	obj->py._int = value;
	return obj;
}

pyobj_t pyobj_int64_new(int64_t value) {
	pyobj_t obj = pyobj_alloc(PYOBJ_INT64);
	obj->py._int64 = value;
	return obj;
}

pyobj_t pyobj_float_new(double value) {
	// Create float
	pyobj_t obj = pyobj_alloc(PYOBJ_FLOAT);
	obj->py._float = value;
	return obj;
}

pyobj_t pyobj_string_new(const char *s) {
	// Create string
	//modif now a string is no longer pointer to char it's a new struct with len included (to solve problems with serialiser)
	pyobj_t obj = pyobj_alloc(PYOBJ_STRING);
    
    if (s) {
        size_t len = strlen(s);
        // On remplit les champs de la nouvelle structure
        obj->py._string.length = (int)len;
        obj->py._string.buffer = malloc(len + 1); // +1 pour le \0 de sécurité
        assert(obj->py._string.buffer);
        memcpy(obj->py._string.buffer, s, len + 1);
    } else {
        obj->py._string.length = 0;
        obj->py._string.buffer = malloc(1);
        assert(obj->py._string.buffer);
        obj->py._string.buffer[0] = '\0';
    }
    
    return obj;
}

pyobj_t pyobj_none_new(void) {
	return pyobj_alloc(PYOBJ_NONE);
}

pyobj_t pyobj_true_new(void) {
	return pyobj_alloc(PYOBJ_TRUE);
}

pyobj_t pyobj_false_new(void) {
	return pyobj_alloc(PYOBJ_FALSE);
}


pyobj_t pyobj_list_new(void) {
	// Create empty list
	pyobj_t obj = pyobj_alloc(PYOBJ_LIST);
	obj->py._list = list_new();
	return obj;
}

pyobj_t pyobj_tuple_new_from_list(list_t elements) {
	pyobj_t obj = pyobj_alloc(PYOBJ_TUPLE);
	/* Shallow-clone the list nodes so tuple/list don't share maillons.
	   Elements (pyobj_t) are not duplicated; the tuple owns its elements. */
	if (list_is_empty(elements)) {
		obj->py._list = list_new();
		return obj;
	}

	list_t clone = list_new();
	for ( ; !list_is_empty(elements); elements = list_next(elements) ) {
		void *el = list_first(elements);
		clone = list_add_last(el, clone);
	}

	obj->py._list = clone;
	return obj;
}

int pyobj_list_append(pyobj_t list, pyobj_t item) {
	// Append item to a list/tuple during construction
	if (NULL == list || NULL == item) return -1;
	if (list->type != PYOBJ_LIST && list->type != PYOBJ_TUPLE) return -1;

	list->py._list = list_add_last(item, list->py._list);
	return 0;
}


pyobj_t pyobj_code_new(void) {
	// Create code object
	pyobj_t obj = pyobj_alloc(PYOBJ_CODE);
	obj->py._code.instructions = list_new();

	return obj;
}


static int pyobj_print_list_like(list_t l, const char *open, const char *close) {
	// Helper to print list/tuple
	int nchars = 0;
	nchars += printf("%s", open);

	for (; !list_is_empty(l); l = list_next(l)) {
		pyobj_t item = (pyobj_t)list_first(l);
		nchars += pyobj_print(item);
		if (!list_is_empty(list_next(l))) nchars += printf(", ");
	}

	nchars += printf("%s", close);
	return nchars;
}


void pyobj_list_reverse(pyobj_t list) {
	// Reverse a list/tuple
	if (NULL == list) return;
	if (list->type != PYOBJ_LIST && list->type != PYOBJ_TUPLE) return;

	list->py._list = list_reverse(list->py._list);
}

int pyobj_list_prepend(pyobj_t list, pyobj_t item) {
	// Prepend an empty item to a list/tuple
	if (NULL == list) return -1;
	if (list->type != PYOBJ_LIST && list->type != PYOBJ_TUPLE) return -1;

	list->py._list = list_add_first(item, list->py._list);
	return 0;
}


int pyobj_print(void *_obj) {
	// Print a Python object (debug output)
	pyobj_t obj = (pyobj_t)_obj;

	if (NULL == obj) return printf("<null>");

	switch (obj->type) {
	case PYOBJ_NONE:
		return printf("None");
	case PYOBJ_TRUE:
		return printf("True");
	case PYOBJ_FALSE:
		return printf("False");
	case PYOBJ_INT:
		return printf("%'" PRId32, obj->py._int);
	case PYOBJ_INT64:
		return printf("%'" PRId64, obj->py._int64);
	case PYOBJ_FLOAT:
		return printf("%g", obj->py._float);
	case PYOBJ_STRING:
		return printf("%s", obj->py._string.buffer ? obj->py._string.buffer : ""); //modified aftr string we show the buffer of _string
	case PYOBJ_LIST:
		return pyobj_print_list_like(obj->py._list, "[", "]");
	case PYOBJ_TUPLE:
		return pyobj_print_list_like(obj->py._list, "(", ")");
	case PYOBJ_CODE:
		return printf("<code object>");
	case PYOBJ_NULL:
	default:
		return printf("<pyobj:%d>", obj->type);
	}
}

int pyobj_print_all_recursif(pyobj_t obj){

	if (NULL == obj) {
		return printf("<null>");
	}

	switch (obj->type) {
		case PYOBJ_NONE:
			return printf("None");
		case PYOBJ_TRUE:
			return printf("True");
		case PYOBJ_FALSE:
			return printf("False");
		case PYOBJ_INT:
			return printf("%'" PRId32, obj->py._int);
		case PYOBJ_INT64:
			return printf("%'" PRId64, obj->py._int64);
		case PYOBJ_FLOAT:
			return printf("%g", obj->py._float);
		case PYOBJ_STRING:
			return printf("%s", obj->py._string.buffer ? obj->py._string.buffer : "");
		case PYOBJ_LIST: {
			int nchars = printf("[");
			for (list_t l = obj->py._list; !list_is_empty(l); l = list_next(l)) {
				pyobj_t item = (pyobj_t)list_first(l);
				nchars += pyobj_print_all_recursif(item);
				if (!list_is_empty(list_next(l))) {
					nchars += printf(", ");
				}
			}
			nchars += printf("]");
			return nchars;
		}
		case PYOBJ_TUPLE: {
			int nchars = printf("(");
			for (list_t l = obj->py._list; !list_is_empty(l); l = list_next(l)) {
				pyobj_t item = (pyobj_t)list_first(l);
				nchars += pyobj_print_all_recursif(item);
				if (!list_is_empty(list_next(l))) {
					nchars += printf(", ");
				}
			}
			nchars += printf(")");
			return nchars;
		}
		case PYOBJ_CODE:
			//Write the code object details
			printf("Code Object:\n");
			printf(" |- Arg Count: %u\n", obj->py._code.header.arg_count);
			printf(" |- Local Count: %u\n", obj->py._code.header.local_count);
			printf(" |- Stack Size: %u\n", obj->py._code.header.stack_size);
			printf(" |- Flags: %u\n", obj->py._code.header.flags);

			//parent
			printf(" |- Parent: ");
			pyobj_print_all_recursif(obj->py._code.parent);
			printf("\n");

			//Header
			printf(" |- Binary Header:\n");
			printf(" |    |- Version PyVM: %u\n", obj->py._code.binary.header.version_pyvm);
			printf(" |    |- Magic: %u\n", obj->py._code.binary.header.magic);
			printf(" |    |- Source Size: %u\n", obj->py._code.binary.header.source_size);

			//in py._code.binary.content print all the content
			printf(" |- Binary Content:\n");
			printf(" |    |- Interned: ");
			pyobj_print_all_recursif(obj->py._code.binary.content.interned);
			printf("\n |    |- Bytecode: ");
			pyobj_print_all_recursif(obj->py._code.binary.content.bytecode);
			printf("\n |    |- Consts: ");
			pyobj_print_all_recursif(obj->py._code.binary.content.consts);

			printf("\n |    |- Names: ");
			pyobj_print_all_recursif(obj->py._code.binary.content.names);
			printf("\n |    |- Varnames: ");
			pyobj_print_all_recursif(obj->py._code.binary.content.varnames);
			printf("\n |    |- Freevars: ");
			pyobj_print_all_recursif(obj->py._code.binary.content.freevars);
			printf("\n |    |- Cellvars: ");
			pyobj_print_all_recursif(obj->py._code.binary.content.cellvars);
			printf("\n");

			//Print trailer
			printf(" |- Trailer:\n");
			printf(" |    |- Filename: ");
			pyobj_print_all_recursif(obj->py._code.binary.trailer.filename);
			printf("\n |    |- Name: ");
			pyobj_print_all_recursif(obj->py._code.binary.trailer.name);
			printf("\n |    |- First Line No: %u\n", obj->py._code.binary.trailer.firstlineno);
			printf(" |    |- Lnotab: ");
			pyobj_print_all_recursif(obj->py._code.binary.trailer.lnotab);
			printf("\n");

			//Print instructions
			printf(" |- Instructions:\n");
			for (list_t l = obj->py._code.instructions; !list_is_empty(l); l = list_next(l)) {
				lexem_t instr = (lexem_t)list_first(l);
				printf(" |    |- ");lexem_print(instr);
				printf("\n");
			}

			return 0;

		case PYOBJ_NULL:
		default:
			return printf("<pyobj:%d>", obj->type);
	}
}

int pyobj_delete(void *_obj) {
	// memory cleaning (récursif pour list/tuple/code)
	pyobj_t obj = (pyobj_t)_obj;
	if (NULL == obj) return 0;

	switch (obj->type) {
	case PYOBJ_STRING:
		if (obj->py._string.buffer) {
            free(obj->py._string.buffer);
        }
        break;

	case PYOBJ_LIST:
		list_delete(obj->py._list, pyobj_delete);
		break;
		
	case PYOBJ_TUPLE:
		list_delete(obj->py._list, pyobj_delete);
		break;

	case PYOBJ_CODE:
		pyobj_delete(obj->py._code.binary.content.interned);
		pyobj_delete(obj->py._code.binary.content.bytecode);
		pyobj_delete(obj->py._code.binary.content.consts);
		pyobj_delete(obj->py._code.binary.content.names);
		pyobj_delete(obj->py._code.binary.content.varnames);
		pyobj_delete(obj->py._code.binary.content.freevars);
		pyobj_delete(obj->py._code.binary.content.cellvars);

		pyobj_delete(obj->py._code.binary.trailer.filename);
		pyobj_delete(obj->py._code.binary.trailer.name);
		pyobj_delete(obj->py._code.binary.trailer.lnotab);

		list_delete(obj->py._code.instructions, lexem_delete);
		break;

	case PYOBJ_NONE:
	case PYOBJ_TRUE:
	case PYOBJ_FALSE:
	case PYOBJ_INT:
	case PYOBJ_INT64:
	case PYOBJ_FLOAT:
	case PYOBJ_NULL:
	default:
		break;
	}

	free(obj);
	return 0;
}
