#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <limits.h>
#include <dirent.h>
#include <regex.h>

//STRUCTURE
  //Thread file transfer client to server
typedef struct file_transfert_client_to_serveur_struct file_transfert_client_to_serveur_struct;
struct file_transfert_client_to_serveur_struct{
  FILE *fps;
  char fileName[240];
  size_t size;
};


/**
* @brief function to finish the discussion and to send the message "/fin" to the server
*/

void end();

/**
* part of the code of the teacher's example
*/

int get_last_tty();

/**
* @brief function to open a new control terminal 
* @return a file corresponding to a control terminal 
*/

FILE* new_tty();

/**
* @brief function which ask to a user wich file to display, initialize the structure and manages the file transfer with a thread
*/

void cmd_file();



