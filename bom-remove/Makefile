TARGET=pkg-remove.dylib
SOURCES=main.c bom.c
OBJS=$(SOURCES:.c=.o)
CFLAGS=-Wall -Wextra -Werror -pedantic -g3
LIBS=-framework CoreFoundation

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ -dynamiclib $^ $(LIBS)

%.o: %.c
	$(CC) -o $@ -c $^ $(CFLAGS)

clean:
	rm -f $(TARGET) $(OBJS) test

test: test.c $(TARGET)
	$(CC) -o $@ test.c $(CFLAGS) $(TARGET)
