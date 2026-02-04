/**
 * @file regexp.c
 * @author Ninon Mouhat - Abdellah Malki 
 * @brief chargroup
 *
 * chargroup code, as in the project's pdf document
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <regexp/chargroup.h>


chargroup_t chargroup_new() {
    chargroup_t cg = malloc(sizeof(struct chargroup));
    if (cg == NULL)
        return NULL;

    for (int i = 0; i < 128; i++) {
        cg->set[i] = 0;
    }

    cg->has_star_operator = 0;
    cg->has_plus_operator = 0; // <--- AJOUT T2.2
    cg->has_qmark_operator = 0; // <--- AJOUT T2.2
    cg->is_negated = 0; // <--- INITIALISATION T4.2

    return cg;
}



void chargroup_delete(chargroup_t cg) {
    if (cg == NULL)
        return;

    free(cg);
}
//T3.2 On ajoute une fontion dÃ©diÃ©e seulement Ã  l'affichage des Ã©lÃ©ments echappÃ©s

static void print_escaped_char(int i, int inside_brackets) { //static car fonction interne pas besoin de l'ajouter au chargroup.h
    if (inside_brackets) {
        // Dans les crochets [], on Ã©chappe ] et \ et -
        if (i == ']' || i == '\\' || i == '-') printf("\\");
        if (i== 127 || i < 33){
            printf("â‚¬");
            return;
    }}
    // hors crochets on met notre bout de code dÃ©ja fait
    else {
            if (i== 127 || i < 33){
            printf("â‚¬");
            return;
    }
            if (i == 40 || i== 41 || i== 42 || i== 43 || i== 45 || i== 46 || i== 63 || i== 91 || i== 92 || i== 93 || i== 94 || i== 124) { 
            printf("\\");
            }
    }
    printf("%c", i); //on affiche toujours notre char (ex : on a +, on affiche /et+)
}

int chargroup_print_as_regular_expressions(chargroup_t cg) {
    if (cg == NULL)
        return 0;

    int to_print = 0;
    int count =0;
    
    //On ajoute un compteur de caractÃ¨res actifs 
    for (int i = 0; i < 128; i++) {
        if (cg->set[i]) count++;
    }
    //Test negation
    // Si la nÃ©gation est active, on affiche le ^ AVANT tout le reste
    if (cg->is_negated) {
        to_print += printf("^");
    }
    // Test if caracter is '.'
    if (count == 128) { 
    // T3.2 modification du code prÃ©cedent : on verifi*e si c'est un point si count=128 
        to_print += printf(".");
        to_print += 127;
    } 
    
    // Cas vide 
    else if (count == 0) {
        // Rien Ã  afficher
    }

    // Cas caractÃ¨re unique (ex: 'a') -> Pas de crochets
    else if (count == 1) {
        for (int i = 0; i < 128; i++) {
            if (cg->set[i]) {
                print_escaped_char(i, 0); // 0 = pas dans des crochets
                to_print++; 
                break;
            }
        }
    }

    // Cas Groupe (ex: [a-z] ou [abc]) -> Avec crochets
    else {
        to_print += printf("[");
        
        // DÃ©tection de range
        int i = 0;
        while (i < 128) {
            if (cg->set[i]) {
                int start = i;
                int end = i;

                // Chercher la fin de la sÃ©quence on avance jusqu'Ã  ce que le caractere n'est plus dans set
                while (end < 127 && cg->set[end + 1]) {
                    end++;
                }

                // Affichage intelligent (on ajoute - automatiquement si c'est une plage)
                if (end - start >= 2) { 
                    // Si on a au moins 3 caractÃ¨res de suite (ex: a,b,c -> a-c)
                    print_escaped_char(start, 1);
                    to_print += printf("-");
                    print_escaped_char(end, 1);
                    i = end + 1;
                } else {
                    // Sinon on affiche juste le caractÃ¨re courant
                    print_escaped_char(i, 1);
                    i++;
                }
            } else {
                i++;
            }
        }
        to_print += printf("]");
    }

    // Add star if needed
    if (cg->has_star_operator)
        to_print += printf("*");

    // Affichage des autres opÃ©rateurs
    else if (cg->has_plus_operator)
        to_print += printf("+");
    else if (cg->has_qmark_operator)
        to_print += printf("?");


    return to_print;
}

int chargroup_print_as_regular_expressions_cb(void *cg) {
    //Using the chargroup_print function makes it easier
    return chargroup_print_as_regular_expressions((chargroup_t)cg);
}

/* =========================================================================
   VERSION ALTERNATIVE "TUTORAT" (Verbeuse)
   ON decommente cette fonction et on commente la prÃ©cÃ©dente si nÃ©cessaire.
   =========================================================================
*/
/* REMPLACEMENT POUR AFFICHAGE VERBEUX (COMME TUTORAT) */

