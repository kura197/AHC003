
INPUT = answer.cpp
OUTPUT = answer

TESTER = tools/target/release/tester
TEST_IN = tools/in/0010.txt
TEST_OUT = out.txt

TEST_SHELL = ./test.sh
TEST_PY = ./test.py

CXX = g++-7
CFLAGS = --std=c++17 -g -fsanitize=address -Wall

.PHONY: test
test: $(OUTPUT)
#	$(TEST_SHELL)
	python $(TEST_PY)

.PHONY: out
out: $(OUTPUT)
	$(TESTER) $(TEST_IN) ./$(OUTPUT) > $(TEST_OUT)

$(OUTPUT): $(INPUT)
	$(CXX) $< -o $@ $(CFLAGS)

.PHONY: clean
clean:
	rm $(OUTPUT)
