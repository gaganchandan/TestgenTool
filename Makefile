# TestGen Makefile
# Specification-Based API Test Generation Tool

# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -I. -Isee -Itester -Ispecs \
           -Wno-delete-non-abstract-non-virtual-dtor \
           -Wno-unqualified-std-cast-call \
           -Wno-reorder-ctor \
           -Wno-deprecated-declarations \
           -Wno-unused-variable \
           -Wno-unused-function \
           -Wno-unused-private-field

# Platform-specific paths (adjust as needed)
# macOS (Homebrew)
# INCLUDES = -I/opt/homebrew/include
# LDFLAGS = -L/opt/homebrew/lib

# Linux (uncomment if using Linux)
INCLUDES = -I/usr/include
LDFLAGS = -L/usr/lib

# Libraries
LIBS = -lz3 -lcurl

# Output binary
TARGET = test_libapplication

# Source files
SRCS = test/test_libapplication.cpp \
       tester/algo.cpp \
       language/ast.cc \
       language/visitor.cc \
       language/printer.cc \
	   language/typechecker.cc \
       language/rewrite_globals_visitor.cc \
       language/symvar.cc \
       language/env.cc \
       language/typemap.cc \
       specs/RestaurantSpec.cpp \
       specs/EcommerceSpec.cpp \
       specs/LibrarySpec.cpp \
       see/see.cc \
       see/solver.cc \
       see/z3solver.cc \
       see/functionfactory.cc \
       see/httpclient.cc \
       see/restaurantfunctionfactory.cc \
       see/ecommercefunctionfactory.cc \
       see/libraryfunctionfactory.cc \
       tester/tester.cc

# Default target
all: $(TARGET)

# Build the executable
$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SRCS) $(LDFLAGS) $(LIBS) -o $(TARGET)

# Build and run
run: $(TARGET)
	./$(TARGET)

# Clean build artifacts
clean:
	rm -f $(TARGET)

# Rebuild from scratch
rebuild: clean all

# Help
help:
	@echo "TestGen Makefile"
	@echo ""
	@echo "Usage:"
	@echo "  make          - Build the project"
	@echo "  make run      - Build and run the project"
	@echo "  make clean    - Remove build artifacts"
	@echo "  make rebuild  - Clean and rebuild"
	@echo "  make help     - Show this help message"

.PHONY: all run clean rebuild help
