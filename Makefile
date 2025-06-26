# Makefile for WPANUSB Extended API Mock Tests
# Copyright (c) 2025 BeagleBoard.org Foundation

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2
TARGET = mock_test_extended_api
SRCDIR = test
LOGDIR = test/logs

# Source files
SOURCES = $(SRCDIR)/mock_test_extended_api.c

# Create directories
$(shell mkdir -p $(LOGDIR))

.PHONY: all clean test run-tests

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES)

test: $(TARGET)
	@echo "Running Extended API Mock Tests..."
	@echo "=================================="
	./$(TARGET) | tee $(LOGDIR)/test_results_$(shell date +%Y%m%d_%H%M%S).log
	@echo "Test results saved to $(LOGDIR)/"

run-tests: test

clean:
	rm -f $(TARGET)
	rm -f $(LOGDIR)/*.log

help:
	@echo "Available targets:"
	@echo "  all       - Build the test executable"
	@echo "  test      - Run tests and save logs"
	@echo "  clean     - Remove build artifacts and logs"
	@echo "  help      - Show this help message"
