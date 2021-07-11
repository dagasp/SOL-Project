CC		=  gcc
AR              =  ar
CFLAGS	        += -std=gnu99 -Wall -Werror -g
ARFLAGS         =  rvs
INCDIR          = ./headers
INCLUDES	= -I. -I $(INCDIR)
LDFLAGS 	= -L.
OPTFLAGS	= #-O3 
LIBS            = -lpthread
OBJ_DIR		= ./obj_files/
SRC_DIR	    = ./src/
LIB_DIR     = ./libs/
SCRIPT_DIR  = ./script/


TARGETS		= file_storage_server client

.PHONY: all clean cleanall
.SUFFIXES: .c .h

$(OBJ_DIR)%.o: $(SRC_DIR)%.c
	$(CC) $(CFLAGS) $(INCLUDES) $(OPTFLAGS) -c -o $@ $<

all		: $(TARGETS)

file_storage_server: $(OBJ_DIR)file_storage_server.o $(OBJ_DIR)util.o $(LIB_DIR)libPool.a $(LIB_DIR)libStruct.a $(LIB_DIR)libApi.a
	$(CC) $(CFLAGS) $(INCLUDES) $(OPTFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)

client: $(OBJ_DIR)client.o $(OBJ_DIR)util.o $(LIB_DIR)libStruct.a $(LIB_DIR)libApi.a
	$(CC) $(CFLAGS) $(INCLUDES) $(OPTFLAGS) -o $@ $^

#Libreria threadpool
$(LIB_DIR)libPool.a: $(OBJ_DIR)threadpool.o 
	$(AR) $(ARFLAGS) $@ $^

#Libreria strutture dati
$(LIB_DIR)libStruct.a: $(OBJ_DIR)hash.o $(OBJ_DIR)queue.o $(OBJ_DIR)list.o
	$(AR) $(ARFLAGS) $@ $^

#Libreria API
$(LIB_DIR)libApi.a: $(OBJ_DIR)api.o ./headers/api.h
	$(AR) $(ARFLAGS) $@ $^

#Script bash dei test
test1: client file_storage_server
	$(SCRIPT_DIR)test1.sh
test2: client file_storage_server
	$(SCRIPT_DIR)test2.sh

#Make dei file oggetto
$(OBJ_DIR)file_storage_server.o: $(SRC_DIR)file_storage_server.c 

$(OBJ_DIR)client.o: $(SRC_DIR)client.c

$(OBJ_DIR)util.o: $(SRC_DIR)util.c

$(OBJ_DIR)hash.o: $(SRC_DIR)hash.c

$(OBJ_DIR)queue.o: $(SRC_DIR)queue.c

$(OBJ_DIR)list.o: $(SRC_DIR)list.c

$(OBJ_DIR)api.o: $(SRC_DIR)api.c

$(OBJ_DIR)threadpool.o: $(SRC_DIR)threadpool.c 

#Clean per eliminare taregets, file oggetto e librerie
clean		: 
	rm -f $(TARGETS)
cleanall	: clean
	\rm -f *~ $(LIB_DIR)*.a ./cs_sock $(OBJ_DIR)*.o $(SRC_DIR)*~