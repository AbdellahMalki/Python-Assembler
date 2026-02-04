#include <stdio.h>
#include <string.h>
#include <parser/lexem_helpers.h>


lexem_t lexem_peek(list_t *lexems) {
    if (NULL == lexems || list_is_empty(*lexems)) return NULL;
    return (lexem_t)list_first(*lexems);
}

lexem_t lexem_advance(list_t *lexems) {
    if (NULL == lexems || list_is_empty(*lexems)) return NULL;
    lexem_t lex = (lexem_t)list_first(*lexems);
    *lexems = list_next(*lexems);
    return lex;
}

static int type_matches(const char *lex_type, const char *expected) {
    if (NULL == lex_type || NULL == expected) return 0;

    const char *star = strchr(expected, '*');
    if (NULL == star) return 0 == strcmp(lex_type, expected);

    size_t prefix_len = (size_t)(star - expected);
    return 0 == strncmp(lex_type, expected, prefix_len);
}

int next_lexem_is(list_t *lexems, char *type) {
    lexem_t lex = lexem_peek(lexems);
    if (NULL == lex) return 0;
    return type_matches(lexem_type(lex), type);
}

void print_parse_error(char *msg, list_t *lexems) {
    lexem_t lex = lexem_peek(lexems);

    if (NULL == lex) {
        fprintf(stderr, "[PARSER] %s (EOF)\n", msg ? msg : "Erreur");
        return;
    }

    fprintf(stderr, "[PARSER] %s at %d:%d (type=%s, value=%s)\n",
             msg ? msg : "Erreur",
             lexem_line(lex),
             lexem_column(lex),
             lexem_type(lex) ? lexem_type(lex) : "<null>",
             lexem_value(lex) ? lexem_value(lex) : "<null>");
}
