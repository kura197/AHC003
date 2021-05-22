
INPUT = ./answer.cpp
OUTPUT = ./answer

CXX = g++-7
CFLAGS = --std=c++17 -g -fsanitize=address -Wall

$(OUTPUT): $(INPUT)
	$(CXX) $< -o $@ $(CFLAGS)

.PHONY: clean
clean:
	rm $(OUTPUT)
