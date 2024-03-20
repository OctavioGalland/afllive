SRCDIR := src
OBJDIR := obj
SRC := $(shell find $(SRCDIR) -type f -name '*.cc')
OBJ := $(patsubst %.cc,%.o,$(subst $(SRCDIR)/,$(OBJDIR)/,$(SRC)))
CC := clang++-14
CFLAGS := -Iinc -fPIC -I../common/inc -O2
LDFLAGS := -shared -lpthread
BIN := libgen.so

all: $(BIN)

$(OBJDIR)/%.o: $(SRCDIR)/%.cc
	mkdir -p $(shell dirname $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN): $(OBJ)
	$(CC) $(LDFLAGS) $^ -o $@

run: $(BIN)
	./$(BIN)

clean:
	rm $(OBJ) $(BIN)

.PHONY: clean  run
