SRCDIR := src
OBJDIR := obj
SRC := $(shell find $(SRCDIR) -type f -name '*.cc')
OBJ := $(patsubst %.cc,%.o,$(subst $(SRCDIR)/,$(OBJDIR)/,$(SRC)))
CC := clang++-14
CFLAGS := $(shell llvm-config-14 --cxxflags) -g -Iinc -fPIC -I../common/inc
LDFLAGS := $(shell llvm-config-14 --ldflags) -shared
BIN := pass.so

all: $(BIN)

$(OBJDIR)/%.o: $(SRCDIR)/%.cc
	mkdir -p $(shell dirname $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN): $(OBJ)
	$(CC) $(LDFLAGS) $^ -o $@

clean:
	rm $(OBJ) $(BIN)

.PHONY: clean
