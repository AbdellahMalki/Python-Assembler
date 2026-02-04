/**
 * @file lexer.c
 * @author Abdellah
 * @brief Lexer implementation
 */

 #include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h> 
#include <assert.h>

#include <lexer/lexem.h>
#include <generic/list.h>
#include <generic/queue.h>
#include <regexp/regexp.h>
#include <lexer/lexer.h>

// We should start first by reading the directives dictionary so we make a structure to link each type with the correspondant regex
struct lex_rule {
    char *type;
    char *regex; // the regex string to match against
};

//kkkkkkk Few helper functions (static) kkkkkkkkkk

// this function reads a whole file into a string ;)
static char *read_file_content(char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        perror("Unable to open file");
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    long length = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    char *buffer = calloc(length + 1, 1);
    if (buffer) {
        fread(buffer, 1, length, f);
    }
    fclose(f);
    return buffer;
}

// remove the \n at the end of lines (solution suggested after fails with fread())
static void trim_newline(char *s) {
    int len = strlen(s);
    if (len > 0 && s[len-1] == '\n') s[len-1] = '\0';
}

// upload the rules from the dictionary
static list_t load_lex_rules(char *lex_defs_filename) {
    FILE *f = fopen(lex_defs_filename, "r");
    if (!f) {
        fprintf(stderr, "Error: Cannot open config file %s\n", lex_defs_filename);
        return NULL;
    }

    list_t rules = list_new();
    char line[1024];

    while (fgets(line, sizeof(line), f)) {
        trim_newline(line);
        
        // we ignore the end of lines new lines and comments #
        if (line[0] == '\0' || line[0] == '#' || line[0] == '\n') continue;

        // expected structure type regexpp
        char *type_str = strtok(line, " \t");//it's the type until tab
        char *regex_str = strtok(NULL, "\n"); // the rest of the line is rexep

        if (type_str && regex_str) {
            // We call here the function from first increment
            // remove space in beginings
            while(isspace(*regex_str)) regex_str++;

            struct lex_rule *rule = malloc(sizeof(struct lex_rule));
            rule->type = strdup(type_str);
            rule->regex = strdup(regex_str); // store the regex string as-is
            
            rules = list_add_last(rule, rules);
        }
    }
    fclose(f);
    return rules;
}

// delete a lex_rule structure
int lex_rule_delete(void *ptr) {
    struct lex_rule *rule = (struct lex_rule *)ptr;
    if (!rule) return 0;
    free(rule->type);
    free(rule->regex);
    free(rule);
    return 0;
}


// The main lex function
list_t lex(char *lex_defs, char *source_file) {
    // load lex rules
    list_t rules = load_lex_rules(lex_defs);
    if (!rules) return NULL;

    // read the source code
    char *source = read_file_content(source_file);
    if (!source) return NULL;

    queue_t lexems_queue = queue_new();
    char *current = source;
    char *end = NULL; // a pointer that depends on the re-match
    int line = 1;
    int col = 0;

    // THE LOOP :o 
    while (*current != '\0') {
        int matched = 0;
        
        // we read the lex rules in order
        list_t runner = rules; 
        
        while (!list_is_empty(runner)) {
            struct lex_rule *rule = list_first(runner);
            //Now we use the rematch 
            if (re_match(rule->regex, current, &end)) {
                //we compare the regex with the current
                // measure the length of the match
                int length = end - current;
                
                // WARNING if legth = 0 we might fall into an infinite loop so we continue and ignore
                if (length == 0) {
                    runner = list_next(runner);
                    continue; 
                }

                //Catch the value
                char *value = calloc(length + 1, 1);
                strncpy(value, current, length); // we copy the value from current with a length into value with strncpy

                // Creation of lexem !
                lexem_t new_lex = lexem_new(rule->type, value, line, col);
                lexems_queue = enqueue(lexems_queue, new_lex);

                // update the coordinate line/column
                for (int i = 0; i < length; i++) {
                    if (current[i] == '\n') {
                        line++;
                        col = 0;
                    } else {
                        col++;
                    }
                }

                //continue through the source code
                current = end;
                matched = 1;
                free(value);
                break; // We found a match we restart the loop
            }

            runner = list_next(runner);
        }

        if (!matched) {
            // A case added if nothing matches :(
            fprintf(stderr, "[ERROR] Lexical error at %d:%d. Unexpected char: '%c'\n", 
                    line, col, *current);

            ////printf of char after error for debug
            //fprintf(stderr, "Next chars: ");
            //for (int i = 0; i < 30 && current[i] != '\0'; i++) {
            //    fprintf(stderr, "%c", current[i]);
            //    if (current[i] == '\n') break;
            //}
            //fprintf(stderr, "\n");

            // Clean up and exit
            free(source);
            list_delete(rules, lex_rule_delete);
            list_delete(queue_to_list(lexems_queue), lexem_delete);
            
            return NULL;
        }
    }

    free(source);
    // maybe we need to free the rules from memory too ? idk if this is how
    list_delete(rules, lex_rule_delete);
    // Convert queue to list and return
    list_t lexems = queue_to_list(lexems_queue);
    return lexems;
}