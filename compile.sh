gcc -Wall -pthread -o Serveur/serveur Serveur/serveur.c
gcc -Wall -pthread -o Client/client Client/client.c
gcc -Wall -pthread -o Client/clientRoom Client/clientRoom.c

chmod 700 Serveur/serveur.c
chmod 700 Client/client.c
chmod 700 Client/clientRoom.c
