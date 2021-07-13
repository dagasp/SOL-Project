#!/bin/bash

echo "Welcome to Test2"; 
echo -e "4\n1000000\n10\ncs_sock\nServerLog.txt" > ./test/config.txt #Setta i parametri del file config.txt

SERVER_CMD=./file_storage_server #Eseguibile del server
CLIENT_CMD=./client #Eseguibile del client


valgrind --leak-check=full --show-leak-kinds=all $SERVER_CMD & #Avvia il server in background con valgrind e i flag settati
pid=$!
sleep 2s

#Prova a scrivere 15 files diversi
for i in {1..15}; do
    $CLIENT_CMD -f cs_sock -w ./files/file$i.txt
done

sleep 2s

kill -s SIGHUP $pid #Chiude il server mandando il segnale SIGHUP
wait $pid
echo "ByeBye!";