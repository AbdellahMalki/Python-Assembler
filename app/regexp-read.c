/**
 * @file regexp-read.c
 * @brief Programme principal pour tester le parser de regex (Incrément 1)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include <regexp/chargroup.h>
#include <regexp/regexp.h>
#include <generic/list.h>

int main(int argc, char *argv[]) {
    
    //aarguments verification
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <regexp_string>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *regexp_str = argv[1];


    //Parser call (Fonction re_read)
    // This function transforms a string into a chargroup
    list_t list = re_read(regexp_str);

    if (list == NULL) {

        fprintf(stderr, "Invalid regexp: Failed to parse regexp (or empty regexp).\n");
        return EXIT_FAILURE;
    }

    // Show results
    // On utilise la fonction générique list_print 

    list_print(list, chargroup_print_cb);
    printf("\n");

    // memory cleaning
    // On détruit la liste aavec callback
    list_delete(list, chargroup_delete_cb);

    return EXIT_SUCCESS;
}