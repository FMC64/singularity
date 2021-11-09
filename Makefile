CXXFLAGS = -std=c++20 -Wall -Wextra -O3 -Os -Wno-char-subscripts $(CXXFLAGS_EXTRA) -I src
#SANITIZE = true

ifdef SANITIZE
CXXFLAGS += -g -fsanitize=address -fno-omit-frame-pointer -fsanitize=undefined
endif

OUT = singularity

COMMON_SRC = $(wildcard src/*.cpp)
OUT_SRC = $(wildcard src/nix/*.cpp)
TEST_SRC = $(wildcard test/*.cpp)
TEST_STL_SRC = $(wildcard test/stl/*.cpp)

COMMON_OBJ = $(COMMON_SRC:.cpp=.o)
OUT_OBJ = $(OUT_SRC:.cpp=.o)
TEST_OBJ = $(TEST_SRC:.cpp=.o)
TEST_STL_OBJ = $(TEST_STL_SRC:.cpp=.o)

OUT_ALL_OBJ = $(COMMON_OBJ) $(OUT_OBJ)
TEST_ALL_OBJ = $(COMMON_OBJ) $(TEST_OBJ) $(TEST_STL_OBJ)

TEST_OUT = test.exe	# don't collide with test/

all: $(OUT)

$(OUT): $(OUT_ALL_OBJ)
	$(CXX) $(CXXFLAGS) $(OUT_ALL_OBJ) -o $(OUT)

test_all_obj: CXXFLAGS_EXTRA = -I test
test_all_obj: $(TEST_ALL_OBJ)

$(TEST_OUT): test_all_obj
	$(CXX) $(CXXFLAGS) $(TEST_ALL_OBJ) -o $(TEST_OUT)

test: $(TEST_OUT)

clean:
	rm -f $(COMMON_OBJ) $(OUT_OBJ) $(TEST_OBJ) $(OUT) $(TEST_OUT)

clean_all: clean
	rm -f $(TEST_STL_OBJ)

.PHONY: all clean test_all_obj