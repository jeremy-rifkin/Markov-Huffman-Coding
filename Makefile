# based off of https://spin.atomicobject.com/2016/08/26/makefile-c-projects/
BUILD_DIR = bin/obj
SRC_DIRS = src
ifeq ($(OS),Windows_NT)
    TARGET_BINARY = bin/markovhuffman.exe
    PY = python
else
    TARGET_BINARY = markovhuffman
    PY = python3
endif

SRCS = $(shell find $(SRC_DIRS) -name '*.cpp' -or -name '*.c')
OBJS = $(SRCS:%=$(BUILD_DIR)/%.o)
DEPENDENCIES = $(OBJS:.o=.d)

CPP = g++
CC = gcc
CCFLAGS = -MMD -MP -s -O3 -funroll-loops -Wall -Wno-sign-compare -Wno-parentheses
#CCFLAGS = -MMD -MP -g -Og -DMEMORY_DEBUG -Wall -Wno-sign-compare -Wno-parentheses
CPPFLAGS = $(CCFLAGS)

MKDIR_P ?= mkdir -p

$(TARGET_BINARY): $(OBJS)
	$(CPP) $(OBJS) -o $@ $(LDFLAGS)

# c source
$(BUILD_DIR)/%.c.o: %.c
	$(MKDIR_P) $(dir $@)
	$(CC) $(CCFLAGS) -c $< -o $@

# c++ source
$(BUILD_DIR)/%.cpp.o: %.cpp
	$(MKDIR_P) $(dir $@)
	$(CPP) $(CPPFLAGS) -c $< -o $@

.PHONY: clean test remake

clean:
	$(RM) -r $(BUILD_DIR) $(TARGET_BINARY)

remake: clean
	$(MAKE) $(MAKEFLAGS)

test:
	$(PY) test/main.py

-include $(DEPENDENCIES)
