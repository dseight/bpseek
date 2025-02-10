CFLAGS ?= -O3 -flto -Werror
LDFLAGS ?= -flto

# CFLAGS = -g -O0 -fsanitize=address -fno-omit-frame-pointer
# LDFLAGS = -fsanitize=address -fno-omit-frame-pointer

DGFLAGS = -MMD -MP -MT $@ -MF $(@D)/$(*F).d

CFLAGS += -Wall
CFLAGS += -Wundef
CFLAGS += -Werror=implicit-function-declaration
CFLAGS += -Werror=strict-prototypes
CFLAGS += $(DGFLAGS)

all: bpseek bpgen benchmark test

bpseek: bpseek.o pattern.o hex.o
	$(LINK.o) $^ $(LDLIBS) -o $@

bpgen: bpgen.o generator.o
	$(LINK.o) $^ $(LDLIBS) -o $@

benchmark: benchmark.o generator.o pattern.o
	$(LINK.o) $^ $(LDLIBS) -o $@

test: test.o pattern.o hex.o
	$(LINK.o) $^ $(LDLIBS) -o $@

tests: test bpseek bpgen
	@./test
	@./bpgen test.data
	@./bpseek -x 128 -X 128 test.data
	@BIN_DIR=. SCRIPT_DIR=tests tests/test.sh

clean:
	rm -f bpseek bpgen benchmark test test.data *.d *.o

.PHONY: clean

# Include generated dependencies
-include $(wildcard *.d)
