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
#include <errno.h>

#include "errlib.h"
#include "sockwrap.h"
#include "panta.h"

char *prog_name;

int printable = 1;

int s;

int continue_service;

int main(int argc, char *argv[]){
	char *host, *port;
        char request[NAME_LEN];

	prog_name = argv[0];

	P_parse_args_client_1_4(argc, argv, &host, &port, request);

	/* creating socket */
	//AF_UNSPEC
	s = P_connectUDP(host, port, AF_INET);
	p_printf("socket() and connect() succeded\n");

	P_service_client_1_4(s, request);

	/* closing */
	Close(s);
	p_printf("close() succeded\n");

	return 0;
}
