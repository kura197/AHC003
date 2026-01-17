
DEBUG ?= no

SUBMIT ?= no    ## for AWS lambda

INPUT = answer.cpp
OUTPUT = answer

TESTER = ./test.sh
VIS = tools/target/release/vis
TEST_IN = tools/in/0000.txt

TEST_PY = ./test.py

PERF_OUT = profile.json

TMP_OUT = ./out/tmp.txt

AC_LIB = ${HOME}/.atcoder/ac-library

CXX = g++   # compatible to g++-13
## -fopenmp is removed for AWS lambda
CXXFLAGS = --std=gnu++23 -Wall -fconcepts -g -Wall -Wextra
ifeq ($(DEBUG), yes)
	CXXFLAGS += -O0 -fsanitize=address -fconcepts
else
	CXXFLAGS += -O2
endif

ifeq ($(SUBMIT), yes)
	CXXFLAGS += -DONLINE_JUDGE
endif

.PHONY: test_light
test_light: $(OUTPUT)
	./tools/target/release/tester $(TEST_IN) ./$(OUTPUT) > ./out/tmp.txt

.PHONY: test
test: $(OUTPUT)
	python3 $(TEST_PY)

.PHONY: svg
svg: $(OUTPUT)
	$(TESTER) $(TEST_IN) ./$(OUTPUT) tmp
	eog out.svg

.PHONY: perf
perf: $(OUTPUT)
	samply record -o $(PERF_OUT) ./$(OUTPUT) < $(TEST_IN) > $(TMP_OUT)

.PHONY: load_perf
load_perf:
	samply load $(PERF_OUT)

$(OUTPUT): $(INPUT)
	$(CXX) $^ -o $@ $(CXXFLAGS)

.PHONY: prof
prof: $(OUTPUT) gmon.out
	gprof $(OUTPUT) gmon.out

.PHONY: analysis
analysis:
	streamlit run analysis.py

.PHONY: clean
clean:
	rm $(OUTPUT) 

.PHONY: clean_results
clean_results:
	git clean -fdx ./out/
