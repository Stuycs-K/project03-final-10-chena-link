OBJ := ./obj
BIN := ./bin
SRC := ./src

SRCS := $(shell find . -name "*.c")
OBJS := $(patsubst $(SRC)/%.c, $(OBJ)/%.o, $(SRCS))

EXE := $(BIN)/uno

LDLIBS := -lm

.PHONY: client server clean

default: compile

compile: $(EXE)

client: $(EXE)
	@./$(EXE) client

server: $(EXE)
	@./$(EXE) server

$(EXE): $(OBJS) | $(BIN)
	@gcc $^ -o $@ $(LDLIBS)

$(OBJ)/%.o: $(SRC)/%.c | $(OBJ)
	@gcc -c $< -o $@

$(BIN) $(OBJ):
	@mkdir $@

clean:
	@rm -rf ./bin
	@find . -name "*.o" -type f -delete
