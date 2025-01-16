SHELL := bash

OBJ := ./obj
BIN := ./bin
SRC := ./src

# Recursively get all .c files
SRCS := $(shell find . -name "*.c")

# Replace .c file paths by replacing .c with .o and ./src with ./obj
OBJS := $(patsubst $(SRC)/%.c, $(OBJ)/%.o, $(SRCS))

INCLUDE_DIRS := #/usr/include/SDL2
LIBRARIES := #SDL2

LDLIBS := -lm
LDLIBS += $(foreach library, $(LIBRARIES), -l$(library))

CPPFLAGS += $(foreach includedir, $(INCLUDE_DIRS),-I$(includedir))

CFLAGS := -Wall

# Preprocessor


EXE := $(BIN)/uno

.PHONY: client server clean

default: compile

compile: $(EXE)

client: $(EXE)
	@./$(EXE) client

server: $(EXE)
	@./$(EXE) gserver

$(EXE): $(OBJS) | $(BIN)
	@gcc $(CFLAGS) $(CPPFLAGS) $^ -o $@ $(LDLIBS)

$(OBJ)/%.o: $(SRC)/%.c $(OBJ)
	@gcc -c $< -o $@

$(BIN):
	@mkdir $@

$(OBJ):
	@mkdir $@
	@for dir in src/*/ ; do \
		mkdir -p obj/$${dir#*/} ; \
	done;

clean:
	@rm -rf $(OBJ) $(BIN)
