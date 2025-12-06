# ---------------------------------------
# CONFIGURATION BEGIN
# ---------------------------------------

# entry point for the program and target name
C_SRCS = src/main.c
CPP_SRCS = src/memory_cache.cpp

# Object files
C_OBJS = $(C_SRCS:.c=.o)
CPP_OBJS = $(CPP_SRCS:.cpp=.o)

# assignment task file (header files)
HEADERS := include/cache_level.hpp include/main_memory.hpp include/cache.hpp include/request.h include/result.h include/strategy.hpp include/cache_line.hpp

# target name
TARGET := project

# Path to your systemc installation
SCPATH = $(SYSTEMC_HOME)

# Additional flags for the compiler
CXXFLAGS := -std=c++14 -DSC_CXX_STANDARD=14 -I"$(SCPATH)/include" -I"./include" -g
LDFLAGS := -L"$(SCPATH)/lib" -lsystemc -lm

CFLAGS := -std=c17 -I"./include"

# ---------------------------------------
# CONFIGURATION END
# ---------------------------------------

# Determine if clang or gcc is available
CXX := $(shell command -v g++ || command -v clang++)
ifeq ($(strip $(CXX)),)
    $(error Neither clang++ nor g++ is available. Exiting.)
endif

CC := $(shell command -v gcc || command -v clang)
ifeq ($(strip $(CC)),)
    $(error Neither clang nor gcc is available. Exiting.)
endif

# Add rpath except for MacOS
UNAME_S := $(shell uname -s)

ifneq ($(UNAME_S), Darwin)
    CXXFLAGS += -Wl,-rpath="$(SCPATH)/lib"
endif

.DEFAULT_GOAL := project

# Rule to compile .c files to .o files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Rule to compile .cpp files to .o files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Rule to link object files to executable
$(TARGET): $(C_OBJS) $(CPP_OBJS)
	$(CXX) $(CXXFLAGS) $(C_OBJS) $(CPP_OBJS) $(LDFLAGS) -o $(TARGET)

# clean up
clean:
	rm -f $(TARGET)
	rm -rf $(C_OBJS) $(CPP_OBJS)
	rm -rf *.vcd
	find . -name "*.vcd" -type f -delete
	find . -type d -empty -delete

.PHONY: project clean