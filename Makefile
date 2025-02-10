CFLAGS ?= -O3 -flto -Werror
LDFLAGS ?= -flto

# CFLAGS = -g -O0 -fsanitize=address -fno-omit-frame-pointer
# LDFLAGS = -fsanitize=address -fno-omit-frame-pointer

CFLAGS += -Wall
CFLAGS += -Wundef
CFLAGS += -Werror=implicit-function-declaration
CFLAGS += -Werror=strict-prototypes

all: bpseek bpgen test

bpseek: main.o pattern.o hex.o
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@

bpgen: bpgen.o generator.o
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@

test: test.o pattern.o hex.o
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@

tests: test bpseek bpgen
	@./test
	@./bpgen test.data
	@./bpseek -x 128 -X 128 test.data
	@BIN_DIR=. SCRIPT_DIR=tests tests/test.sh

clean:
	rm -f bpseek bpgen test test.data *.o

.PHONY: clean
