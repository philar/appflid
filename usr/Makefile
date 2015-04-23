CC = gcc
LD = ld

INCDIRS += -Iinclude -I../include
# TODO this is too specific for our setup. Remove it for the release.
INCDIRS += -I/usr/include/x86_64-linux-gnu
#LDLIBS += -lpthread
CFLAGS += -Wall -g ${INCDIRS}
#CFLAGS += -DDEBUG


PROGS =appflid
OBJS += appflid.o 
#OBJS += str_utils.o 
OBJS += nl_ucmd.o 



appflid: $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDLIBS)

appflid.o: appflid.c
	$(CC) -c $(CFLAGS) $< -o $@
#str_utils.o: str_utils.c
#	$(CC) -c $(CFLAGS) $< -o $@
nl_ucmd.o: nl_ucmd.c
	$(CC) -c $(CFLAGS) $< -o $@


all: appflid

clean:
	rm -f $(PROGS) $(OBJS)
