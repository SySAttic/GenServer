

#include <stdio.h>
#include <Winsock2.h>

#define PORTNUM 80 //

//mangler1.generals.ea.com 4321
//mangler2.generals.ea.com  4321
//mangler3.generals.ea.com  4321
//mangler4.generals.ea.com  4321 Generals main serves 
//spew back what main serv sends to client
void do_something(SOCKET s)
{
	char inStr[256];
	int ret;
	printf("On new connection\n");
	
	ret = recv(s, inStr, sizeof(inStr), 0);
	
	printf("Server Received: %s\n\r", inStr);

	char GameKey[] = ":s 705 * SjcYwcde>PQImGcE RC>AjNWf[m>jd=gr\r\n";
	send( s, GameKey, sizeof(GameKey), 0);
    //Need to re-create encryption, looks like x0r to me.

	char MyIP[] = ":s 302  :=+@127.0.0.1\n";
	send( s, MyIP, sizeof(MyIP), 0);  
	char MOTD_START[] = ":s 001 SyS :Welcome to the Matrix SyS\n\r";
	 send( s, MOTD_START, sizeof(MOTD_START), 0);  
	
	char MOTD_START2[] = ":s 004 SyS s 1.0 iq biklmnopqustvhe\n\r";
	 send( s, MOTD_START2, sizeof(MOTD_START2), 0); 
	
	
	char MOTD_START3[] = ":s 375 SyS :- (M) Message of the day - \n\r";
	 send( s, MOTD_START3, sizeof(MOTD_START3), 0);  
	
	
	char END_MOTD[] = ":s 376 SyS :End of MOTD command\n\r";
	 send( s, END_MOTD, sizeof(END_MOTD), 0);   
	
	
	char CHAN_JOIN[] = ":SyS!XlG1W4OFpX|153849803@* JOIN :#GPG!2266\n\r";
	send( s, CHAN_JOIN, sizeof(CHAN_JOIN), 0);
	
	char TOPIC_START[] = ":s 332 SyS #GPG!2266 :TOPIC Test??\n\r";
	 send( s, TOPIC_START, sizeof(TOPIC_START), 0);
	

	char NAMES_LIST[] = ":s 353 SyS * #GPG!2266 :SyS\n\r";
	 send( s, NAMES_LIST, sizeof(NAMES_LIST), 0);

	 	char NAMES_END[] = ":s 366 SyS #GPG!2266 :End of NAMES list\n\r";
	 send( s, NAMES_END, sizeof(NAMES_END), 0);
	
	 	char UNKCMD[] = ":s 333 SyS #GPG!2266 SERVER 1217231339\n\r";
	 send( s, UNKCMD, sizeof(UNKCMD), 0);
	
	
	char MODETEST[] = ":s 324 SyS #GPG!2266 +tnp\n\r";
	 send( s, MODETEST, sizeof(MODETEST), 0);
	
	char ckey[] = ":s 702 SyS #GPG!2266 SyS 000 :XlG1W4OFpX|153849803\n\r";
	 send( s, ckey, sizeof(ckey), 0);
	
	char Eckey[] = ":s 703 SyS #GPG!2266 001 :End of GETCKEY\n\r";//end chan bcasty
	 send( s, Eckey, sizeof(Eckey), 0);
}


/* code to establish a socket
*/
SOCKET establish(unsigned short portnum)
{ 
	char   myname[256];
	SOCKET s;
	struct sockaddr_in sa;
	struct hostent *hp;
	
	memset(&sa, 0, sizeof(struct sockaddr_in)); /* clear our address */
	gethostname(myname, sizeof(myname));        /* who are we? */
	hp = gethostbyname(myname);                 /* get our address info */
	if (hp == NULL)                             /* we don't exist !? */
		return(INVALID_SOCKET);
	sa.sin_family = hp->h_addrtype;             /* this is our host address */
	sa.sin_port = htons(portnum);               /* this is our port number */
	s = socket(AF_INET, SOCK_STREAM, 0);        /* create the socket */
	if (s == INVALID_SOCKET)
		return INVALID_SOCKET;
	
	/* bind the socket to the internet address */
	if (bind(s, (struct sockaddr *)&sa, sizeof(struct sockaddr_in)) ==
		SOCKET_ERROR) 
	{
		closesocket(s);
		return(INVALID_SOCKET);
	}
	listen(s, 3);                               /* max # of queued connects */
	return(s);
}


int main(int argc, char* argv[])
{ 
	SOCKET s;
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;
	
	wVersionRequested = MAKEWORD( 2, 2 );
	
	err = WSAStartup( wVersionRequested, &wsaData );
	if ( err != 0 ) {
		/* Tell the user that we could not find a usable */
		/* WinSock DLL.                                  */
		return 1;
	}
	
	char   myname[256];
	struct sockaddr_in sa;
	struct hostent *hp;
	

	memset(&sa, 0, sizeof(struct sockaddr_in)); /* clear our address */
	gethostname(myname, sizeof(myname));        /* who are we? */
	hp = gethostbyname(myname);                 /* get our address info */
	if (hp == NULL)                             /* we don't exist !? */
		return(INVALID_SOCKET);
	sa.sin_family = hp->h_addrtype;             /* this is our host address */
	sa.sin_port = htons(PORTNUM);               /* this is our port number */
	s = socket(AF_INET, SOCK_DGRAM, 0);        /* create the socket */
	if (s == INVALID_SOCKET)
		return INVALID_SOCKET;
	
	/* bind the socket to the internet address */
	if (bind(s, (struct sockaddr *)&sa, sizeof(struct sockaddr_in)) ==
		SOCKET_ERROR) 
	{
		closesocket(s);
		return(INVALID_SOCKET);
	}
	
	if ((s = establish(PORTNUM)) == INVALID_SOCKET) { /* plug in the phone */
		perror("establish");
		exit(1);	
	}
	
	SOCKET new_sock;
	for (;;) 
	{                            /* loop for phone calls */
		new_sock = accept(s, NULL, NULL);
		if (s == INVALID_SOCKET) 
		{
			fprintf(stderr, "Error waiting for new connection!\n");
			exit(1);
		}
		do_something(new_sock);
	}
    closesocket(new_sock);
	return 1;
}


