# projet-FAR
Une messagerie instantanée, avec plusieurs fonctionnalités, développé en C.

Commandes pour se connecter à l'application :
	
	- ouvrir un terminal dans la racine du projet (dossier Application), il servira seulement à compiler 
	- exécuter la commande : chmod 700 compile.sh (qui donne les droits d'accès au fichier) compile.sh
	- exécuter la commande : ./compile.sh (pour compiler)
	

	- ouvrir un terminal dans le dossier Serveur 
	- exécuter la commande : ./serveur <AdresseIP> <Port> <NombreMaxConnexion> <NombreMaxCaractèrePseudo> <RépertoireFichier> <Nombre de création maximum> <Nombre de connexion maximum par salon>
	- ouvrir un terminal dans le dossier Client 
	- obtenir l'adresse du serveur (avec ifconfig sur la machine du serveur)
	- exécuter la commande : ./client <Adresse_IP> 5555 ./clientFiles

Bienvenue sur notre application,une fois le pseudo rentré, utilisez la commande /man si vous voulez la liste des commandes disponibles ! 

