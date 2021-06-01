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
#include "client.h"
#define BUFFER_SIZE_FILE_TRANSFER 1024
#define GRN   "\x1B[32m"
#define RESET "\x1B[0m"

int dS;
int dSFTCTS;
int dSFTSTC;
int dSI;

pthread_t thread_idRec;
pthread_t thread_idSend;
int pseudoAvailable;
char ip_adress[240];
char port[240];
char directoryFile[240];


regex_t onlyDigit;
regex_t serverInfo;

int correctFileNameToImport = 0;
pthread_mutex_t lockBoolean;
pthread_mutex_t lockInfoFT;


void end(int n){
  pthread_cancel(thread_idRec);
  printf("Fin de la discussion\n");
  char messageToSend[240] = "/fin";
  int sendM = send(dS, messageToSend, sizeof(messageToSend), 0) ;
  if(sendM == -1){ 
    perror("Problème avec Send");
    exit(-1);
  }
  close(dS);
  close(dSFTCTS);
  close(dSFTSTC);
  pthread_cancel(thread_idSend);
}

/*---------------------CHOOSING FILE------------------------------*/
int get_last_tty() {
  FILE *fp;
  char path[1035];
  fp = popen("/bin/ls /dev/pts", "r");
  if (fp == NULL) {
    printf("Impossible d'exécuter la commande\n" );
    exit(1);
  }
  int i = INT_MIN;
  while (fgets(path, sizeof(path)-1, fp) != NULL) {
    if(strcmp(path,"ptmx")!=0){
      int tty = atoi(path);
      if(tty > i) i = tty;
    }
  }

  pclose(fp);
  return i;
}

FILE* new_tty() {
  pthread_mutex_t the_mutex;  
  pthread_mutex_init(&the_mutex,0);
  pthread_mutex_lock(&the_mutex);
  system("gnome-terminal"); sleep(1);
  char *tty_name = ttyname(STDIN_FILENO);
  int ltty = get_last_tty();
  char str[2];
  sprintf(str,"%d",ltty);
  int i;
  for(i = strlen(tty_name)-1; i >= 0; i--) {
    if(tty_name[i] == '/') break;
  }
  tty_name[i+1] = '\0';  
  strcat(tty_name,str);  
  FILE *fp = fopen(tty_name,"wb+");
  pthread_mutex_unlock(&the_mutex);
  pthread_mutex_destroy(&the_mutex);
  return fp;
}

// THREAD FILE TRANSFER SERVER TO CLIENT--------------------------------------------

void* thread_file_transfer_server_to_client(){
  dSFTSTC = socket(PF_INET, SOCK_STREAM, 0);
  if (dSFTSTC == -1){
    perror("Problème pendant la création de la socket");
    exit(-1);
  }
  struct sockaddr_in aS;
  aS.sin_family = AF_INET;
  inet_pton(AF_INET,ip_adress,&(aS.sin_addr)) ;
  aS.sin_port = htons(atoi(port)+2);
  socklen_t lgA = sizeof(struct sockaddr_in) ;

  if(connect(dSFTSTC, (struct sockaddr *) &aS, lgA) == -1){
    perror("Problème lors de la demande de connexion");
  }

  //Réception du nom du fichier
  char fileName[240];
  if(recv(dSFTSTC,fileName,sizeof(fileName),0) == -1){
    perror("Problème lors de la réception du nom du fichier");
  }

  //Réception de la taille du fichier
  size_t fileSize;
  if(recv(dSFTSTC,&fileSize,sizeof(fileSize),0) == -1){
    perror("Problème lors de la réception de la taille du fichier");
  }

  FILE *fp;
  char path[240];
  strcat(path,directoryFile);
  strcat(path,"/");
  strcat(path,fileName);
  fp = fopen(path,"wb+");
  char buffer[BUFFER_SIZE_FILE_TRANSFER];
  size_t currentSize = 0;
  while (currentSize <fileSize) {
    int n = recv(dSFTSTC, buffer, BUFFER_SIZE_FILE_TRANSFER, MSG_WAITALL);
    if(n == -1){
      perror("Une erreur est survenue pendant le transfert du fichier");
    }
    if(currentSize+BUFFER_SIZE_FILE_TRANSFER > fileSize){
      fwrite(buffer,fileSize-currentSize,1,fp);
    }
    else{
      fwrite(buffer, BUFFER_SIZE_FILE_TRANSFER,1,fp);
    }
    bzero(buffer, BUFFER_SIZE_FILE_TRANSFER);
    fseek(fp,0,SEEK_END);
    currentSize = ftell(fp);

  }
  printf("--Fichier reçu--\n");
  fclose(fp);
  shutdown(dSFTSTC, 2);
  pthread_exit(NULL);
}

