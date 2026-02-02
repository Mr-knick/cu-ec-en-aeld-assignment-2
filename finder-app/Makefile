TARGET = writer

# Compiler settings
# If CROSS_COMPILE is provided (e.g., aarch64-none-linux-gnu-), 
# it prefixes gcc. Otherwise, it defaults to native gcc.
CC = $(CROSS_COMPILE)gcc
CFLAGS = -Wall -Werror -g

# Source and Object files
SRCS = writer.c
OBJS = $(SRCS:.c=.o)

# Default target: builds the "writer" application
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Pattern rule for object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) *.o

.PHONY: all clean