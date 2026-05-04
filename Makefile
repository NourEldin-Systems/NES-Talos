# NES-Talos Makefile
# Works in MSYS2 (ucrt64) / Linux / macOS with C++23-capable compiler.

CXX := g++
CXXFLAGS := -std=c++23 -Wall -Wextra -O3 -DNDEBUG -Iinclude

SRC := src/main.cpp src/parser.cpp
OBJ := $(addprefix build/,$(SRC:.cpp=.o))
BIN := build/nes-talos

.PHONY: all clean

all: $(BIN)

$(BIN): $(OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@

# Ensure build directory AND subdirectory for object file exist
build/%.o: %.cpp | build
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

build:
	mkdir -p build

clean:
	rm -rf build