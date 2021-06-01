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
#include <sys/types.h>
#include <sys/stat.h>
#include "serveur.h"
#define BUFFER_SIZE_FILE_TRANSFER 1024

void end(int n){
  pthread_mutex_lock(&lockTabRoom);
  for(int i = 0; i <maxIndexRoom; i++){
    shutdown(tabRoom[i].dSR,2);
  }
  pthread_mutex_unlock(&lockTabRoom);
  pthread_mutex_lock(&lock);
  for(int i = 0; i <maxIndex; i++){
    shutdown(tabdSC[i].dSC,2);
  }
  pthread_mutex_unlock(&lock);
  shutdown(dS,2);
  shutdown(dSI,2);
  shutdown(dSFTCTS,2);
  shutdown(dSFTSTC,2);
  printf("\nTERMINAISON SERVEUR\n");
  exit(-1);
}

//FUNCTION ABOUT ROOMS------------------------------

//ABOUT ROOM FOLDERS
void createRoomFolder(char *directoryFile, char *roomName){
    char command[240];
    strcpy(command,directoryFile);
    strcat(command,"/");
    strcat(command,roomName);
    rmdir(command);
    strcpy(command,directoryFile);
    strcat(command,"/");
    strcat(command,roomName);
    mkdir(command,0700);
}

void modifyRoomFolder(char *directoryFile, char *formerRoomName, char *newRoomName){
    char command[240];
    strcpy(command,"mv ");
    strcat(command,directoryFile);
    strcat(command,"/");
    strcat(command,formerRoomName);
    strcat(command," ");
    strcat(command,directoryFile);
    strcat(command,"/");
    strcat(command,newRoomName);
    system(command);
}

void deleteRoomFolder(char *directoryFile, char *roomName){
    char command[240];
    strcpy(command,directoryFile);
    strcat(command,"/");
    strcat(command,roomName);
    rmdir(command);
}

//FUNCTION THAT DISPLAY FOR THE CLIENT CORRESPONDING TO dSC, THE ROOMS AVAILABLE 
void displayRoomsAvailable(int dSC){
  char messageChoice[240] = "[+]Voici les salons actuellement disponibles (Identifiant/Nom/Thème)\n";
  if(send(dSC,messageChoice,sizeof(messageChoice),0) == -1){
    perror("Erreur d'envoi du message du choix des salons");
  }
  pthread_mutex_lock(&lockTabRoom);
  for(int i = 0 ; i <maxIndexRoom ; i++){
    char currentRoomMsg[240];
    sprintf(currentRoomMsg,"%d - ",tabRoom[i].idRoom);
    strcat(currentRoomMsg,tabRoom[i].roomName);
    strcat(currentRoomMsg," - ");
    strcat(currentRoomMsg,tabRoom[i].theme);
    strcat(currentRoomMsg,"\n");
    if(send(dSC,currentRoomMsg,sizeof(currentRoomMsg),0) == -1){
      perror("Erreur d'envoi d'un choix de la liste des salons");
    }
    currentRoomMsg[0] = '\0';
  }
  pthread_mutex_unlock(&lockTabRoom);
}

//FUNCTION THAT RETURNS 0 IF A ROOM EXISTS OTHERWISE 1
int roomExists(char* message){
  int exists = 1;
  pthread_mutex_lock(&lockTabRoom);
  for(int i = 0; i < maxIndexRoom ; i++){
    if(tabRoom[i].idRoom == atoi(message) || strcmp(tabRoom[i].roomName,message) == 0){
      exists = 0;
    }
  }
  pthread_mutex_unlock(&lockTabRoom);
  return exists;
}

//FUNCIOTN THAT RETURNS THE ID OF THE ROOM
int getIDRoom(char* message){
  int idRoom = -1;
  pthread_mutex_lock(&lockTabRoom);
  for(int i = 0; i < maxIndexRoom ; i++){
    if(tabRoom[i].idRoom == atoi(message) || strcmp(tabRoom[i].roomName,message) == 0){
      idRoom = tabRoom[i].idRoom;
    }
  }
  pthread_mutex_unlock(&lockTabRoom);
  return idRoom;
}

//FUNCTION THAT RETURNS THE DSR OF THE ROOM
int getdSRRoom(char* message){
  int dSR = -1;
  pthread_mutex_lock(&lockTabRoom);
  for(int i = 0; i < maxIndexRoom ; i++){
    if(tabRoom[i].idRoom == atoi(message) || strcmp(tabRoom[i].roomName,message) == 0){
      dSR = tabRoom[i].dSR;
    }
  }
  pthread_mutex_unlock(&lockTabRoom);
  return dSR;
}

//FUNCTION THAT RETURNS THE PORT OF THE ROOM
int getPortRoom(char* message){
  int portR = -1;
  pthread_mutex_lock(&lockTabRoom);
  for(int i = 0; i < maxIndexRoom ; i++){
    if(tabRoom[i].idRoom == atoi(message) || strcmp(tabRoom[i].roomName,message) == 0){
      portR = tabRoom[i].portR;
    }
  }
  pthread_mutex_unlock(&lockTabRoom);
  return portR;
}

//FUNCTION THAT RETURNS THE CURRENT INDEX
int getCurrentIndexRoom(int id){
  int index = -1;
  pthread_mutex_lock(&lockTabRoom);
  for(int i = 0; i < maxIndexRoom ; i++){
    if(id == tabRoom[i].idRoom){
      index = i;
    }
  }
  pthread_mutex_unlock(&lockTabRoom);
  return index;
}

//FUNCTION THAT UPDATE THE FILE rooms.txt IN ADEQUACY WITH TAB_ROOM
void updateDatabaseRooms(){
  FILE* fp = fopen("utiles/rooms.txt","w");
  if(!fp){
    printf("Can't open file rooms.txt\n");
    exit(-1);
  }
  pthread_mutex_lock(&lockTabRoom);
  for(int i = 0; i < maxIndexRoom; i++){
    char roomName[240];
    char theme[240];
    strcpy(roomName,tabRoom[i].roomName);
    strcpy(theme,tabRoom[i].theme);
    fprintf(fp, "%s,", roomName);
    fprintf(fp, "%s\n", theme);
  }
  pthread_mutex_unlock(&lockTabRoom);
  if(fclose(fp) != 0){
    perror("Erreur lors de la fermeture du fichier");
  }
}
//FUNCTION THAT RETURNS THE FIRST ID AVAILABLE

int getFirstUnusedId(){
  int id = 1;
  int isUsed = 0;
  pthread_mutex_lock(&lockTabRoom);
  while (isUsed == 0){
    isUsed = 1;
    for(int i = 0; i < maxIndexRoom; i++){
      if(tabRoom[i].idRoom == id){
        isUsed = 0;
      }
    }
    if(isUsed == 0){
      id = id + 1;
    }
  }
  pthread_mutex_unlock(&lockTabRoom);
  return id;
}

//FUNCTION THAT RETURNS THE FIRST PORT FOR ROOM AVAILABLE

int getFirstRoomPortUnused(){
  /*PROTEGER LA VARIABLE roomSocketStartingPort PAR UN MUTEX ?*/
  pthread_mutex_lock(&lockTabRoom);
  int portR = roomSocketStartingPort;
  int isUsed = 0;
  while (isUsed == 0){
    isUsed = 1;
    for(int i = 0; i < maxIndexRoom; i++){
      if(tabRoom[i].portR == portR){
        isUsed = 0;
      }
    }
    if(isUsed == 0){
      portR++;
    }
  }
  pthread_mutex_unlock(&lockTabRoom);
  return portR;
}