// THREAD FILE TRANSFER CLIENT TO SERVER----------------------------------------------

void* thread_file_transfer_client_to_server(void* args){
  file_transfert_client_to_serveur_struct *currentArgs = (file_transfert_client_to_serveur_struct *) args;

  dSFTCTS = socket(PF_INET, SOCK_STREAM, 0);
  if (dSFTCTS == -1){
    perror("Problème pendant la création de la socket");
    exit(-1);
  }
  struct sockaddr_in aS;
  aS.sin_family = AF_INET;
  inet_pton(AF_INET,ip_adress,&(aS.sin_addr)) ;
  aS.sin_port = htons(atoi(port)+1);
  socklen_t lgA = sizeof(struct sockaddr_in) ;

  if(connect(dSFTCTS, (struct sockaddr *) &aS, lgA) == -1){
    perror("Problème lors de la demande de connexion");
  }

  //Envoi du nom
  char fileName[240];
  strcpy(fileName,currentArgs->fileName);
  if(send(dSFTCTS,fileName,sizeof(fileName),0) == -1){
    perror("Problème lors de l'envoi du nom du fichier");
  }
  //Envoi de la taille
  size_t size = currentArgs->size;
  if(send(dSFTCTS,&size,sizeof(size),0) == -1){
    perror("Problème lors de l'envoi de la taille du fichier");
  }

  FILE *fps = currentArgs->fps;

  char buffer[BUFFER_SIZE_FILE_TRANSFER];
  while(!feof(fps)){
    int numberOfBlocRead = fread(buffer, BUFFER_SIZE_FILE_TRANSFER,1,fps);
    if(numberOfBlocRead == -1){
      perror("Problème fread");
    }
    int n = send(dSFTCTS, buffer, sizeof(buffer), 0);
    if (n == -1) {
      perror("[-]Error in sending file.");
      exit(1);
    }
    if(n != 1024){
      printf("SEND %d\n",n );
    }
    bzero(buffer, BUFFER_SIZE_FILE_TRANSFER);
  }
  if(fclose(fps) != 0){
    perror("Erreur lors de la fermeture du fichier");
  }
  printf("--Fichier envoyé--\n");
  shutdown(dSFTCTS,2);
  pthread_exit(NULL);
}
//--------------------------------------------FUNCTION COMMAND-----------------------------------------------

void cmd_fin(){
  pthread_cancel(thread_idRec);
  printf("Fin de la discussion\n");
  char messageToSend[240] = "/fin";
  int sendM = send(dS, messageToSend, sizeof(messageToSend), 0) ;
  if(sendM == -1){ 
    perror("Problème avec Send");
    exit(-1);
  }
  close(dS);
  close(dSFTCTS);
  close(dSFTSTC);
  pthread_exit(NULL);
}

