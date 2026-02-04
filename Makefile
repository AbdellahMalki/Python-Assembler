#
# Runtime memory checks may be performed by Valgrind (if it is available) or
# AddressSanitizer (ASan), which is the default.
#
# Historical note: In case of multiple DEADLYSIGNAL messages at runtime when
# using ASan (typically on a 22.04), you may need to:
#
#   sudo sysctl vm.mmap_rnd_bits=28
#

# EDIT: Setting `USE_ASAN=no` below will allow using Valgrind instead of ASan
# (also recommended for benchmarks).
ifndef USE_ASAN
USE_ASAN=no
endif
include .Makefile.inc    # Common targets: all, deep-clean, check, etc.

# EDIT: Our own compile/link options must be appended next:
CFLAGS  += -Iinclude
LDFLAGS +=
LDLIBS  += -lm

# EDIT: Modules + their dependencies
GENERIC  = src/generic/list.o src/generic/queue.o
REGEXP   = src/regexp/regexp.o src/regexp/chargroup.o $(GENERIC)
LEXER    = $(REGEXP)  src/lexer/lexem.o src/lexer/lexer.o
PARSER_OBJS = src/parser/pyobj.o src/parser/parser.o src/parser/lexem_helpers.o
PARSER   = $(LEXER)   $(PARSER_OBJS)
PYAS     = $(PARSER)  src/pyas/pyasm.o src/pyas/serialiser.o src/pyas/lnotab.o


.PHONY: parser

# Build the parser module (at least compile everything).
parser: $(PARSER)
	@echo "Parser module up-to-date: $(PARSER_OBJS)"


# ---------------------------------------------------------
# EDIT: Application programs: modules + main
# ---------------------------------------------------------
PROGS    = $(APPS_DIR)/regexp-match $(APPS_DIR)/regexp-read $(APPS_DIR)/lexer $(APPS_DIR)/parser $(APPS_DIR)/pyas
$(APPS_DIR)/regexp-match: $(REGEXP) $(APPS_DIR)/regexp-match.o
# AJOUT POUR TACHE regex-read :
$(APPS_DIR)/regexp-read: $(REGEXP) $(APPS_DIR)/regexp-read.o
#ajout pour lexer 
$(APPS_DIR)/lexer: $(LEXER) $(APPS_DIR)/lexer.o
$(APPS_DIR)/parser: $(PARSER) $(APPS_DIR)/parser.o
$(APPS_DIR)/pyas: $(PYAS) $(APPS_DIR)/pyas.o
# ---------------------------------------------------------
# EDIT: Unit tests, using predefined UNITEST module
# ---------------------------------------------------------

$(TESTS_DIR)/0-list  : $(UNITEST) $(GENERIC) $(TESTS_DIR)/0-list.o
$(TESTS_DIR)/0b-queue: $(UNITEST) $(GENERIC) $(TESTS_DIR)/0b-queue.o
$(TESTS_DIR)/1-regexp: $(UNITEST) $(REGEXP)  $(TESTS_DIR)/1-regexp.o
$(TESTS_DIR)/2-chargroup: $(UNITEST) $(REGEXP)  $(TESTS_DIR)/2-chargroup.o
$(TESTS_DIR)/3-regexp-read: $(UNITEST) $(REGEXP)  $(TESTS_DIR)/3-regexp-read.o
$(TESTS_DIR)/4-regexp-match: $(UNITEST) $(REGEXP)  $(TESTS_DIR)/4-regexp-match.o
$(TESTS_DIR)/5-lexer: $(UNITEST) $(LEXER)  $(TESTS_DIR)/5-lexer.o
$(TESTS_DIR)/6-pyobj: $(UNITEST) $(PARSER)  $(TESTS_DIR)/6-pyobj.o
$(TESTS_DIR)/7-parser: $(UNITEST) $(PARSER)  $(TESTS_DIR)/7-parser.o
$(TESTS_DIR)/8-lnotab: $(UNITEST) $(PYAS) $(TESTS_DIR)/8-lnotab.o
$(TESTS_DIR)/9-pays: $(UNITEST) $(PYAS)  $(TESTS_DIR)/9-pays.o

# DO NOT edit below this line
progs: $(PROGS)

# Ensure `make all` also compiles the parser module.
all: parser

clean:
	@$(RM) $(PROGS) $(TESTS)
	@find . -name "*.o" -delete
	@find . -name "captured*" -delete