void cmd_man(char* fileName, int dSC){
 char messageMan[240] = "[+]Voici les listes des commandes disponibles sur l'application :\n";
      if(send(dSC,messageMan,sizeof(messageMan),0) == -1){
        perror("Erreur d'envoi du message du man");
      }
      /*Envoi de la liste des commandes disponibles*/
      FILE* manFile;
      char path[240];
      strcpy(path,"utiles/");
      strcat(path,fileName);
      char c;
      char currentLine[240];
      currentLine[0] = '\0';
      manFile = fopen(path, "r");
      if(manFile){
        while((c = getc(manFile)) != EOF){
          char character[2];
          sprintf(character,"%c",c);
          strcat(currentLine, character);
          if(c == '\n'){
            if(send(dSC, currentLine, sizeof(currentLine),0) == -1){
                perror("Problème lors de l'envoi de la ligne");
            }
            currentLine[0] = '\0';
          }

        }
            fclose(manFile);
      } else{
        perror("Erreur lors de l'ouverture du fichier");
      }
}


// THREAD FILE TRANSFER SERVER TO CLIENT

void* thread_file_transfer_server_to_client(void *args){
  file_transfert_server_to_client_struct *currentArgs = (file_transfert_server_to_client_struct *) args;
  
  //Envoi du nom
  char fileName[240];
  strcpy(fileName,currentArgs->fileName);
  int dSCFT = currentArgs->dSCFT;
  if(send(dSCFT,fileName,sizeof(fileName),0) == -1){
    perror("Problème lors de l'envoi du nom du fichier");
  }

  //Envoi de la taille
  size_t size = currentArgs->size;
  if(send(dSCFT,&size,sizeof(size),0) == -1){
    perror("Problème lors de l'envoi de la taille du fichier");
  }


  FILE *fps = currentArgs->fps;

  char buffer[BUFFER_SIZE_FILE_TRANSFER];
  while(!feof(fps)){
    int numberOfBlocRead = fread(buffer, BUFFER_SIZE_FILE_TRANSFER,1,fps);
    if(numberOfBlocRead == -1){
      perror("Problème fread");
    }
    if (send(dSCFT, buffer, sizeof(buffer), 0) == -1) {
      perror("[-]Error in sending file.");
      exit(1);
    }
    bzero(buffer, BUFFER_SIZE_FILE_TRANSFER);
  }
  if(fclose(fps) != 0){
    perror("Erreur lors de la fermeture du fichier");
  }
  printf("--Fichier envoyé--\n");
  shutdown(dSCFT,2);
  pthread_exit(NULL);
}

// THREAD FILE TRANSFER CLIENT TO SERVER

void* thread_file_transfer_client_to_server(void *args){
  file_transfert_client_to_serveur_struct *currentArgs = (file_transfert_client_to_serveur_struct *) args;
  
  char folder[240];
  strcpy(folder,currentArgs->folder);
  int dSCFT = currentArgs->dSCFT;
  strcpy(folder,(char *) args);
  //args correspond au dossier
  
  //Réception du nom du fichier
  char fileName[240];
  if(recv(dSCFT,fileName,sizeof(fileName),0) == -1){
    perror("Problème lors de la réception du nom du fichier");
  }

  //Réception de la taille du fichier
  size_t fileSize;
  if(recv(dSCFT,&fileSize,sizeof(fileSize),0) == -1){
    perror("Problème lors de la réception de la taille du fichier");
  }

  FILE *fp;
  char path[240];
  strcpy(path,directoryFile);
  strcat(path,"/");
  if(strcmp(folder,"") != 0){
    strcat(path,folder);
    strcat(path,"/");
  }
  strcat(path,fileName);
  fp = fopen(path,"wb+");
  char buffer[BUFFER_SIZE_FILE_TRANSFER];
  size_t currentSize = 0;
  while (currentSize < fileSize) {
    int n = recv(dSCFT, buffer, BUFFER_SIZE_FILE_TRANSFER, MSG_WAITALL);
    //PROBLEME ICI
    if(n != BUFFER_SIZE_FILE_TRANSFER){
      printf("RECV %d\n",n);
    }
    if(n == -1){
      perror("Une erreur est survenue pendant le transfert du fichier");
    }
    if(currentSize+BUFFER_SIZE_FILE_TRANSFER > fileSize){
      if(fwrite(buffer,fileSize-currentSize,1,fp) == -1){
        perror("Problème fwrite");
      }
    }
    else{
      if(fwrite(buffer, BUFFER_SIZE_FILE_TRANSFER,1,fp) == -1){
        perror("Problème fwrite");
      }
    }
    bzero(buffer, BUFFER_SIZE_FILE_TRANSFER);
    fseek(fp,0,SEEK_END);
    currentSize = ftell(fp);

  }
  printf("--Fichier reçu--\n");
  fclose(fp);
  shutdown(dSCFT, 2);
  pthread_exit(NULL);
  
}

//VERIFICATION DU PSEUDO, 0 -> unregistered, 1 -> registered

int pseudoUsed(char* pseudo){
  int registered = 0;
  pthread_mutex_lock(&lock);
  for(int i = 0; i <maxIndex; i++){
    if(strcmp(tabdSC[i].pseudo, pseudo) == 0){
      registered = 1;
    }
  }
  pthread_mutex_unlock(&lock);
  return registered;
}

int getPseudoBydSC(char* message,long dSC){
  int erreur = -1;
  pthread_mutex_lock(&lock);
  for (int i = 0; i < maxIndex; ++i){
    if(tabdSC[i].dSC == dSC){
      strcpy(message,tabdSC[i].pseudo);
      erreur = 0;
    }
  }
  pthread_mutex_unlock(&lock);
  return erreur;
}

//-------------------------------------------------COMMAND SECTION--------------------------------

//-----------------------------------COMMAND FIN-----------------------------------

void cmd_fin(int dSC){
  char pseudo[240];
  // Suppress the current dSC form the tab
  int indexOfTheCurrentClient = -1;
  pthread_mutex_lock(&lock);
  for(int i = 0; i < maxIndex; i++){
    if(tabdSC[i].dSC == dSC){
      indexOfTheCurrentClient = i;
    }
  }
  // COPIE DU PSEUDO
  strcpy(pseudo,tabdSC[indexOfTheCurrentClient].pseudo);

  tabdSC[indexOfTheCurrentClient] = tabdSC[maxIndex-1];
  maxIndex = maxIndex-1;
  tabdSC = (Utilisateur*) realloc(tabdSC,(maxIndex+1)*sizeof(Utilisateur));

  char leavingMessageToAll[240];
  strcpy(leavingMessageToAll,pseudo);
  strcat(leavingMessageToAll," a quitté le serveur!\n");
  for(int i = 0; i < maxIndex; i++){
    // Envoi
    if(tabdSC[i].dSC != dSC){
      int sendC = send(tabdSC[i].dSC, leavingMessageToAll, sizeof(leavingMessageToAll), 0);
      if (sendC == -1){
        perror("send()");
        exit(0);
      }
    }
  }
  pthread_mutex_unlock(&lock);
  printf("%s a quitté le serveur\n",pseudo);
  int sem = sem_post(&semaphore);
  if(sem == -1){
    perror("A problem occured (sem init)");
  }
  shutdown(dSC, 2);
  pthread_exit(NULL);
}

//----------------------------------COMMAND FILE------------------------------------------------------

void cmd_file(char *folder){
  struct sockaddr_in aC ;
  socklen_t lg = sizeof(struct sockaddr_in);

  long dSCFT = accept(dSFTCTS, (struct sockaddr*) &aC,&lg);
  //Faire la gestion d'erreur
  if(dSCFT == -1){
    perror("Problème lors de l'acceptation d'une connexion");
  }
  pthread_t fileTransfer;
  file_transfert_client_to_serveur_struct *args = (file_transfert_client_to_serveur_struct*) malloc(sizeof(file_transfert_client_to_serveur_struct));
  
  args->dSCFT = dSCFT;
  strcpy(args->folder,folder);
  int thread_file = pthread_create(&fileTransfer,NULL,thread_file_transfer_client_to_server,(void*) args);
  if(thread_file != 0){
    perror("Erreur thread transfert de fichier");
    exit(-1);
  }
}

