#!/bin/bash

echo "Welcome to Test1"; 
echo -e "1\n128000000\n10000\ncs_sock" > ./test/config.txt #Setta i parametri del file config.txt

SERVER_CMD=./file_storage_server #Eseguibile del server
CLIENT_CMD=./client #Eseguibile del client


valgrind --leak-check=full --show-leak-kinds=all $SERVER_CMD & #Avvia il server in background con valgrind e i flag settati
pid=$!
sleep 2s

#Legge pippo con -r e tutti i file con -R con 200ms tra una richiesta e l'altra
$CLIENT_CMD -f cs_sock* -r pippo -R -t 200 -p 

#Legge pippo con -r e 3 file con -R3 con -R con 200ms tra una richiesta e l'altra, salvando i files letti in locale
$CLIENT_CMD -f cs_sock* -r pippo -R3 -d StoredFiles -t 200 -p 

#Legge tutti i file con -R salvando i files letti in locale e richiedendo la stampa delle operazioni effettuate con -p
$CLIENT_CMD -f cs_sock* -R -d GetAllFiles -p 

#Stampa le istruzioni d'uso del client
$CLIENT_CMD -h 

sleep 1s

kill -s SIGHUP $pid #Chiude il server mandando il segnale SIGHUP
wait $pid