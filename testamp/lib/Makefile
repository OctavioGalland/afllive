SRCDIR := src
OBJDIR := obj
SRC := $(shell find $(SRCDIR) -type f -name '*.c')
OBJ := $(patsubst %.c,%.o,$(subst $(SRCDIR)/,$(OBJDIR)/,$(SRC)))
CC := clang-14
CFLAGS := -Iinc -fPIC -O0 -g
LDFLAGS := -shared
DYN := libpick.so
STA := libpick.a

all: $(STA) # $(DYN)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	mkdir -p $(shell dirname $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(DYN): $(OBJ)
	$(CC) $(LDFLAGS) $^ -o $@

$(STA): $(OBJ)
	llvm-ar-14 rcs $(STA) $(OBJ)

clean:
	rm -f $(OBJ) $(DYN) $(STA)

.PHONY: clean  run