//------------------------------COMMAND DISPLAY USERS (CONNECTED TO THE SERVER)-------------------------

void cmd_displayUsers(int dSC){
  /*Envoi de la liste des utilisateurs connectés*/
  pthread_mutex_lock(&lock);
  char currentPseudo[240];
  for(int i = 0; i <maxIndex; i++){
    strcpy(currentPseudo," - ");
    strcat(currentPseudo, tabdSC[i].pseudo);
    strcat(currentPseudo,"\n");
    if(send(dSC,currentPseudo, sizeof(currentPseudo),0) == -1){
          perror("Problème lors de l'envoi du pseudo");
        }
  }
  pthread_mutex_unlock(&lock);
}

//-----------------------------COMMAND DISPLAY FILES (FROM SERVER)------------------------------------------

void cmd_displayFiles(char *folder,int dSC){
  /*Envoyez la liste des fichiers*/
  DIR *d;
  struct dirent *dir;
  char directory[240];
  char currentFileName[240];
  strcpy(directory,directoryFile);
  strcat(directory,"/");
  if(strcmp(folder,"") != 0){
    strcat(directory,folder);
    strcat(directory,"/");
  }
  d = opendir(directory);
  if (d) {
    while ((dir = readdir(d)) != NULL) {
      if(dir-> d_type != DT_DIR){
        strcpy(currentFileName," - ");
        strcat(currentFileName,dir->d_name);
        strcat(currentFileName,"\n");
        if(send(dSC,currentFileName, sizeof(currentFileName),0) == -1){
          perror("Problème lors de l'envoi du message de bienvenue");
        }
      }
      
    }
    closedir(d);
  }
  else{
    perror("Erreur lors de l'ouverture du répertoire contenant les fichiers");
  }
}

//-----------------------------COMMAND FETCH------------------------

void cmd_fetch(char *folder,int dSC){
  struct sockaddr_in aC ;
  socklen_t lg = sizeof(struct sockaddr_in);

  long dSCFT = accept(dSFTSTC, (struct sockaddr*) &aC,&lg);
  //Faire la gestion d'erreur
  if(dSCFT == -1){
    perror("Problème lors de l'acceptation d'une connexion");
  }

  // ENVOI DE LA LISTE DES FICHIERS
  DIR *d;
  struct dirent *dir;
  char directory[240];
  char currentFileName[240];
  strcpy(directory,directoryFile);
  //Si le demande concerne un sous dossier
  if(strcmp(folder,"") != 0){
    strcat(directory,"/");
    strcat(directory,folder);
  }
  strcat(directory,"/");
  printf("%s\n",directory );
  d = opendir(directory);
  if (d) {
    char message[240] = "Voici la liste des fichiers :\n";
    if(send(dSC,message,sizeof(message),0) == -1){
      perror("Problème lors de l'envoi du message de bienvenue");
    }
    while ((dir = readdir(d)) != NULL) {
      if(dir-> d_type != DT_DIR){
        strcpy(currentFileName," - ");
        strcat(currentFileName,dir->d_name);
        strcat(currentFileName,"\n");
        if(send(dSC,currentFileName, sizeof(currentFileName),0) == -1){
          perror("Problème lors de l'envoi du message de bienvenue");
        }
      }
      
    }
    closedir(d);
  }
  else{
    perror("Erreur lors de l'ouverture du répertoire contenant les fichiers");
  }
  //DEMANDE DU NOM DU FICHIER
  char requestFileName[240] = "[+]Choisissez un nom de fichier parmi ceux disponibles\n";
  if(send(dSC,requestFileName,sizeof(requestFileName),0) == -1){
    perror("Problème lors de l'envoi du message pour demander");
  }
  char fileName[240];
  if(recv(dSC,fileName,sizeof(fileName),0) == -1){
    perror("Problème lors de la réception du nom du fichier à envoyer");
  }
  char path[240];
  strcpy(path,directory);
  strcat(path,fileName);
  printf("%s\n",path);
  FILE *fps;
  while(!(fps = fopen(path, "rb"))){
    char requestFileNameErr[240] = "[+]Ce fichier n'est pas présent dans la liste des fichiers disponibles\n";
    if(send(dSC,requestFileNameErr,sizeof(requestFileNameErr),0) == -1){
      perror("Problème lors de l'envoi du message pour demander");
    }
    char requestFileNameErr2[240] = "[+]Choisissez un nom de fichier parmi ceux disponibles\n";
    if(send(dSC,requestFileNameErr2,sizeof(requestFileNameErr2),0) == -1){
      perror("Problème lors de l'envoi du message pour demander");
    }
    if(recv(dSC,fileName,sizeof(fileName),0) == -1){
      perror("Problème lors de la réception du nom du fichier à envoyer");
    }
    strcpy(path,directory);
    strcat(path,fileName);
  }

  fseek(fps,0,SEEK_END);
  size_t size = ftell(fps);
  fseek(fps,0,SEEK_SET);

  /*Créer thread*/
  file_transfert_server_to_client_struct *args = (file_transfert_server_to_client_struct*) malloc(sizeof(file_transfert_server_to_client_struct));
  
  args->fps = fps;
  strcpy(args->fileName,fileName);
  args->size = size;
  args->dSCFT = dSCFT;

  pthread_t fileTransfer;
  int thread_file = pthread_create(&fileTransfer,NULL,thread_file_transfer_server_to_client,(void*) args);
  if(thread_file != 0){
    perror("Erreur thread transfert de fichier");
    exit(-1);
  }
}

//--------------------------------------------------------------RELAY SECTION-------------------------------------------------

//------------------------------------------------THREAD RELAY ROOM-----------------------------------------------------------