void cmd_file(){
  FILE* fp1 = new_tty();
  fprintf(fp1,"%s\n","Ce terminal sera utilisé uniquement pour l'affichage");

  // Demander à l'utilisateur quel fichier afficher
  DIR *dp;
  struct dirent *ep;     
  dp = opendir (directoryFile);
  if (dp != NULL) {
    fprintf(fp1,"Voilà la liste de fichiers :\n");
    while ((ep = readdir (dp))) {
      if(strcmp(ep->d_name,".")!=0 && strcmp(ep->d_name,"..")!=0) 
        fprintf(fp1,"%s\n",ep->d_name);
    }    
    (void) closedir (dp);
  }
  else {
    perror ("Ne peux pas ouvrir le répertoire");
  }
  printf("Indiquer le nom du fichier : ");
  char fileName[240];
  fgets(fileName,sizeof(fileName),stdin);
  fileName[strlen(fileName)-1]='\0';
  char path[240];
  strcpy(path,directoryFile);
  strcat(path,"/");
  strcat(path,fileName);
  FILE *fps;
  //CANCEL
  while(!(fps = fopen(path, "rb"))){
    printf("Le fichier suivant est indisponible dans votre répertoire : %s\n",fileName);
    printf("Veuillez entrer un nom de fichier existant : ");
    fgets(fileName,sizeof(fileName),stdin);
    fileName[strlen(fileName)-1]='\0';
    path[0]='\0';
    strcat(path,directoryFile);
    strcat(path,"/");
    strcat(path,fileName);
  }
  path[0]='\0';
  fseek(fps,0,SEEK_END);
  size_t size = ftell(fps);
  fseek(fps,0,SEEK_SET);

  /*INITIALISATION DE LA STRUCTURE*/
  file_transfert_client_to_serveur_struct *args = (file_transfert_client_to_serveur_struct *) malloc(sizeof(file_transfert_client_to_serveur_struct));

  args->fps = fps;
  strcpy(args->fileName,fileName);
  args->size = size;

  /*GESTION TRANSFERT*/
  pthread_t fileTransfer;
  int thread_file = pthread_create(&fileTransfer,NULL,thread_file_transfer_client_to_server,(void*) args);
  if(thread_file != 0){
    perror("Erreur thread transfert de fichier");
    exit(-1);
  }
}


//--------------------------------------THREAD JOINNNG----------------------------------
void* thread_joinning(){

  dSI = socket(PF_INET, SOCK_STREAM, 0);
  if (dSI == -1){
    perror("Problème pendant la création de la socket");
    exit(-1);
  }
  struct sockaddr_in aS;
  aS.sin_family = AF_INET;
  inet_pton(AF_INET,ip_adress,&(aS.sin_addr)) ;
  aS.sin_port = htons(atoi(port)+3);
  socklen_t lgA = sizeof(struct sockaddr_in) ;

  if(connect(dSI, (struct sockaddr *) &aS, lgA) == -1){
    perror("Problème lors de la demande de connexion");
  }
  //RECEIVE THE RIGHT PORT ON INTERMIDATE CONNEXION
  int portOfRoom;
  if(recv(dSI,&portOfRoom,sizeof(portOfRoom),0) == -1){
    perror("Problème lors du receive du port choisi");
  }
  shutdown(dSI,2);

  /*NEW SOCKET TO ROOM*/
  
  char command[240] ="gnome-terminal -- bash -c \"./clientRoom ";
  strcat(command,ip_adress);
  char portRoom[10];
  sprintf(portRoom," %d ",portOfRoom);
  strcat(command,portRoom);
  strcat(command,directoryFile);
  strcat(command," ");
  strcat(command,port);
  strcat(command,"\" ");
  system(command);
  sleep(1);

  pthread_exit(NULL);
}

//-------------------------------------THREAD SEND---------------------------------

void* thread_send(){
  char messageToSend[240];
  while(1){
    fgets(messageToSend,240,stdin);
    char *pos = strchr(messageToSend,'\n');
    *pos = '\0';

    /*Condition de terminaison du client*/
    if(strcmp(messageToSend,"/fin") == 0){
      cmd_fin();
    }
    int sendPseudo = send(dS, messageToSend, sizeof(messageToSend), 0) ;
    if(sendPseudo == -1){ 
      perror("Problème avec Send");
      exit(-1);
    }

    /*--------------------------------FILE FUNCTIONALIY-----------------------------------*/

    if(strcmp(messageToSend,"/file") == 0){
      cmd_file();
    }

    /*----------------------------------------FETCH FUNCTIONALITY------------------------*/

    if(strcmp(messageToSend,"/fetch") == 0){
      pthread_t fileTransfer;
      int thread_file = pthread_create(&fileTransfer,NULL,thread_file_transfer_server_to_client,NULL);
      if(thread_file != 0){
        perror("Erreur thread transfert de fichier");
        exit(-1);
      }
    }

    /*---------------------------------------JOIN FUNCTIONALITY-----------------------------*/
    if(strcmp(messageToSend,"/join") == 0){
      pthread_t joinning;
      int check = pthread_create(&joinning,NULL,thread_joinning,NULL);
      if(check != 0){
        perror("Erreur thread du démarrage du thread joinning");
        exit(-1);
      }
    }
  }
}

