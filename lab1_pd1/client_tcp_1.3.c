/*
 Sviluppare un client che si colleghi ad un server TCP all’indirizzo e porta specificati come primo e
 secondo parametro sulla riga di comando e quindi invii due numeri interi senza segno letti da
 standard input e riceva la risposta (somma, o errore) dal server
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#include "errlib.h"
#include "sockwrap.h"
#include "panta.h"

char *prog_name;

int printable = 1;

int s;

int continue_service;

int main(int argc, char *argv[]){
	char *host = NULL, *port = NULL;
	uint16_t x,y;
	prog_name = argv[0];

	P_parse_args_client_1_3(argc, argv, &host, &port, &x, &y);

	//AF_UNSPECF
	s = P_connectTCP(host, port, AF_INET);
	p_printf("socket() and connect() succeded\n");

	P_service_client_1_3(s, x, y);

	/* closing */
	Close(s);
	p_printf("close() succeded\n");

	return 0;
}