void* thread_relay_room(void *args){
  //COTE CLIENT DANS UN NOUVEAU TERMINAL

  //pré requis : dSR du client, moyen de trouvé la room
  relay_room_struct *currentArgs = (relay_room_struct *) args;
  int dSR = currentArgs->dSR;
  int dSC = currentArgs->dSC;
  char* pseudo = (char *) malloc(240*sizeof(char));
  if(getPseudoBydSC(pseudo,dSC)==-1){
    printf("Erreur de recherche du pseudo\n");
  }

  int idRoom = currentArgs->idRoom;

  int roomIndex = getCurrentIndexRoom(idRoom);
  char folder[240];
  pthread_mutex_lock(&lockTabRoom);
  strcpy(folder,tabRoom[roomIndex].roomName);
  pthread_mutex_unlock(&lockTabRoom);
  char clientMessage[240];
  while(1){
    // Réception du message sous le format suivant -> <Utilisateur>message 
    int recvC = recv(dSR, clientMessage, sizeof(clientMessage), 0);
    if(recvC == -1){
      perror("Problème avec la réception");
    }
    printf("[->]%s\n",clientMessage );

//----------------------------------------COMMAND PART--------------------------------------------------------

    /*---------CONDITION FIN DU THREAD--------------------*/

    if(strcmp(clientMessage,"/fin") == 0){
      cmd_fin(dSR);
    }

    /*---------------------------FILE FUNCTIONALITY---------------------*/

    if(strcmp(clientMessage,"/file") == 0){
      int roomIndex = getCurrentIndexRoom(idRoom);
      char folder[240];
      pthread_mutex_lock(&lockTabRoom);
      strcpy(folder,tabRoom[roomIndex].roomName);
      pthread_mutex_unlock(&lockTabRoom);
      cmd_file(folder);
    }
    /*--------------------------------FETCH FUNCTIONALITY------------------------------------*/

    else if(strcmp(clientMessage,"/fetch") == 0){
      
      cmd_fetch(folder,dSR);
    }

    /*------------------------------DISPLAY USER--------------------------*/

    else if(strcmp(clientMessage, "/displayUsers") == 0){
      cmd_displayUsers(dSR);
    }

    /*-------------------------------------DISPLAY FILE IN SERVER-----------------*/

    else if(strcmp(clientMessage,"/displayFiles") == 0){

      cmd_displayFiles(folder,dSR);
    }

    /*----------------------------------MAN----------------------------------*/

    else if(strcmp(clientMessage, "/man") == 0){
      cmd_man("manRoom.txt",dSR);      
    }

    /*---------------------------------------LEAVE----------------------------------*/

    else if(strcmp(clientMessage,"/leave") == 0){

      shutdown(dSR,2);
      //SUPPRESSION DU DSR DU TABLEAU DE LA ROOM
      int currentRoomIndex = getCurrentIndexRoom(idRoom);
      pthread_mutex_lock(&lockTabRoom);
      int currentdSRIndex = -1;
      for (int i = 0; i < tabRoom[currentRoomIndex].maxIndexdSRRoom; ++i){
        if(tabRoom[currentRoomIndex].tab_dSRC[i] == dSR){
          currentdSRIndex = i;
        }
      }
      int maxIndexdSRRoom = tabRoom[currentRoomIndex].maxIndexdSRRoom;
      tabRoom[currentRoomIndex].tab_dSRC[currentdSRIndex] = tabRoom[currentRoomIndex].tab_dSRC[maxIndexdSRRoom-1];
      tabRoom[currentRoomIndex].maxIndexdSRRoom--;
      tabRoom[currentRoomIndex].tab_dSRC = (int*) realloc(tabRoom[currentRoomIndex].tab_dSRC, (maxIndexdSRRoom+1)*sizeof(int));
      
      char leavingMessage[240];
      strcpy(leavingMessage,pseudo);
      strcat(leavingMessage," a quitté le salon\n");

      for (int i = 0; i < tabRoom[currentRoomIndex].maxIndexdSRRoom; ++i){
            if(send(tabRoom[currentRoomIndex].tab_dSRC[i],leavingMessage,sizeof(leavingMessage),0) == -1){
              perror("Message non envoyé");
            }
        }
      pthread_mutex_unlock(&lockTabRoom);

      pthread_exit(NULL);
    }

    else{
      if(regexec(&regex,clientMessage,(size_t) 0, NULL, 0) == 0){
        //TRAITEMENT DE TEXTE (ALL + MESSAGE PRIVE)
      }else{
        int currentRoomIndex = getCurrentIndexRoom(idRoom);

        // MESSAGE A ENVOYER
        char messageToSend[240];
        strcpy(messageToSend,pseudo);
        strcat(messageToSend," : ");
        strcat(messageToSend,clientMessage);
        strcat(messageToSend,"\n");
        printf("[<-]%s\n",messageToSend);
        //ENVOYER A TOUTE LA ROOM
        pthread_mutex_lock(&lockTabRoom);
        for (int i = 0; i < tabRoom[currentRoomIndex].maxIndexdSRRoom; ++i){
          if(tabRoom[currentRoomIndex].tab_dSRC[i] != dSR){
            if(send(tabRoom[currentRoomIndex].tab_dSRC[i],messageToSend,sizeof(messageToSend),0) == -1){
              perror("Message non envoyé");
            }
          }
        }
        pthread_mutex_unlock(&lockTabRoom);
      }
    }
  }
}

/*--------------------------------------THREAD RELAY CLIENT-------------------------------------*/

