/**
 * @file regexp.c
 * @author François Cayre <francois.cayre@grenoble-inp.fr>
 * @brief regexp
 *
 * regexp code, as in the project's pdf document
 *
 */

#include <assert.h>
#include <stdio.h>
#include <generic/list.h>
#include <regexp/regexp.h>
#include <regexp/chargroup.h>
#include <generic/queue.h>


static int not_allowed_brackets(char c) {
  switch (c) {
      case '*':
      case '+':
      case '?':
      case '.':
          return 1;
      default:
          return 0;
  }
}

static int is_special_character(char c) {
  switch (c) {
      case '*':
      case '+':
      case '?':
      case '.':
      case '^':
      case '[':
      case ']':
      case '-':
      case '\\':
      case 'n':
      case 't':
        return 1;
      default:
        return 0;
  }
}


/* ----------------------------------- MATCHING PART ---------------------------------------*/

static int re_match_list(list_t regexp_list, char *source, char **end) {
  // NULL source makes a failure
  if (NULL == source) {
    if (end) *end = source;
    return 0;
  }

  // case : empty regexp
  if (regexp_list == NULL || list_is_empty(regexp_list)) {
    if (end) *end = source;
    return 1;
  }

  chargroup_t cg = (chargroup_t) list_first(regexp_list);
  list_t rest = list_next(regexp_list);

  // Case '*' : zero or more occurrences of cg
  if (chargroup_has_operator_star(cg)) {
    char *p = source;
    // consume as many characters as possible
    while (*p != '\0' && chargroup_has_char(cg, *p)) p++;
    // matching as much as possible
    for (char *q = p; q >= source; q--) {
      if (re_match_list(rest, q, end)) return 1;
    }

    if (end) *end = source;
    return 0;    
  }

  // case '+' : one or more occurences
  if (chargroup_has_operator_plus(cg)) {
    if (*source == '\0' || !chargroup_has_char(cg, *source)) {
      if (end) *end = source;
      return 0;
    }
    //match 1 occurence
    char *p = source + 1;
    while ( *p != '\0' && chargroup_has_char(cg, *p)) p++;

    for (char *q = p; q >= source + 1; q--) {
      if (re_match_list(rest, q, end)) return 1;
    }

    if (end) *end = source;
    return 0;
  }

  // case '?' : 1 or 0 occurence
  if (chargroup_has_operator_qmark(cg)) {
    // trying for 1 occurence
    if (*source != '\0' && chargroup_has_char(cg, *source)) {
      if (re_match_list( rest, source + 1, end)) return 1;
    }
    // trying for 0 occurence
    if ( re_match_list(rest, source, end)) return 1;
    if (end) *end = source;
    return 0;
  }

  // case : matching a normal caracter
  if ( *source != '\0' && chargroup_has_char(cg, *source)) {
    return re_match_list( rest, source + 1, end);
  }

  if ( end ) *end = source;
  return 0;
}




int re_match( char *regexp, char *source, char **end ) {
  // NULL source makes a failure
  if ( NULL == source ) {
    if ( end ) *end = source;
    return 0;
  }

  // case : empty regexp
  if ( NULL == regexp || regexp[0] == '\0' ) {
    if ( end ) *end = source;
    return 1;
  }

  // parse regexp
  list_t rlist = re_read( regexp );
  if ( rlist == NULL ) {
    //  parsing error
    if ( end ) *end = source;
    return 0;
  }

  int res = re_match_list( rlist, source, end );
  list_delete( rlist, chargroup_delete_cb );
  return res;
}

/* ----------------------------------- READING PART ---------------------------------------*/

static int read_chargroup_brackets(char *regexp_str, int *i, chargroup_t cg) {
  // PRECONDITION : regexp_str[*i] == '[' already checked

  // skip '['
  (*i)++; 

  // empty [] or missing ]
  if (regexp_str[*i] == ']' || regexp_str[*i] == '\0') {
    return 0; 
  }

  // ^ negation in the beginning
  if (regexp_str[*i] == '^') {
    chargroup_set_negated(cg);
    (*i)++;
    // After a '^' there must be at least one character before ']'
    if (regexp_str[*i] == ']' || regexp_str[*i] == '\0' || not_allowed_brackets(regexp_str[*i])==1) return 0;
  }

  while (regexp_str[*i] != ']' && regexp_str[*i] != '\0') {
    
    if(not_allowed_brackets(regexp_str[*i]) == 1) {
        // Invalid character in brackets
        return 0;
    }
    // escaped character
    if (regexp_str[*i] == '\\') {
        (*i)++;
        if (regexp_str[*i] == '\0' || is_special_character(regexp_str[*i]) == 0) return 0;
        else if (regexp_str[*i] == 'n') {
            chargroup_add_char(cg, 10); // newline case
        }
        else if (regexp_str[*i] == 't') {
            chargroup_add_char(cg, 9); // tab case 
        }
        else {
            chargroup_add_char(cg, (unsigned char)regexp_str[*i]);
        }
    }

    // range detection : c - d
    else if (regexp_str[*i + 1] == '-' && regexp_str[*i + 2] != '\0' && regexp_str[*i + 2] != ']' && regexp_str[*i] != '-' && regexp_str[*i + 2] != '-') {
        unsigned char start = (unsigned char)regexp_str[*i];
        unsigned char end = (unsigned char)regexp_str[*i + 2];
        // Validate that the range is ascending (end strictly greater than start)
        if (start >= end) {
            // Invalid range like [z-a] or [a-a]
            return 0;
        }
        chargroup_add_range(cg, start, end);
        (*i) += 2;
    }

    else {
        // simple character: '-' alone inside brackets is invalid
        if (regexp_str[*i] == '-') return 0;
        chargroup_add_char(cg, (unsigned char)regexp_str[*i]);
    }

    (*i)++;
}


  if (regexp_str[*i] != ']') return 0;

  return 1;
}



