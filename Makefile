# Compiler
CXX := g++

# Compiler flags
CXX_FLAGS := -O3 -std=c++20 -Wno-unused-result

# SFML flags
SFML_DIR := .
SFML_FLAGS := -lsfml-graphics -lsfml-window -lsfml-system -lGL -L$(SFML_DIR)/lib

# Include directories
INCLUDES := -I./src -I$(SFML_DIR)/include

# Source files
SRC_FILES := $(wildcard src/*.cpp src/imgui/*.cpp)

# Object files
OBJ_FILES := $(SRC_FILES:.cpp=.o)

# Executable name
OUTPUT := bin/myTerminal

# Default target
all: $(OUTPUT)

# Link the object files to create the executable
$(OUTPUT): $(OBJ_FILES)
	$(CXX) $(OBJ_FILES) -o $(OUTPUT) $(SFML_FLAGS)

# Compile source files into object files
%.o: %.cpp
	$(CXX) $(CXX_FLAGS) $(INCLUDES) -c $< -o $@

# Clean target to remove built files
clean:
	rm -f $(OUTPUT) $(OBJ_FILES)
