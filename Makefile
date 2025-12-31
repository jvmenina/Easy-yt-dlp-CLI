# ----- VARIABLE DECLARATIONS -----

# Compiler and flags
COMPILER = g++
# COMPILERFLAGS = -std=c++17 -Wall -O2
COMPILERFLAGS = -std=c++17 -O2

# Target executable name
TARGET ?= easy_ytdlp_cli
ifeq ($(OS),Windows_NT)
	ifneq ($(suffix $(TARGET)),.exe)
		TARGET := $(TARGET:.exe=).exe
	endif
endif

# Source files
SOURCES = ytdlp-cli.cpp

# Object files
OBJECTS = $(SOURCES:.cpp=.o)

# ----- TARGET DECLARATIONS -----

# Default target
all: $(TARGET)

# Compile source files to object files
%.o: %.cpp
	$(COMPILER) $(COMPILERFLAGS) -c $< -o $@

# Link object files to create executable
$(TARGET): $(OBJECTS)
	$(COMPILER) $(OBJECTS) -o $(TARGET)

# Clean build artifacts
clean:
	ifeq ($(OS),Windows_NT)
		del /Q $(subst /,\,$(OBJECTS)) $(TARGET).exe 2>nul || true
	else
		rm -f $(OBJECTS) $(TARGET)
	endif

# Phony targets
.PHONY: all clean