// Helper modifiÃ© pour l'affichage simple des caractÃ¨res dans les guillemets
static void print_verbose_char(int i) {
    
    if (i == 127 || i < 33) {
        printf("â‚¬");
        return;
    }
    // On Ã©chappe juste les guillemets et le backslash pour ne pas casser l'affichage
    if (i == '"' || i == '\\') {
        printf("\\");
    }
    printf("%c", i);
}

int chargroup_print(chargroup_t cg) {
    if (cg == NULL) return 0;

    int count = 0; // Compteur de caractÃ¨res (pour le retour)

    // NEGATION
    // Affiche "One NOT in" si c'est ^ ou "One in" si c'est [...]
    if (cg->is_negated) {
        printf("One NOT in \"");
    } else {
        printf("One in \"");
    }

    // On liste TOUS les caractÃ¨res un par un
    // SANS PLAGES !!
    for (int i = 0; i < 128; i++) {
        if (cg->set[i]) {
            print_verbose_char(i);
            count++;
        }
    }

    // Fermeture des guillemets de la liste
    printf("\"");

    // SUFFIXE : Traduction des opÃ©rateurs en texte
    if (cg->has_star_operator) {
        printf(", zero or more times");  // Correspond Ã  *
    } 
    else if (cg->has_plus_operator) {
        printf(", one or more times");   // Correspond Ã  +
    } 
    else if (cg->has_qmark_operator) {
        printf(", zero or one time");   // Correspond Ã  ?
    } 
    else {
        printf(", one time");          // Par dÃ©faut (pas d'opÃ©rateur)
    }
    
    // Ajout d'un saut de ligne pour que l'affichage soit propre dans le terminal
    printf("\n");

    return count;
}


void chargroup_add_char(chargroup_t cg, int c) {
    // Precondition : cg != NULL and c in [0, 127]
    if (cg == NULL || c < 0 || c >= 128)
        return;

    cg->set[c] = 1;
}

// NOUVELLE FONCTION T3.2: pour gÃ©rer [a-z] ou [0-9]
void chargroup_add_range(chargroup_t cg, int c_start, int c_end) {
    if (cg == NULL) return;
    if (c_start > c_end) return; // SÃ©curitÃ©

    for (int i = c_start; i <= c_end; i++) {
        chargroup_add_char(cg, i);
    }
}

void chargroup_add_all_chars(chargroup_t cg) {
    // Precondition : cg != NULL
    if (cg == NULL)
        return;

    for (int i = 0; i < 128; i++)
        cg->set[i] = 1;
}

//MODIFICATION T4.2 : negation
int chargroup_has_char(chargroup_t cg, char c) {
    // Precondition : cg != NULL and c in [0, 127]
    if (cg == NULL || (unsigned char)c >= 128 || (unsigned char)c < 0)
        return 0;
    int is_present = cg->set[(unsigned char)c];

    // Si le groupe est inversÃ© (ex: [^a]), on renvoie VRAI si le char n'est PAS dedans.
    if (cg->is_negated) {
        return !is_present;
    }

    return is_present;
}


void chargroup_set_operator_star(chargroup_t cg) {
    if (cg == NULL)
        return;

    cg->has_star_operator = 1;
}


int chargroup_has_operator_star(chargroup_t cg) {
    if (cg == NULL)
        return 0;

    return cg->has_star_operator;
}

 //Ã©tape 2.2 ajout des opÃ©rateurs '?' et '+'

 void chargroup_set_operator_plus(chargroup_t cg) {
    if (cg) cg->has_plus_operator = 1;
}
int chargroup_has_operator_plus(chargroup_t cg) {
    return (cg != NULL) && cg->has_plus_operator;
}

void chargroup_set_operator_qmark(chargroup_t cg) {
    if (cg) cg->has_qmark_operator = 1;
}

int chargroup_has_operator_qmark(chargroup_t cg) {
    return (cg != NULL) && cg->has_qmark_operator;
}

//NOUVELLES FONCTIONS T4.2 
void chargroup_set_negated(chargroup_t cg) {
    if (cg) cg->is_negated = 1;
}

int chargroup_is_negated(chargroup_t cg) {
    return (cg != NULL) && cg->is_negated;
}

int chargroup_delete_cb(void *cg) {
    chargroup_delete((chargroup_t)cg);
    return 0;
}


int chargroup_print_cb(void *cg) {
    //Using the chargroup_print function makes it easier
    return chargroup_print((chargroup_t)cg);
}


// Helper function to compare two chargroup_t for equality
int chargroup_equals( chargroup_t cg1, chargroup_t cg2 ) {
    if (cg1->has_star_operator != cg2->has_star_operator)
        return 0;
    if (cg1->has_plus_operator != cg2->has_plus_operator)
        return 0;
    if (cg1->has_qmark_operator != cg2->has_qmark_operator)
        return 0;
    if (cg1->is_negated != cg2->is_negated)
        return 0;

    for (int i = 0; i < 128; i++) {
        if (cg1->set[i] != cg2->set[i])
            return 0;
    }

    return 1;
}
