CC     = cc
CFLAGS = -std=c99 -Wall -Wextra -Wno-unused-parameter -Wno-unused-function
SRCS   = src/main.c src/util.c src/lexer.c src/parser.c \
         src/interpreter.c src/env.c src/value.c src/stdlib.c src/error.c
TARGET = vel

.PHONY: all debug test examples clean install uninstall

all: $(TARGET)

$(TARGET): $(SRCS) src/vel.h
	$(CC) $(CFLAGS) -O2 -o $@ $(SRCS) -lm
	@echo "  Built: ./$(TARGET)"
	@echo "  Run:   ./$(TARGET) run examples/hello.vel"

debug:
	$(CC) $(CFLAGS) -g -fsanitize=address -o $(TARGET)-debug $(SRCS) -lm

# test always recompiles from source — never uses a cached binary
test: clean all
	@bash tests/run_tests.sh

examples: all
	@for f in examples/*.vel; do \
	    echo "=== $$f ==="; \
	    ./$(TARGET) run "$$f"; \
	    echo; \
	done

clean:
	rm -f $(TARGET) $(TARGET)-debug

install: all
	cp $(TARGET) /usr/local/bin/vel
	@echo "  Installed: /usr/local/bin/vel"

uninstall:
	rm -f /usr/local/bin/vel
