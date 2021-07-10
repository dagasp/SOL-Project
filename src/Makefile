CC		=  gcc
AR              =  ar
CFLAGS	        += -std=gnu99 -Wall -Werror -g
ARFLAGS         =  rvs
INCDIR          = ../headers
INCLUDES	= -I. -I $(INCDIR)
LDFLAGS 	= -L.
OPTFLAGS	= #-O3 
LIBS            = -lpthread

# aggiungere qui altri targets
TARGETS		= file_storage_server client

.PHONY: all clean cleanall
.SUFFIXES: .c .h

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) $(OPTFLAGS) -c -o $@ $<

all		: $(TARGETS)

file_storage_server: file_storage_server.o util.o libPool.a libStruct.a libApi.a
	$(CC) $(CFLAGS) $(INCLUDES) $(OPTFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)

client: client.o util.o libStruct.a libApi.a
	$(CC) $(CFLAGS) $(INCLUDES) $(OPTFLAGS) -o $@ $^

#Libreria threadpool
libPool.a: threadpool.o ../headers/threadpool.h
	$(AR) $(ARFLAGS) $@ $^

#Libreria strutture dati
libStruct.a: hash.o ../headers/hash.h queue.o ../headers/queue.h list.o ../headers/list.h
	$(AR) $(ARFLAGS) $@ $^

#Libreria API
libApi.a: api.o ../headers/api.h
	$(AR) $(ARFLAGS) $@ $^

test1: client file_storage_server

test2: client file_storage_server

file_storage_server.o: file_storage_server.c 

client.o: client.c

util.o: util.c

hash.o: hash.c

queue.o: queue.c

list.o: list.c

api.o: api.c

threadpool.o: threadpool.c 

clean		: 
	rm -f $(TARGETS)
cleanall	: clean
	\rm -f *.o *~ *.a ./cs_sock src/*.o src/*~