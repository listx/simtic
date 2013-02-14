CC = gcc
CFLAGS = -Wall -Wextra
# Optimization
CFLAGS += -O3
# CPU-specific optimization
CFLAGS += -march=native
# Use pipe instead of temporary files b/n various stages of compilation
CFLAGS += -pipe
# Debugging symbols
#CFLAGS += -g -O0
OBJECTS = main.o

simtic : $(OBJECTS)
		$(CC) $(CFLAGS) $(OBJECTS) -o simtic

%.o : %.c
		$(CC) $(CFLAGS) -c $<

clean:
		$(RM) *.o simtic
