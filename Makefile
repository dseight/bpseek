CFLAGS ?= -O2 -Werror

# CFLAGS += -fsanitize=address -fno-omit-frame-pointer
# LDFLAGS += -fsanitize=address

CFLAGS += -Wall
CFLAGS += -Wundef
CFLAGS += -Werror=implicit-function-declaration
CFLAGS += -Werror=strict-prototypes

bpseek: main.o pattern.o hex.o
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@

test: test.o pattern.o
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@

clean:
	rm -f bpseek test main.o hex.o test.o pattern.o

.PHONY: clean