void* relayClient(void* args){
    long dSC = *(long *)args;

  /*--------------------------------------INTRODUCTION-------------------------------*/

    char pseudo[240];
    int pseudoAvailability = 1;
    /*Introduction*/
    char introduction[240] = "[+]Bienvenue sur notre serveur! \n";
    int sendIntro = send(dSC,introduction, sizeof(introduction),0);
    if(sendIntro == -1){
      perror("Problème lors de l'envoi du message de bienvenue");
    }
    char introduction2[240] = "[+]Ici vous pourrez communiquer avec les autres personnes connectées ainsi qu'envoyer des fichiers au serveur!\n\n";
    int sendIntro2 = send(dSC,introduction2, sizeof(introduction2),0);
    if(sendIntro2 == -1){
      perror("Problème lors de l'envoi du message de bienvenue");
    }
    char information[240] = "[+]Voici le format à respecter pour vos messages :\n <Utilisateur>Message pour envoyer un message privé\n <Utilisateur1,Utilisateur2>Message pour envoyer plusieurs messages privés\n <All>Message pour envoyer à tous les salons\n";
    int sendInfo = send(dSC,information, sizeof(information),0);
    if(sendInfo == -1){
      perror("Problème lors de l'envoi du message de bienvenue");
    }
    char information3[240] = "[+]Pour envoyer un fichier au serveur, veuillez saisir le mot <<file>>\n";
    int sendInfo3 = send(dSC,information3, sizeof(information3),0);
    if(sendInfo3 == -1){
      perror("Problème lors de l'envoi du message de bienvenue");
    }
    char information4[240] = "[+]Pour afficher les fichiers disponibles sur le serveur, veuillez saisir le mot <<display>>\n";
    int sendInfo4 = send(dSC,information4, sizeof(information4),0);
    if(sendInfo4 == -1){
      perror("Problème lors de l'envoi du message de bienvenue");
    }
    char information5[240] = "[+]Pour récupérer un fichier du serveur, veuillez saisir le mot <<fetch>>\n\n";
    int sendInfo5 = send(dSC,information5, sizeof(information5),0);
    if(sendInfo5 == -1){
      perror("Problème lors de l'envoi du message de bienvenue");
    }

    /*--------------------------------------GESTION DU PSEUDO-------------------------------*/

    char pseudoMessage[240] = "[+]Nous allons pouvoir débuter! Veuillez saisir un pseudo : \n";
    while(pseudoAvailability ==1){
      int sendEnterPseudo = send(dSC,pseudoMessage, sizeof(pseudoMessage),0);
      if(sendEnterPseudo == -1){
        printf("Problème pour l'envoi du message de saisie du pseudo\n");
      }
      int recvPseudo = recv(dSC, pseudo, sizeof(pseudo), 0);
      if(recvPseudo == -1){
        perror("Problème lors de la réception du pseudo");
      }
      if(strcmp(pseudo,"/fin") == 0){
        int sem = sem_post(&semaphore);
        if(sem == -1){
          perror("A problem occured (sem init)");
        }
        shutdown(dSC, 2);
        pthread_exit(NULL);
      }
      else if(strlen(pseudo) > pseudoLength){
        char messageError[240] = "[+]La taille maximum d'un pseudo est ";
        char bufferPseudoLength[240];
        sprintf(bufferPseudoLength,"%d",pseudoLength);
        strcat(messageError,bufferPseudoLength);
        strcat(messageError, ". Veuillez réessayer\n");
        int sendEnterPseudo = send(dSC,messageError, sizeof(messageError),0);
        if(sendEnterPseudo == -1){
          printf("Problème pour l'envoi du message de saisie du pseudo\n");
        }
      }
      else if(strstr(pseudo,"<") != NULL || strstr(pseudo,">") != NULL || strstr(pseudo,"/") != NULL){
        char messageError[240] = "[+]Les caractères '<','>' et '/' ne sont pas autorisés dans les pseudos\n";
        int sendEnterPseudo = send(dSC,messageError, sizeof(messageError),0);
        if(sendEnterPseudo == -1){
          printf("Problème pour l'envoi du message de saisie du pseudo\n");
        }
      }
      else{
        //Envoyer l'erreur au client
        if(pseudoUsed(pseudo) == 0){
          pseudoAvailability = 0;
        }
        else if(pseudoUsed(pseudo) == 1){
          char erreur[240] = "[+]Pseudo déjà utilisé..\n";
          int sendErr = send(dSC,erreur, sizeof(erreur),0);
          if(sendErr == -1){
            perror("Problème lors de l'envoi du message d'erreur");
          }
        }
        else{
          printf("An error occured\n");
        }
      }
    }

/*----------------------------AJOUT UTILISATEUR DANS TABLEAU-----------------------*/

    pthread_mutex_lock(&lock);
    strcpy(tabdSC[maxIndex].pseudo,pseudo);
    tabdSC[maxIndex].dSC = dSC;
    maxIndex = maxIndex + 1;
    tabdSC = (Utilisateur*) realloc(tabdSC,(maxIndex+1)*sizeof(Utilisateur));
    pthread_mutex_unlock(&lock);
    char welcome[240] = "[+]Vous voilà entré dans le chat. Bienvenue!\n";
    int sendWel = send(dSC,welcome, sizeof(welcome),0);
    if(sendWel == -1){
      perror("Problème lors de l'envoi du message de bienvenue");
    }
    printf("%s est entré dans le serveur..\n",pseudo);
    char enteringMessageToAll[240];
    strcpy(enteringMessageToAll,pseudo);
    strcat(enteringMessageToAll," est entré dans le serveur!\n");
    pthread_mutex_lock(&lock);
    for(int i = 0; i < maxIndex; i++){
      // Envoi
      if(tabdSC[i].dSC != dSC){
        int sendC = send(tabdSC[i].dSC, enteringMessageToAll, sizeof(enteringMessageToAll), 0);
        if (sendC == -1){
          perror("send()");
          exit(0);
        }
      }
    }
    pthread_mutex_unlock(&lock);

/*-------------------------------DEBUT DU RELAY-----------------------------------------*/

  while(1){
    char clientMessage[240];
    // Réception du message sous le format suivant -> <Utilisateur>message 
    int recvC = recv(dSC, clientMessage, sizeof(clientMessage), 0);
    if(recvC == -1){
      perror("Problème avec la réception");
    }
    printf("[->]%s\n",clientMessage );

//----------------------------------------COMMAND PART--------------------------------------------------------

    /*---------CONDITION FIN DU THREAD--------------------*/

    if(strcmp(clientMessage,"/fin") == 0){
      cmd_fin(dSC);
    }

    /*---------------------------FILE FUNCTIONALITY---------------------*/

    if(strcmp(clientMessage,"/file") == 0){
      cmd_file("");
    }

    /*------------------------------DISPLAY USER--------------------------*/

    else if(strcmp(clientMessage, "/displayUsers") == 0){
      cmd_displayUsers(dSC);
    }

    /*-------------------------------------DISPLAY FILE IN SERVER-----------------*/

    else if(strcmp(clientMessage,"/displayFiles") == 0){
      cmd_displayFiles("",dSC);
    }

    /*--------------------------------FETCH FUNCTIONALITY------------------------------------*/

    else if(strcmp(clientMessage,"/fetch") == 0){
      cmd_fetch("",dSC);
    }
    /*----------------------------------MAN----------------------------------*/

    else if(strcmp(clientMessage, "/man") == 0){
      cmd_man("manLobby.txt",dSC);      
    }

    else if(strcmp(clientMessage,"/displayRooms") == 0){
      displayRoomsAvailable(dSC);
    }

    /*---------------------------------CREATE CHANNEL FUNCTIONALITY---------------------------*/

    else if(strcmp(clientMessage,"/createChannel") == 0){
      char messageCreate[240] = "[+]Saisissez le nom du nouveau salon que vous voulez créer\n";
      if(send(dSC,messageCreate,sizeof(messageCreate),0) == -1){
        perror("Erreur d'envoi du message du choix des salons");
      }
      char roomName[240];
      if(recv(dSC,roomName,sizeof(roomName),0) == -1){
        perror("Erreur d'envoi du message du choix des salons");
      }
      while(roomExists(roomName) == 0){
        char messageError[240] = "[+]Ce nom de salon est déjà utilisé..\nVeuillez réessayer\nSaisissez le nom du nouveau salon que vous voulez créer\n";
        if(send(dSC,messageError,sizeof(messageError),0) == -1){
          perror("Problème lors de l'envoi du message d'erreur");
        }
        if(recv(dSC,roomName,sizeof(roomName),0) == -1){
        perror("Erreur réception du choix");
        }
      }

      char messageCreate2[240] = "[+]Veuillez à présent saisir un thème d'au maximum 240 caractères\n";
      if(send(dSC,messageCreate2,sizeof(messageCreate2),0) == -1){
        perror("Erreur d'envoi du message du choix des salons");
      }
      char theme[240];
      if(recv(dSC,theme,sizeof(theme),0) == -1){
        perror("Erreur d'envoi du message du choix des salons");
      }

      int firstUnusedId = getFirstUnusedId();
      int firstUnusedPort = getFirstRoomPortUnused();

      pthread_mutex_lock(&lockTabRoom);
      strcpy(tabRoom[maxIndexRoom].roomName,roomName);
      strcpy(tabRoom[maxIndexRoom].theme,theme);
      tabRoom[maxIndexRoom].idRoom = firstUnusedId;

      createRoomFolder(directoryFile,tabRoom[maxIndexRoom].roomName);


      //CREATION DE LA SOCKET
      int dSR = socket(PF_INET, SOCK_STREAM, 0);
      if (dSR == -1){
        perror("Problème pendant la création de la socket");
        exit(-1);
      }
      int portR = firstUnusedPort;
      struct sockaddr_in adCtoS;
      adCtoS.sin_family = AF_INET;
      adCtoS.sin_addr.s_addr = INADDR_ANY ;
      adCtoS.sin_port = htons(portR) ;
      int bindR = bind(dSR, (struct sockaddr*)&adCtoS, sizeof(adCtoS));
      if (bindR == -1){
          perror("Problème pendant le nommage de la socket");
      }
      if (listen(dSR, 7) == -1){
        perror("Problème pendant la mise en écoute de la socket");
        exit(-1);
      }
      tabRoom[maxIndexRoom].dSR = dSR;
      tabRoom[maxIndexRoom].portR = portR;
      //INITIALISATION DU TABLEAU DES DSRC CLIENT DE LA ROOM
      tabRoom[maxIndexRoom].maxIndexdSRRoom = 0;
      tabRoom[maxIndexRoom].tab_dSRC = (int *) malloc((tabRoom[maxIndexRoom].maxIndexdSRRoom + 1 ) *sizeof(int));

      //AGRANDISSEMENT DU TABLEAU
      maxIndexRoom = maxIndexRoom + 1;
      tabRoom = (Room*) realloc(tabRoom,(maxIndexRoom+1)*sizeof(Room));
      pthread_mutex_unlock(&lockTabRoom);
      updateDatabaseRooms();
      char created[240] = "[+]Votre salon a bien été créé\n";
      int checkSend = send(dSC,created, sizeof(created),0);
      if(checkSend == -1){
        perror("Problème lors de l'envoi du message de bienvenue");
      }
    }

    /*---------------------------------MODIFY CHANNEL FUNCITONALITY---------------------------*/

    else if(strcmp(clientMessage,"/modifyChannel") == 0){
      //DEMANDE DU CHOIX DU SALON A MODIFIER
      char messageChoice[240] = "[+]Saisissez le nom ou le numéro du salon que vous voulez modifier :\n";
      if(send(dSC,messageChoice,sizeof(messageChoice),0) == -1){
        perror("Erreur d'envoi du message du choix des salons");
      }
      //AFFICHAGE DES SALONS DISPONIBLES
      displayRoomsAvailable(dSC);
      char choice[240];
      if(recv(dSC,choice,sizeof(choice),0) == -1){
        perror("Erreur réception du choix");
      }
      while(roomExists(choice) != 0){
        char messageError[240] = "[+]Le nom ou le numéro saisit ne correpond pas à un choix de la liste\nVeuillez réessayer\n";
        if(send(dSC,messageError,sizeof(messageError),0) == -1){
          perror("Problème lors de l'envoi du message d'erreur");
        }
        if(recv(dSC,choice,sizeof(choice),0) == -1){
        perror("Erreur réception du choix");
        }
      }
      //DEMANDE DU CHOIX DE LA MODIFICATION
      char messageChoiceMod[240] = "[+]Saisissez 'a' pour modifier le nom, ou 'b' pour modifier le thème du salon choisi:\n";
      if(send(dSC,messageChoiceMod,sizeof(messageChoiceMod),0) == -1){
        perror("Erreur d'envoi du message du choix des salons");
      }

      char choiceModify[240];

      if(recv(dSC,choiceModify,sizeof(choiceModify),0) == -1){
        perror("Erreur réception du choix");
      }
      while(strcmp(choiceModify,"a") != 0 && strcmp(choiceModify,"b") != 0){
        char messageError[240] = "[+]La demande est incompatible \nVeuillez réessayer\n";
        if(send(dSC,messageError,sizeof(messageError),0) == -1){
          perror("Problème lors de l'envoi du message d'erreur");
        }
        if(recv(dSC,choiceModify,sizeof(choiceModify),0) == -1){
        perror("Erreur réception du choix");
        }
      }

      //Get the current index of the room in tabRoom
      int idRoom = getIDRoom(choice);
      int currentIndexRoom = getCurrentIndexRoom(idRoom);

      if(strcmp(choiceModify,"a") == 0){
        //NOUVEAU NOM
        char askingNewName[240] = "[+]Veuillez saisir le nouveau nom :\n";
        if(send(dSC,askingNewName,sizeof(askingNewName),0) == -1){
          perror("Erreur d'envoi du message du choix des salons");
        }
        char newName[240];
        if(recv(dSC,newName,sizeof(newName),0) == -1){
          perror("Erreur réception du choix");
        }
        pthread_mutex_lock(&lockTabRoom);
        modifyRoomFolder(directoryFile,tabRoom[currentIndexRoom].roomName,newName);
        strcpy(tabRoom[currentIndexRoom].roomName,newName);
        pthread_mutex_unlock(&lockTabRoom);
      }
      else{
        //NOUVEAU THEME
        char askingNewTheme[240] = "[+]Veuillez saisir le nouveau thème (maximum 240 caractères) :\n";
        if(send(dSC,askingNewTheme,sizeof(askingNewTheme),0) == -1){
          perror("Erreur d'envoi du message du choix des salons");
        }
        char newTheme[240];
        if(recv(dSC,newTheme,sizeof(newTheme),0) == -1){
          perror("Erreur réception du choix");
        }
        pthread_mutex_lock(&lockTabRoom);
        close(tabRoom[currentIndexRoom].dSR);
        strcpy(tabRoom[currentIndexRoom].theme,newTheme);
        pthread_mutex_unlock(&lockTabRoom);
      }
      //update tab
      updateDatabaseRooms();
      char modified[240] = "[+]Votre salon a bien été modifié\n";
      int checkSend = send(dSC,modified, sizeof(modified),0);
      if(checkSend == -1){
        perror("Problème lors de l'envoi du message de bienvenue");
      }
    }

    /*---------------------------------DELETE CHANNEL FUNCITONALITY---------------------------*/

    else if(strcmp(clientMessage,"/deleteChannel") == 0){
      //DEMANDE DU CHOIX DU SALON A SUPPRIMER
      char messageChoice[240] = "[+]Saisissez le nom ou le numéro du salon que vous voulez supprimer :\n";
      if(send(dSC,messageChoice,sizeof(messageChoice),0) == -1){
        perror("Erreur d'envoi du message du choix des salons");
      }
      //AFFICHAGE DES SALONS DISPONIBLES
      displayRoomsAvailable(dSC);
      char choice[240];
      if(recv(dSC,choice,sizeof(choice),0) == -1){
        perror("Erreur réception du choix");
      }
      while(roomExists(choice) != 0){
        char messageError[240] = "Le nom ou le numéro saisit ne correspond pas à un choix de la liste\nVeuillez réessayer\n";
        if(send(dSC,messageError,sizeof(messageError),0) == -1){
          perror("Problème lors de l'envoi du message d'erreur");
        }
        if(recv(dSC,choice,sizeof(choice),0) == -1){
        perror("Erreur réception du choix");
        }
      }

      int idRoom = getIDRoom(choice);
      int currentIndexRoom = getCurrentIndexRoom(idRoom);

      pthread_mutex_lock(&lockTabRoom);
      deleteRoomFolder(directoryFile,tabRoom[currentIndexRoom].roomName);
      tabRoom[currentIndexRoom] = tabRoom[maxIndexRoom-1];
      maxIndexRoom--;
      pthread_mutex_unlock(&lockTabRoom);
      //update tab
      updateDatabaseRooms();
      char deleted[240] = "[+]Votre salon a bien été supprimé\n";
      int checkSend = send(dSC,deleted, sizeof(deleted),0);
      if(checkSend == -1){
        perror("Problème lors de l'envoi du message de bienvenue");
      }
    }
    else if(strcmp(clientMessage,"/join") ==  0){
      //ACCEPT CLIENT ON INTERMEDIATE CONNEXION
      struct sockaddr_in aCI ;
      socklen_t lgI = sizeof(struct sockaddr_in);
      long dSIC = accept(dSI, (struct sockaddr*) &aCI,&lgI);
      //DEMANDE DU CHOIX DU SALON
      char messageChoice[240] = "[+]Saisissez le nom ou le numéro du salon que vous voulez rejoindre :\n";
      if(send(dSC,messageChoice,sizeof(messageChoice),0) == -1){
        perror("Erreur d'envoi du message du choix des salons");
      }
      //AFFICHAGE DES SALONS DISPONIBLES
      displayRoomsAvailable(dSC);
      char choice[240];
      if(recv(dSC,choice,sizeof(choice),0) == -1){
        perror("Erreur réception du choix");
      }
      while(roomExists(choice) != 0){
        char messageError[240] = "Le nom ou le numéro saisit ne correspond pas à un choix de la liste\nVeuillez réessayer\n";
        if(send(dSC,messageError,sizeof(messageError),0) == -1){
          perror("Problème lors de l'envoi du message d'erreur");
        }
        if(recv(dSC,choice,sizeof(choice),0) == -1){
        perror("Erreur réception du choix");
        }
      }
      //ROOM INFO
      int portR = getPortRoom(choice);
      int dSR = getdSRRoom(choice);
      //SENDING RIGHT PORT ON INTERMEDIATE CONNEXION -----------------
      if(send(dSIC,&portR,sizeof(portR),0) == -1){
        perror("Erreur lors de l'envoi du port");
      }
      shutdown(dSIC,2);

      struct sockaddr_in aC ;
      socklen_t lg = sizeof(struct sockaddr_in);
      long dSRC = accept(dSR, (struct sockaddr*) &aC,&lg);
      //Faire la gestion d'erreur
      if(dSRC == -1){
        perror("Problème lors de l'acceptation d'une connexion");
      }

      //INTRODUCTION CHANNEL
      int idRoom = getIDRoom(choice); //Si le choix était un nom de room sinon retourne quand même l'id
      int indexChosenRoom = getCurrentIndexRoom(idRoom);
      char welcomeRoom[240] = "[+]Bienvenue dans le salon ";
      char descriptionRoom[240] = "[+]Description : ";
      pthread_mutex_lock(&lockTabRoom);
      strcat(welcomeRoom,tabRoom[indexChosenRoom].roomName);
      strcat(descriptionRoom,tabRoom[indexChosenRoom].theme);
      pthread_mutex_unlock(&lockTabRoom);

      strcat(welcomeRoom," . Ici vous pourrez communiquer avec les participants du salon\n");
      strcat(descriptionRoom,"\n");

      int check = send(dSRC,welcomeRoom, sizeof(welcomeRoom),0);
      if(check == -1){
        perror("Problème lors de l'envoi d'introduction au salon");
      }

      check = send(dSRC,descriptionRoom, sizeof(descriptionRoom),0);
      if(check == -1){
        perror("Problème lors de l'envoi d'introduction au salon");
      }

      /*AJOUT DU DSR DANS LE TABLEAU*/
      pthread_mutex_lock(&lockTabRoom);
      tabRoom[indexChosenRoom].tab_dSRC[tabRoom[indexChosenRoom].maxIndexdSRRoom] = dSRC;
      tabRoom[indexChosenRoom].maxIndexdSRRoom++;
      tabRoom[indexChosenRoom].tab_dSRC = realloc(tabRoom[indexChosenRoom].tab_dSRC,(tabRoom[indexChosenRoom].maxIndexdSRRoom + 1)*sizeof(int));
      pthread_mutex_unlock(&lockTabRoom);

      pthread_mutex_lock(&lockTabRoom);
      printf("%s est entré dans le salon %s\n",pseudo,tabRoom[indexChosenRoom].roomName);
      pthread_mutex_unlock(&lockTabRoom);
      char enteringMessageToRoom[240];
      strcpy(enteringMessageToRoom,pseudo);
      strcat(enteringMessageToRoom," est entré dans le salon!\n");

      pthread_mutex_lock(&lockTabRoom);
      for(int i = 0; i < tabRoom[indexChosenRoom].maxIndexdSRRoom; i++){
        // Envoi
        if(tabRoom[indexChosenRoom].tab_dSRC[i] != dSRC){
          int sendC = send(tabRoom[indexChosenRoom].tab_dSRC[i], enteringMessageToRoom, sizeof(enteringMessageToRoom), 0);
          if (sendC == -1){
            perror("send()");
            exit(0);
          }
        }
      }
      pthread_mutex_unlock(&lockTabRoom);

      //NEW RELAY ROOM THREAD 
      relay_room_struct *args = (relay_room_struct*) malloc(sizeof(relay_room_struct));
      args->idRoom = getIDRoom(choice);
      args->dSR = dSRC;
      args->dSC = dSC;

      pthread_t relay_room;
      int checkT = pthread_create(&relay_room,NULL,thread_relay_room,(void*) args);
      if(checkT != 0){
        perror("Erreur thread relay room");
        exit(-1);
      }
    }
    else{

//-------------------------------------------MESSAGE PART------------------------------------------------

      if(regexec(&regex,clientMessage,(size_t) 0, NULL, 0) != 0){
        char wrongFormat[240] = "[+]Le format n'est pas respecté\n[+]Rappel :\n <Utilisateur>Message pour envoyer un message privé\n <Utilisateur1,Utilisateur2>Message pour envoyer plusieurs messages privés\n <All>Message pour envoyer à tous les salons\n";
        int sendWrongFormat = send(dSC,wrongFormat, sizeof(wrongFormat),0);
        if(sendWrongFormat == -1){
          perror("Problème lors de l'envoi du message de correction");
        }
      }
      else{

        /*-----------------------------RIGHT FORMAT--------------------------*/

        /* Message treatment */
        char pseudoPart[240];
        char messagePart[240];
        //char sep[3] = "<>";
        pthread_mutex_lock(&tokLock);
        tok = strtok(clientMessage,"<>");
        strcpy(pseudoPart,tok);
        tok = strtok(NULL,"");
        strcpy(messagePart,tok);
        pthread_mutex_unlock(&tokLock);

        /*Reform message with User pseudo */
        char messageToSend[240] = "-";
        strcat(messageToSend,pseudo);
        strcat(messageToSend," : ");
        strcat(messageToSend,messagePart);
        /*Afficher la string par voir sa fin*/
        strcat(messageToSend,"\n");
        printf("[<-]%s\n",messageToSend);

        //----------------------ENVOYER A TOUS------------------------------------------

        if(strcmp(pseudoPart,"All") == 0){
          pthread_mutex_lock(&lock);
          for(int i = 0; i < maxIndex; i++){
            // Envoi
            if(tabdSC[i].dSC != dSC){
              int sendC = send(tabdSC[i].dSC, messageToSend, sizeof(messageToSend), 0);
              if (sendC == -1){
                perror("send()");
                exit(0);
              }
            }
          }
          pthread_mutex_unlock(&lock);
        } 
        //--------------------------MESSAGE PRIVE----------------------------------------
        else{
            /*Enter all seized pseudo in tab*/
            int indexPseudo = 0;
            char **pseudoTab = (char **) malloc((indexPseudo+1)*sizeof(char*));
            pseudoTab[indexPseudo] = (char *) malloc(pseudoLength*sizeof(char));
            /*About the first pseudo*/
            char sep[2] = ",";
            pthread_mutex_lock(&tokLock);
            tok = strtok(pseudoPart,sep);

            while(tok != NULL){
              strcpy(pseudoTab[indexPseudo],tok);
              /*Agrandissement du tableau*/
              indexPseudo += 1;
              pseudoTab = (char **) realloc(pseudoTab,(indexPseudo+1)*sizeof(char*));
              /*Création de l'espace nécessaire pour le prochain pseudo*/
              pseudoTab[indexPseudo] = (char *) malloc(pseudoLength*sizeof(char));
              tok = strtok(NULL,sep);
            }
            pthread_mutex_unlock(&tokLock);
            for(int i = 0; i < indexPseudo;i++){
              /*Condition pseudo exist*/
              if(pseudoUsed(pseudoTab[i]) == 1){
                for(int j = 0; j < maxIndex; j++){
                  // Envoi
                  pthread_mutex_lock(&lock);
                  if(strcmp(tabdSC[j].pseudo,pseudoTab[i]) == 0){
                    int sendC = send(tabdSC[j].dSC, messageToSend, sizeof(messageToSend), 0);
                    if (sendC == -1){
                      perror("send()");
                      exit(0);
                    }
                  }
                  pthread_mutex_unlock(&lock);
                }
              }
              else{
                /*Envoi du message d'erreur*/
                char errorMessage[240] = "L'utilisateur ";
                strcat(errorMessage, pseudoTab[i]);
                strcat(errorMessage," n'existe pas\n");
                int sendErr = send(dSC,errorMessage, sizeof(errorMessage),0);
                if(sendErr == -1){
                  perror("Problème lors de l'envoi du message de bienvenue");
                }
              }
          }
        }
      }
    }
  }
}

