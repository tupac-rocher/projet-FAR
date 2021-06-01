#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <regex.h>
#include <dirent.h>


//Répertoire des fichiers du serveur
char directoryFile[240];

//Descripteur de socket
  //Principale
int dS;
  //Intermédiaire
int dSI;
  //Transfert de fichier client vers serveur
int dSFTCTS;
  //Transfert de fichier serveur vers client
int dSFTSTC;

char port[240];
/*Sémaphore*/
sem_t semaphore;

/*RegEx*/
regex_t regex;
/*Taille des pseudos*/
int pseudoLength;

//Port of the first room socket
int roomSocketStartingPort;
//UTILISATEUR
/*Structure Utilisateur*/
typedef struct Utilisateur Utilisateur;
struct Utilisateur{
    int dSC;
    char pseudo[50];
};
//Tableau Utilisateur
Utilisateur* tabdSC;
int maxIndex; // size-1 / index to insert a new value
pthread_mutex_t lock;

//ROOM
typedef struct Room Room;
struct Room{
    int idRoom;
    char roomName[240];
    char theme[240];
    int dSR;
    int portR;
    int *tab_dSRC;
    int maxIndexdSRRoom;
};
//Tableau de salon
Room* tabRoom;
int maxIndexRoom;
pthread_mutex_t lockTabRoom;

//STRUCTURE
  //Thread file transfer serveur to client
typedef struct file_transfert_server_to_client_struct file_transfert_server_to_client_struct;
struct file_transfert_server_to_client_struct{
  FILE *fps;
  char fileName[240];
  size_t size;
  int dSCFT;
};

  //Thread file transfer client to serveur
typedef struct file_transfert_client_to_serveur_struct file_transfert_client_to_serveur_struct;
struct file_transfert_client_to_serveur_struct{
  char folder[240];
  int dSCFT;
};

  //Thread relay room
typedef struct relay_room_struct relay_room_struct;
struct relay_room_struct{
  long dSR;
  long dSC;
  int idRoom;
};

/*token pas safe-thread*/
char *tok;
/*Mutex*/
pthread_mutex_t tokLock;


/**
* @brief function that closes the serveur
*/

void end();
  
/**
* @brief function that creates a room folder
* @param directoryfile is the chosen directory for file transfer
* @param roomName the name of the room (to name the folder with the name)
*/

void createRoomFolder(char *directoryFile, char *roomName);

/**
* @brief function that modify a room folder
* @param directoryfile is the chosen directory for file transfer
* @param formerRoomName to identify which folder we have to change the name
* @param newRoomName to set the new name to the folder
*/

void modifyRoomFolder(char *directoryFile, char *formerRoomName, char *newRoomName);

/**
* @brief function that deletes a room folder
* @param directoryfile is the chosen directory for file transfer
* @param roomName the name of the room (to delete the right folder)
*/

void deleteRoomFolder(char *directoryFile, char *roomName);

/**
* @brief function sends a list of command to the user
* @param fileName the name of file that contains the list of command
* (a different set of command if we are in the lobby or in a room)
* @param dSC socket descriptor from the client broadcasting the message
*/

void cmd_man(char* fileName, int dSC);


/**
* @brief function that display for the client correspondin to dSC, the rooms available
* @param dSC socket descriptor from the client broadcasting the message
*/

void displayRoomsAvailable(int dSC);

/**
* @brief function that returns 0 if a room exists otherwise 1
* @param message
* @return 0 or 1
*/

int roomExists(char* message);
 
/**
* @brief function that returns the id of the room
* @param message : id or name of the room
* @return the id of the room
*/

int getIDRoom(char* message);
 
/**
* @brief function that returns the dSR of the room
* @param message : id or name of the room
* @return the socket dSR of the room
*/

int getdSRRoom(char* message);


/**
* @brief function that returns the port of the room
* @param message : id or name of the room
* @return the port of the room
*/

int getPortRoom(char* message);

/**
* @brief function that returns the current index
* @param id of the room
* @return the current index
*/

int getCurrentIndexRoom(int id);
 
/**
* @brief function that update the file rooms.txt in adequacy with tab_room
*/

void updateDatabaseRooms();
 
/**
* @brief function that returns the first id available
* @return the first id available
*/

int getFirstUnusedId();

/**
* @brief function that returns the first port for room available
* @return the first port for the room available
*/

int getFirstRoomPortUnused();

/**
* @brief function that checks the nickname
* @param pseudo string that contains the user's nickname
* @return 0 if the pseudo is unregistred, 1 if it's registred
*/

int pseudoUsed(char* pseudo);
 
/**
* @brief 
* @param dSC socket descriptor from the client broadcasting the message,
* @param message -> buffer to stock the pseudo
* @return the pseudo of the user
*/

int getPseudoBydSC(char* message,long dSC);

/**
* @brief suppress the current dSC from the tab and  
* @param dSC socket descriptor from the client broadcasting the message
*/

void cmd_fin(int dSC);
 
/**
* @brief manages file transfer
* @param folder-> folder receiving the file on the server side (default directory ->"")
*/

void cmd_file(char *folder);


/**
* @brief Send the list of connected users to the client
* @param dSC socket descriptor from the client broadcasting the message
*/

void cmd_displayUsers(int dSC);

/**
* @brief Send the file list to the client
* @param dSC socket descriptor from the client broadcasting the message
* @param folder-> on server side, files from this folder are to display (default directory ->"")
*/

void cmd_displayFiles(char *folder,int dSC); 

/**
* @brief Send the file list to the client
* @param dSC socket descriptor from the client broadcasting the message
* @param folder -> folder from where to download the file (default directory ->"")
*/
void cmd_fetch(char *folder,int dSC);




