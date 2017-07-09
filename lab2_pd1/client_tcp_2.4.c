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
#include <rpc/rpc.h>

#include "errlib.h"
#include "sockwrap.h"
#include "panta.h"

char *prog_name;

int printable = 1;

int s;

int continue_service;

int main(int argc, char *argv[]){
	char *host = NULL, *port = NULL;
	int x, y;

	prog_name = argv[0];

	P_parse_args_client_2_4(argc, argv, &host, &port, &x, &y);

	//AF_UNSPEC
	s = P_connectTCP(host, port, AF_INET);
	p_printf("socket() and connect() succeded\n");
	
	P_service_client_2_4(s, x, y);
	// NB. s will be closed in P_client_service_2_4()

	return 0;
}
