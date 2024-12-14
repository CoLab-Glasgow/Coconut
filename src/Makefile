# Compiler settings
CXX = g++
CXXFLAGS = -fPIC -shared -O2 -Wall -Wextra -Wno-unused-variable -g -frtti -rdynamic

# Get GCC plugin and include directories
GCC_PLUGIN_DIR := $(shell $(CXX) -print-file-name=plugin)
GCC_INCLUDE_DIR := $(shell $(CXX) -print-file-name=include)

# Plugin headers
PLUGIN_HEADERS = \
    -I$(GCC_PLUGIN_DIR)/include \
    -I$(GCC_PLUGIN_DIR)/include/gcc \
    -I$(GCC_PLUGIN_DIR)/include/gcc/cp \
    -I$(GCC_PLUGIN_DIR)/include/gcc/c-family

# GCC plugin library
GCC_PLUGIN_LIBS := $(shell $(CXX) -print-file-name=libgcc-plugin.so)

LIBS = -lstdc++  # Link the C++ Standard Library

# Plugin 1: Coconut
PLUGIN_NAME1 = Coconut.so
PLUGIN_SRC1 = Coconut.cpp



# Default target to build all plugins
all: $(PLUGIN_NAME1) $(PLUGIN_NAME2)

# Rule for building Coconut plugin
$(PLUGIN_NAME1): $(PLUGIN_SRC1)
	$(CXX) $(CXXFLAGS) $(PLUGIN_HEADERS) $(PLUGIN_SRC1) -o $@ $(LIBS)

# Rule for building alias_tracker_plugin
$(PLUGIN_NAME2): $(PLUGIN_SRC2)
	$(CXX) $(CXXFLAGS) -I$(GCC_INCLUDE_DIR) $(PLUGIN_HEADERS) $(PLUGIN_SRC2) \
	-o $@ $(LIBS) $(GCC_PLUGIN_LIBS)

# Clean target
clean:
	rm -f $(PLUGIN_NAME1) $(PLUGIN_NAME2)
