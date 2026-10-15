CC := gcc
CPPFLAGS := -Isrc
CFLAGS := -std=c23 -Wall -Wextra -Wpedantic
DEBUG_FLAGS := -g -O0
RELEASE_FLAGS := -O2
TARGET := bin/kdlfetch
TEST_TARGET := bin/test_sysinfo
SOURCES := src/main.c src/sysinfo.c

.PHONY: all debug release run test clean

all: debug

debug: $(TARGET)

$(TARGET): $(SOURCES) src/sysinfo.h
	mkdir -p bin
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEBUG_FLAGS) $(SOURCES) -o $@

release: $(SOURCES) src/sysinfo.h
	mkdir -p bin
	$(CC) $(CPPFLAGS) $(CFLAGS) $(RELEASE_FLAGS) $(SOURCES) -o $(TARGET)

$(TEST_TARGET): tests/test_sysinfo.c src/sysinfo.c src/sysinfo.h
	mkdir -p bin
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEBUG_FLAGS) tests/test_sysinfo.c src/sysinfo.c -o $@

run: $(TARGET)
	./$(TARGET)

test: $(TEST_TARGET)
	./$(TEST_TARGET)

clean:
	rm -f $(TARGET) $(TEST_TARGET)
