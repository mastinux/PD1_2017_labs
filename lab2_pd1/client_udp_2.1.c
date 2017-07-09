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
	int requests_sent = 0;

	prog_name = argv[0];

	Signal(SIGINT, sig_handler);

	memset(request, 0, NAME_LEN);

	P_parse_args_client_1_4(argc, argv, &host, &port, request);
	
	//AF_UNSPEC
	/* creating socket */
	s = P_connectUDP(host, port, AF_INET);
	p_printf("socket() and connect() succeded\n");

	while( requests_sent < MAX_REQUESTS_2_1 ){
		p_printf("attempt %d\n", requests_sent + 1);
		if ( P_service_client_1_4(s, request) == 0 )
			break;

		requests_sent++;	
	}

	/* closing */
	Close(s);
	p_printf("close() succeded\n");

	return 0;
}
