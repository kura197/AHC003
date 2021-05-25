
INPUT = answer.cpp
OUTPUT = answer

TESTER = tools/target/release/tester
TEST_IN = tools/in/0025.txt
VIS = tools/target/release/vis

### for test (Q == 10000)
#TESTER = tools/target10000/release/tester
#TEST_IN = tools/in10000/0025.txt
#VIS = tools/target10000/release/vis

TEST_OUT = out.txt


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
	eog out.svg

$(OUTPUT): $(INPUT)
	$(CXX) $< -o $@ $(CFLAGS)

.PHONY: clean
clean:
	rm $(OUTPUT) $(TEST_OUT)
