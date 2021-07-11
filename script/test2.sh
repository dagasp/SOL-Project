#!/bin/bash

echo "Welcome to Test2"; 
echo -e "4\n400\n10\ncs_sock" > ./test/config.txt #Setta i parametri del file config.txt

SERVER_CMD=./file_storage_server #Eseguibile del server
CLIENT_CMD=./client #Eseguibile del client


valgrind --leak-check=full --show-leak-kinds=all $SERVER_CMD & #Avvia il server in background con valgrind e i flag settati
pid=$!
sleep 2s

#Proverà ad effettuare l'append al file con 10 client diversi
for i in {1..10}; do
    $CLIENT_CMD -f cs_sock* -r pippo -p
done




sleep 2s

kill -s SIGHUP $pid #Chiude il server mandando il segnale SIGHUP
wait $pid