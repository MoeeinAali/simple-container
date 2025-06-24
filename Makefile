CC = gcc
CFLAGS = -Wall -Wextra -g -I./include -D_GNU_SOURCE
LDFLAGS = -lbpf -lelf

BUILD_DIR = build
SRC_DIR = src
INCLUDE_DIR = include


SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJECTS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SOURCES))
TARGET = simplecontainer


EXAMPLES_DIR = examples
HELLO_WORLD_SRC = $(EXAMPLES_DIR)/hello_world.c
RESOURCE_TEST_SRC = $(EXAMPLES_DIR)/resource_test.c
HELLO_TARGET = $(EXAMPLES_DIR)/hello
RESOURCE_TEST_TARGET = $(EXAMPLES_DIR)/resource_test


$(shell mkdir -p $(BUILD_DIR))
$(shell mkdir -p $(EXAMPLES_DIR))


all: $(TARGET) examples


$(TARGET): $(OBJECTS)
	@echo "Linking $@..."
	@$(CC) -o $@ $^ $(LDFLAGS)
	@echo "Build completed successfully!"


$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@echo "Compiling $<..."
	@$(CC) $(CFLAGS) -c -o $@ $<


examples: $(HELLO_TARGET) $(RESOURCE_TEST_TARGET)


$(HELLO_TARGET): $(HELLO_WORLD_SRC)
	@echo "Building example $@..."
	@$(CC) $(CFLAGS) -o $@ $<


$(RESOURCE_TEST_TARGET): $(RESOURCE_TEST_SRC)
	@echo "Building example $@..."
	@$(CC) $(CFLAGS) -o $@ $< -lm


install: $(TARGET)
	@echo "Installing SimpleContainer..."
	@sudo mkdir -p /var/lib/simplecontainer/rootfs/base
	@sudo mkdir -p /var/lib/simplecontainer/overlays
	@sudo mkdir -p /var/lib/simplecontainer/logs
	@sudo mkdir -p /var/lib/simplecontainer/images
	@sudo mkdir -p /var/lib/simplecontainer/containers
	@sudo cp $(TARGET) /usr/local/bin/
	@sudo chmod +x /usr/local/bin/$(TARGET)
	@echo "Installation completed."


setup-dirs:
	@echo "Setting up runtime directories..."
	@sudo mkdir -p /var/lib/simplecontainer/rootfs/base
	@sudo mkdir -p /var/lib/simplecontainer/overlays
	@sudo mkdir -p /var/lib/simplecontainer/logs
	@sudo mkdir -p /var/lib/simplecontainer/images
	@sudo mkdir -p /var/lib/simplecontainer/containers
	@sudo mkdir -p /sys/fs/cgroup/simplecontainer 2>/dev/null || true
	@echo "Runtime directories created."


clean:
	@echo "Cleaning up..."
	@rm -f $(BUILD_DIR)/*.o
	@rm -f $(TARGET)
	@rm -f $(HELLO_TARGET)
	@rm -f $(RESOURCE_TEST_TARGET)
	@echo "Clean completed."


distclean: clean
	@echo "Performing full cleanup..."
	@rm -rf $(BUILD_DIR)
	@echo "Full cleanup completed."


test: $(TARGET) examples setup-dirs
	@echo "Running tests..."
	@echo "Testing hello world example:"
	@./$(HELLO_TARGET)
	@echo ""
	@echo "Testing resource limits (requires root):"
	@if [ "$$EUID" -eq 0 ]; then \
		echo "Running memory test..."; \
		sudo ./$(TARGET) run --name memory_test --memory 50M ./$(RESOURCE_TEST_TARGET) --memory-test || true; \
		echo "Running CPU test..."; \
		sudo ./$(TARGET) run --name cpu_test --cpu 0 ./$(RESOURCE_TEST_TARGET) --cpu-test || true; \
		echo "Running I/O test..."; \
		sudo ./$(TARGET) run --name io_test --io-weight 50 ./$(RESOURCE_TEST_TARGET) --io-test || true; \
	else \
		echo "Root privileges required for container tests. Run 'sudo make test'"; \
	fi
	@echo "Tests completed."


test-quick: examples
	@echo "Running quick tests (no root required)..."
	@./$(HELLO_TARGET)
	@./$(RESOURCE_TEST_TARGET) --cpu-test
	@echo "Quick tests completed."


demo: $(TARGET) examples setup-dirs
	@echo "SimpleContainer Demo"
	@echo "==================="
	@echo ""
	@echo "1. Hello World Example:"
	@./$(HELLO_TARGET)
	@echo ""
	@echo "2. Container Tests (requires sudo):"
	@if [ "$$EUID" -eq 0 ]; then \
		echo "Running container with memory limit..."; \
		sudo ./$(TARGET) run --name demo1 --memory 100M ./$(HELLO_TARGET); \
		echo ""; \
		echo "Running container with CPU affinity..."; \
		sudo ./$(TARGET) run --name demo2 --cpu 0 ./$(HELLO_TARGET); \
		echo ""; \
		echo "Listing containers..."; \
		sudo ./$(TARGET) list; \
	else \
		echo "Please run 'sudo make demo' for full demonstration"; \
	fi


help:
	@echo "SimpleContainer Build System"
	@echo "============================"
	@echo ""
	@echo "Available targets:"
	@echo "  all          - Build main program and examples (default)"
	@echo "  $(TARGET)    - Build main SimpleContainer program"
	@echo "  examples     - Build example programs"
	@echo "  install      - Install SimpleContainer system-wide"
	@echo "  setup-dirs   - Create runtime directories"
	@echo "  test         - Run all tests (requires root)"
	@echo "  test-quick   - Run quick tests (no root required)"
	@echo "  demo         - Run demonstration"
	@echo "  clean        - Remove build artifacts"
	@echo "  distclean    - Full cleanup"
	@echo "  help         - Show this help"
	@echo ""
	@echo "Usage examples:"
	@echo "  make                    
	@echo "  make clean && make      
	@echo "  sudo make install       
	@echo "  sudo make test          
	@echo "  sudo make demo          


debug: CFLAGS += -DDEBUG -O0
debug: $(TARGET)


release: CFLAGS += -O2 -DNDEBUG
release: clean $(TARGET)


check:
	@echo "Running syntax checks..."
	@for file in $(SRC_DIR)/*.c; do \
		echo "Checking $$file..."; \
		$(CC) $(CFLAGS) -fsyntax-only $$file; \
	done
	@echo "Syntax check completed."


format:
	@echo "Formatting code..."
	@which clang-format >/dev/null 2>&1 && \
		find $(SRC_DIR) $(INCLUDE_DIR) -name "*.c" -o -name "*.h" | xargs clang-format -i || \
		echo "clang-format not found, skipping format"

.PHONY: all examples install setup-dirs clean distclean test test-quick demo help debug release check format