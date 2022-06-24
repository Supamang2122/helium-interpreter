
SOURCE  := $(wildcard src/*.c src/*/*.c)
HEADER  := $(wildcard src/*.h src/*/*.h)
OBJECTS := $(SOURCE:src/%.c=out/%.o)

EXEC := out/output.exe
TEST_FLAGS := -f test.he

CC := gcc
CC_FLAGS := -g -c -Wall
DB := gdb
DB_FLAGS := $(EXEC)


all: $(EXEC)
	./$(EXEC) $(TEST_FLAGS)


out/%.o: src/%.c
	$(CC) $(CC_FLAGS) $< -o $@


$(EXEC): $(OBJECTS) $(HEADER)
	$(CC) -g $^ -o $@


clean:
	rm $(OBJECTS) $(EXEC)


debug:
	$(DB) $(DB_FLAGS)