//----------------------------------------THREAD RECEIVE-------------------------

void* thread_receive(){
  char msgRec[240];
  while(1){
    int recvM = recv(dS, msgRec,sizeof(msgRec),0) ;
    if(recvM == 0){
      printf("Serveur éteint\n");
      close(dS);
      close(dSFTCTS);
      close(dSFTSTC);
      exit(-1);
    }
    if(recvM == -1){
      perror("Problème avec Recv");
      exit(-1);
    }
    if(regexec(&serverInfo,msgRec,(size_t) 0, NULL, 0) == 0){
      printf(GRN "%s" RESET,msgRec);
      printf("\n");
    } 
    else{
      printf("%s", msgRec);
    }

  }
}

//----------------------------------------------------MAIN FUNCTION---------------------------------------

int main(int argc, char *argv[]){
  //Check parameters
  if(argc != 4){
    printf("Le nombre d'arguments n'est pas respecté.\n");
    printf("1. L'adresse IP du serveur\n2. Le numéro de port\n3. Le répertoire pour la gestion des fichiers sous la forme : ./dossier\n");
    exit(-1);
  }
  if(atoi(argv[2]) < 0){
    printf("Le numéro de port %s un invalide (premier argument)\n",argv[2]);
    exit(-1);
  }
  DIR* dir;
  if (!(dir = opendir(argv[3]))) {
      printf("Le répertoire saisit, %s, n'existe pas (troisième argument)\n",argv[3]);
      exit(-1);
  }else {
    closedir(dir);
  }
  signal(SIGINT,end);
  /*Copie des arguments*/
  strcpy(ip_adress,argv[1]);
  strcpy(port,argv[2]);
  strcpy(directoryFile,argv[3]);

  /*RegEx*/
  if(regcomp(&serverInfo,"^\\[+\\].*",0) == -1){
    perror("A problem occured (regcomp)");
  }

  dS = socket(PF_INET, SOCK_STREAM, 0);
  if (dS == -1){
    perror("Problème pendant la création de la socket");
    exit(-1);
  }

  struct sockaddr_in aS;
  aS.sin_family = AF_INET;
  if(inet_pton(AF_INET,argv[1],&(aS.sin_addr)) != 1){
    printf("L'adresse IP %s est invalide \n",argv[1]);
    exit(-1);
  } 
  aS.sin_port = htons(atoi(argv[2])) ;
  socklen_t lgA = sizeof(struct sockaddr_in) ;

  int connectS = connect(dS, (struct sockaddr *) &aS, lgA) ;
  if (connectS == -1){
    perror("Problème pendant l'envoi de la demande de connexion");
    exit(-1);
  }
/*-------------------------------Création du thread Receive-------------------------*/
  int thread_recep = pthread_create(&thread_idRec,NULL,thread_receive,NULL);
  if(thread_recep != 0){
    perror("Erreur thread réception");
    exit(-1);
  }
/*-------------------------------Création du thread Send----------------------------*/
  int thread_saisie = pthread_create(&thread_idSend,NULL,thread_send,NULL);
  if(thread_saisie != 0){
    perror("Erreur thread saisie");
    exit(-1);
  }
/*-----------------Mise en attente du main jusqu'à la fin du thread Send----------*/ 
  int joinReturn1 = pthread_join(thread_idSend,NULL);
  if(joinReturn1 != 0){
    perror("Probleme join reception");
    exit(-1);
  }
  printf("Terminaison du client\n");
  return 0;
}



