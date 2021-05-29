
INPUT = answer.cpp
OUTPUT = answer

TESTER = tools/target/release/tester
VIS = tools/target/release/vis
TEST_IN = tools/in/0072.txt

### for test (Q == 10000)
#TESTER = tools/target10000/release/tester
#VIS = tools/target10000/release/vis
#TEST_IN = tools/in10000/0025.txt

#TEST_IN = tools/in_e1/0025.txt

TEST_OUT = out.txt


TEST_SHELL = ./test.sh
TEST_PY = ./test.py

CXX = g++-7
CFLAGS = --std=c++17 -Wall -O3
#CFLAGS = --std=c++17 -g -fsanitize=address -Wall
#CFLAGS = --std=c++17 -g -fsanitize=address -Wall -p

.PHONY: test
test: $(OUTPUT)
#	$(TEST_SHELL)
	python $(TEST_PY)

.PHONY: $(TEST_OUT)
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
