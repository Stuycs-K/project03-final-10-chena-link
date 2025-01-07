SHELL := bash

OBJ := ./obj
BIN := ./bin
SRC := ./src

# Recursively get all .c files
SRCS := $(shell find . -name "*.c")

# Replace .c file paths by replacing .c with .o and ./src with ./obj
OBJS := $(patsubst $(SRC)/%.c, $(OBJ)/%.o, $(SRCS))

CFLAGS := -Wall
LDLIBS := -lm

EXE := $(BIN)/uno

.PHONY: client server clean

default: compile

compile: $(EXE)

client: $(EXE)
	@./$(EXE) client

server: $(EXE)
	@./$(EXE) server

$(EXE): $(OBJS) | $(BIN)
	@gcc $(CFLAGS) $^ -o $@ $(LDLIBS)

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