/*-----------------------------------------MAIN FUNCTION---------------------------------*/

int main(int argc, char *argv[]) {
  //Check parameters
  if(argc != 4){
    printf("Le nombre d'arguments n'est pas respecté.\n");
    printf("1. Le numéro de port\n2. Le nombre maximum de personnes dans la discussion\n3. Le répertoire pour la gestion des fichiers sous la forme : ./dossier\n");
    exit(-1);
  }
  if(atoi(argv[1]) < 0){
    printf("Le numéro de port %s un invalide (premier argument)\n",argv[1]);
    exit(-1);
  }
  if(atoi(argv[2]) < 0){
    printf("Le nombre de personne maximum dans la discussion, %s, est invalide (deuxième argument)\n",argv[2]);
    exit(-1);
  }
  DIR* dir;
  if (!(dir = opendir(argv[3]))) {
      printf("Le répertoire saisi, %s, n'existe pas (troisième argument)\n",argv[3]);
      exit(-1);
  }else {
    closedir(dir);
  }

  //ENREGISTREMENT DES PARAMETRES
  strcpy(port,argv[1]);
  strcpy(directoryFile,argv[3]);

  pseudoLength = 12;

  //INITIALISATION DU SEMAPHORE POUR LE NOMBRE DE CONNEXION
  int sem = sem_init(&semaphore, 0, atoi(argv[2]));
  if(sem == -1){
    perror("A problem occured (sem init)");
  }

  int regexReturn = regcomp(&regex,"^<[^<>][^<>]*>..*$",0);
  if(regexReturn == -1){
    perror("A problem occured (regcomp)");
  }

  if (pthread_mutex_init(&lock, 0) != 0) {
      printf("\n mutex init has failed\n");
      return 1;
  }

  //INITIALISATION DES ELEMENTS GLOBAUX

  //ABOUT USERS
  maxIndex = 0;
  tabdSC = (Utilisateur *) malloc ((maxIndex+1)*sizeof(Utilisateur));

  //ABOUT ROOMS
  maxIndexRoom = 0;
  tabRoom = (Room *) malloc ((maxIndexRoom+1)*sizeof(Room));
  //OPEN CSV
  FILE* fp = fopen("utiles/rooms.txt","r");
  if(!fp){
    printf("Can't open file rooms.txt\n");
    exit(-1);
  }
  char currentChar;
  char roomName[240];
  char theme[240];
  int IsName = 1;
  while(!feof(fp)){
    currentChar = fgetc(fp);
    //Name
    if(IsName){
      if(strcmp(&currentChar,",") == 0){
        IsName = 0;
        strcpy(tabRoom[maxIndexRoom].roomName,roomName);
        roomName[0] = '\0';
      }else{
        strcat(roomName,&currentChar);
      }

      
    }
    //Theme
    else{
      if(strcmp(&currentChar,"\n") == 0){
        IsName = 1;
        tabRoom[maxIndexRoom].idRoom = maxIndexRoom + 1;
        strcpy(tabRoom[maxIndexRoom].theme,theme);
        maxIndexRoom++;
        tabRoom = (Room *) realloc (tabRoom,(maxIndexRoom+1)*sizeof(Room));
        theme[0] = '\0';
      }
      else{
        strcat(theme,&currentChar);        
      }      
    }
  }
  fclose(fp);

  //CR3ATION DES SOCKETS SALONS + STOCKAGE DANS LE TABLEAU DES ROOM-------------
  //SAVING THE FIRST PORT WHERE THE ROOM SOCKET START FOR THE FUNCTION getFirstRoomPortUnused()
  roomSocketStartingPort = atoi(argv[1])+4;

  for(int i = 0; i <maxIndexRoom;i++){
    int dSR = socket(PF_INET, SOCK_STREAM, 0);
    if (dSR == -1){
      perror("Problème pendant la création de la socket");
      exit(-1);
    }
    int portR = atoi(argv[1])+4+i;
    struct sockaddr_in adCtoS;
    adCtoS.sin_family = AF_INET;
    adCtoS.sin_addr.s_addr = INADDR_ANY ;
    adCtoS.sin_port = htons(portR) ;
    int bindR = bind(dSR, (struct sockaddr*)&adCtoS, sizeof(adCtoS));
    if (bindR == -1){
        perror("Problème pendant le nommage de la socket room");
        exit(-1);
    }
    if (listen(dSR, 7) == -1){
      perror("Problème pendant la mise en écoute de la socket");
      exit(-1);
    }
    tabRoom[i].dSR = dSR;
    tabRoom[i].portR = portR;
    // Creation des répertoires pour les salons
    createRoomFolder(directoryFile,tabRoom[i].roomName);
    tabRoom[i].maxIndexdSRRoom = 0;
    tabRoom[i].tab_dSRC = (int *) malloc((tabRoom[i].maxIndexdSRRoom + 1 ) *sizeof(int));
  }

  //CREATION DE LA SOCKET INTERMEDIAIRE---------------------------------

  dSI = socket(PF_INET, SOCK_STREAM, 0);
  if (dSI == -1){
    perror("Problème pendant la création de la socket");
    exit(-1);
  }
  int portI = atoi(argv[1])+3;
  struct sockaddr_in adSI;
  adSI.sin_family = AF_INET;
  adSI.sin_addr.s_addr = INADDR_ANY ;
  adSI.sin_port = htons(portI) ;
  int bindI = bind(dSI, (struct sockaddr*)&adSI, sizeof(adSI));
  if (bindI == -1){
      perror("Problème pendant le nommage de la socket intermédiaire");
      exit(-1);
  }
  if (listen(dSI, 7) == -1){
    perror("Problème pendant la mise en écoute de la socket");
    exit(-1);
  }

  //CREATION DE LA SOCKET PRINCIPALE------------------------------------

  dS = socket(PF_INET, SOCK_STREAM, 0);
  if (dS == -1){
    perror("Problème pendant la création de la socket");
    exit(-1);
  }

  struct sockaddr_in ad;
  ad.sin_family = AF_INET;
  ad.sin_addr.s_addr = INADDR_ANY ;
  ad.sin_port = htons(atoi(argv[1])) ;
  int bindS = bind(dS, (struct sockaddr*)&ad, sizeof(ad)) ;
  if (bindS == -1){
    perror("Problème pendant le nommage de la socket principale");
    exit(-1);
  }
  int listenS = listen(dS, 7);
  if (listenS == -1){
    perror("Problème pendant la mise en écoute de la socket");
    exit(-1);
  }
  struct sockaddr_in aC ;
  socklen_t lg = sizeof(struct sockaddr_in);

  //Création de la socket client_to_serveur----------------------------------

  dSFTCTS = socket(PF_INET, SOCK_STREAM, 0);
  if (dSFTCTS == -1){
    perror("Problème pendant la création de la socket");
    exit(-1);
  }
  int portCTS = atoi(argv[1])+1;
  struct sockaddr_in adCtoS;
  adCtoS.sin_family = AF_INET;
  adCtoS.sin_addr.s_addr = INADDR_ANY ;
  adCtoS.sin_port = htons(portCTS) ;
  bindS = bind(dSFTCTS, (struct sockaddr*)&adCtoS, sizeof(adCtoS));
  if (bindS == -1){
      perror("Problème pendant le nommage de la socket client_to_serveur");
      exit(-1);
  }
  if (listen(dSFTCTS, 7) == -1){
    perror("Problème pendant la mise en écoute de la socket");
    exit(-1);
  }

  //Création de la socket server_to_client-------------------------------------------

  dSFTSTC = socket(PF_INET, SOCK_STREAM, 0);
  if (dSFTSTC == -1){
    perror("Problème pendant la création de la socket");
    exit(-1);
  }
  int portSTC = atoi(argv[1])+2;
  struct sockaddr_in adStoC;
  adStoC.sin_family = AF_INET;
  adStoC.sin_addr.s_addr = INADDR_ANY ;
  adStoC.sin_port = htons(portSTC) ;
  bindS = bind(dSFTSTC, (struct sockaddr*)&adStoC, sizeof(adStoC));
  if (bindS == -1){
      perror("Problème pendant le nommage de la socket server_to_client");
      exit(-1);
  }
  if (listen(dSFTSTC, 7) == -1){
    perror("Problème pendant la mise en écoute de la socket");
    exit(-1);
  }
  signal(SIGINT,end);
 /*------------------------------------------------------------------------------*/
  while(1){
    int sem = sem_wait(&semaphore);
    if(sem == -1){
      perror("A problem occured (sem wait)");
    }
    long dSC = accept(dS, (struct sockaddr*) &aC,&lg);
    //Faire la gestion d'erreur
    if(dSC == -1){
      perror("Problème lors de l'acceptation d'une connexion");
    }
/*----------------------------Création du thread client correspondant---------------------*/
    pthread_t thread_id;
    int rc1 = pthread_create(&thread_id, NULL,relayClient,(void*) &dSC);
    if(rc1 != 0){
      perror("Problème lors de la création du thread 1");
    }
  }
}