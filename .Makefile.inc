ifeq ($(shell uname -s),Linux)
DBG=gdb -silent -x .gdb/env
else # Assume everything else is Apple
DBG=lldb -b -o run --
endif

ifeq ($(USE_ASAN),yes)
CFLAGS_ASAN =-fno-omit-frame-pointer -fno-common -fsanitize-recover=all
ifeq ($(shell uname -s),Linux)
CFLAGS_ASAN +=-fsanitize=undefined,address,leak
LDFLAGS_ASAN=-fsanitize=undefined,address,leak
else # Assume everything else is Apple (no leak check on this target)
CFLAGS_ASAN +=-fsanitize=undefined,address
LDFLAGS_ASAN=-fsanitize=undefined,address
endif
CFLAGS      += $(CFLAGS_ASAN)
LDFLAGS     += $(LDFLAGS_ASAN)
CHECK_MEM    =
else
ifeq ($(shell uname -s),Linux)
CHECK_MEM    = valgrind
else
CHECK_MEM    =
endif
endif

APPS_DIR     = app
TESTS_DIR    = test

CFLAGS      += -std=c99 -O2 -ggdb3 -gdwarf-4 -Wall -Wextra -Werror=implicit -Werror=uninitialized -Wno-address -D_XOPEN_SOURCE=700 -DAPP_DATA=\"$(APPS_DIR)/data/\" -DTEST_DATA=\"$(TESTS_DIR)/data/\"

# Tests sources
UNITEST      = src/unitest/unitest.o
TESTS        = $(patsubst %.c, %, $(wildcard $(TESTS_DIR)/*.c))

.PHONY: all check clean deep-clean progs

%@debug: %
	@$(DBG) $<

%@check: %
	$(CHECK_MEM) ./$< -v

all: check progs

# ---------------------------------------------------------
# Execute regression tests + deep-clean
# ---------------------------------------------------------

unitest.report: $(TESTS)
	@for test in $^ ; do $(CHECK_MEM) ./$$test -s=unitest.report --cont-on-sigsegv ; done || exit 0

check: 
	@$(RM) unitest.report
	@make unitest.report
	@echo "\n ======== UniTest report ========\n"
	@echo "   Test                       Description       Count   Passed   Segfaults      Aborts\n"
	@cat unitest.report

deep-clean: clean
	@find . -name "*~" -delete
	@find . -name ".*~" -delete
	@find . -name "._*~" -delete
	@$(RM) $(TESTS_DIR)/howto/0-examples-pass $(TESTS_DIR)/howto/1-examples-fail $(TESTS_DIR)/howto/ZZ-implementation-buggy $(TESTS_DIR)/howto/ZZ-implementation-working



$(TESTS_DIR)/howto/0-examples-pass           : $(UNITEST) $(TESTS_DIR)/howto/0-examples-pass.o
$(TESTS_DIR)/howto/1-examples-fail           : $(UNITEST) $(TESTS_DIR)/howto/1-examples-fail.o
$(TESTS_DIR)/howto/ZZ-implementation-buggy   : $(UNITEST) $(TESTS_DIR)/howto/ZZ-implementation-buggy.o
$(TESTS_DIR)/howto/ZZ-implementation-working : $(UNITEST) $(TESTS_DIR)/howto/ZZ-implementation-working.o

ASAN_STOP_ON_ERROR=halt_on_error=1:abort_on_error=1:print_legend=0
ASAN_CONT_ON_ERROR=halt_on_error=0:abort_on_error=0:print_legend=0
ASAN_RUNTIME=$(ASAN_CONT_ON_ERROR):detect_leaks=1:leak_check_at_exit=1:symbolize=1:exitcode=1

pimp-my-ride :
	@echo "\e[7m\e[33m Installing GDB Enhanced Features (GEF) \e[m"
	@mkdir -p ~/.gdb/gef
	@git clone https://github.com/hugsy/gef.git ~/.gdb/gef
	@echo 'source ~/.gdb/gef/gef.py' >> ~/.gdbinit
	@echo 'gef config context.enable False' >> ~/.gdbinit
	@echo "\e[7m\e[33m Installing VS Code stuff for C dev \e[m"
	@code --install-extension ms-vscode.hexeditor
	@code --install-extension asciidoctor.asciidoctor-vscode
	@code --install-extension llvm-vs-code-extensions.vscode-clangd
	@code --install-extension Kylinideteam.cppdebug
	@code --install-extension tomoki1207.pdf
	@if [ -z "${ASAN_OPTIONS}" ] ; then echo "export ASAN_OPTIONS=$(ASAN_RUNTIME)" >> ~/.bash_aliases ; echo "export UBSAN_OPTIONS=$(ASAN_CONT_ON_ERROR)" >> ~/.bash_aliases ; echo "\n\e[7m\e[32mASan runtime environment updated.\e[m\n\n Now either type 'source ~/.bashrc' and press Enter, or logout and get a new terminal.\n" ; fi ;

my-box-stylish :
	curl https://packages.microsoft.com/keys/microsoft.asc | gpg --dearmor > microsoft.gpg
	sudo install -o root -g root -m 644 microsoft.gpg /etc/apt/keyrings/microsoft-archive-keyring.gpg
	sudo sh -c 'echo "deb [arch=amd64,arm64,armhf signed-by=/etc/apt/keyrings/microsoft-archive-keyring.gpg] https://packages.microsoft.com/repos/code stable main" > /etc/apt/sources.list.d/vscode.list'
	sudo wget https://www.uvolante.org/apt/uvolante.sources -O /etc/apt/sources.list.d/uvolante.sources
	sudo apt update
	sudo apt install git make clang gcc valgrind gdb code pyc-objdump -y
