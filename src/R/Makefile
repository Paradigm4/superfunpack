CFLAGS=-pedantic -W -Wextra -Wall -Wno-variadic-macros -Wno-strict-aliasing -Wno-long-long -Wno-unused-parameter -fPIC -D_STDC_FORMAT_MACROS -Wno-system-headers -isystem -O2 -g -DNDEBUG -ggdb3  -D_STDC_LIMIT_MACROS -std=c99
INC=-I.

all:
	$(CC) $(CFLAGS) -c stirlerr.c
	$(CC) $(CFLAGS) -c bd0.c
	$(CC) $(CFLAGS) -c dbinom.c
	$(CC) $(CFLAGS) -c dhyper.c

clean:
	rm -f *.o
