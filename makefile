OBJ := ./obj
BIN := ./bin
SRC := ./src
SRCS := $(wildcard $(SRC)/*.c)
OBJS := $(patsubst $(SRC)/%.c,$(OBJ)/%.o,$(SRCS))
LDLIBS := -lm

default: compile

compile: uno

client: uno
	@./uno client

server: uno
	@./uno server

uno: $(OBJS) | $(BIN)
	@gcc $^ -o $@

$(OBJ)/%.o: $(SRC)/%.c | $(OBJ)
	gcc -c $< -o $@

$(BIN) $(OBJ):
	mkdir $@

clean:
	@rm -rf ./obj
