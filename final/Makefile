CC = gcc
CFLAGS = -Wall -Wextra 
LDFLAGS = -lgcrypt

EXECS = purenc purdec

.PHONY: all clean

all: $(EXECS)

purenc: purenc.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

purdec: purdec.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

clean:
	rm -f $(EXECS)