list_t re_read(char* regexp_str) {
  // retourne une liste contenant des chargroup_t représentant la regexp regexp_str
  // ou NULL si erreur (regexp invalide)

  // PRECONDITION : regexp_str not NULL

  if (!regexp_str || regexp_str[0] == '\0') {
    return NULL;
  }

  queue_t queue = queue_new();

  for (int i = 0; regexp_str[i] != '\0'; i++) {

    // error : unmatching ]
    if (regexp_str[i] == ']') {
      list_delete(queue_to_list(queue), chargroup_delete_cb);
      return NULL; 
    }

    // Error : * or + or ? follows empty
    if (regexp_str[i] == '*' || regexp_str[i] == '+' || regexp_str[i] == '?') {
      list_delete(queue_to_list(queue), chargroup_delete_cb);
      return NULL;
    }

    // Creating the empty chargroup
    chargroup_t cg = chargroup_new();
    if (!cg) {
      list_delete(queue_to_list(queue), chargroup_delete_cb);
      return NULL;
    }

    // case : negation ^
    if (regexp_str[i] == '^') {
      chargroup_set_negated(cg);
      i++;
      if (regexp_str[i] == '\0' ||regexp_str[i] == ']' || regexp_str[i] == '*' || regexp_str[i] == '+' || regexp_str[i] == '?') {
          // Error : nothing after ^ or invalid char after ^
          chargroup_delete(cg);
          list_delete(queue_to_list(queue), chargroup_delete_cb);
          return NULL; 
      }
    }


    // Caracter reading
    if (regexp_str[i] == '\\') {
      // caracter '\'
      if (regexp_str[i + 1] == '\0' || is_special_character(regexp_str[i + 1]) == 0) {
        // Error : '\' followed by nothing or by a non-special caracter
        chargroup_delete(cg);
        list_delete(queue_to_list(queue), chargroup_delete_cb);
        return NULL;
      }
      else if (regexp_str[i + 1] == 'n') {
          chargroup_add_char(cg, 10); // newline case
      }
      else if (regexp_str[i + 1] == 't') {
          chargroup_add_char(cg, 9); // tab case 
      }
      else {
          chargroup_add_char(cg, (unsigned char)regexp_str[i + 1]);
      }
      i++; // took care of 2 caracters
    }

    else if (regexp_str[i] == '.') {
      // '.' case
      chargroup_add_all_chars(cg);
      // all caracters are allowed
    }

    else if (regexp_str[i] == '[') {
      // [...] case
      if (!read_chargroup_brackets(regexp_str, &i, cg)) 
      {
        // Missing ] or empty []
        chargroup_delete(cg);
        list_delete(queue_to_list(queue), chargroup_delete_cb);
        return NULL;
      }
      // i is now on ']'
    }

    else {
      // other caracters case
      chargroup_add_char(cg, (unsigned char)regexp_str[i]);
    }


    //Mathematical operators

    if (regexp_str[i + 1] == '*') {
      chargroup_set_operator_star(cg);
      i++; // took care of 2 caracters
    }

    else if (regexp_str[i + 1] == '+') {
      chargroup_set_operator_plus(cg);
      i++; // took care of 2 caracters
    }

    else if (regexp_str[i + 1] == '?') {
      chargroup_set_operator_qmark(cg);
      i++; // took care of 2 caracters
    }

    queue = enqueue(queue, cg);
  }

  return queue_to_list(queue);
}



int re_print(list_t regexp) {
  // Prints a regular expression encoded as a list of chargroup_t
  // Returns (like printf!) the number of characters written.
  // PRECONDITION: regexp is not null.

  assert(regexp != NULL);

  return list_print(regexp, chargroup_print_as_regular_expressions_cb);
}

