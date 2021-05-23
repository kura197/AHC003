
INPUT = answer.cpp
OUTPUT = answer

TESTER = tools/target/release/tester
TEST_IN = tools/in/0080.txt
TEST_OUT = out.txt

VIS = tools/target/release/vis

TEST_SHELL = ./test.sh
TEST_PY = ./test.py

CXX = g++-7
CFLAGS = --std=c++17 -g -fsanitize=address -Wall

.PHONY: test
test: $(OUTPUT)
#	$(TEST_SHELL)
	python $(TEST_PY)

#.PHONY: out
$(TEST_OUT): $(OUTPUT)
	$(TESTER) $(TEST_IN) ./$(OUTPUT) > $@

.PHONY: svg
svg: $(OUTPUT) $(TEST_OUT)
	$(VIS) $(TEST_IN) $(TEST_OUT)

$(OUTPUT): $(INPUT)
	$(CXX) $< -o $@ $(CFLAGS)

.PHONY: clean
clean:
	rm $(OUTPUT) $(TEST_OUT)
