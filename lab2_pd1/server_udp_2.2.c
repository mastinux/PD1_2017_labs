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
#include <sys/wait.h>

#include "errlib.h"
#include "sockwrap.h"
#include "panta.h"

char *prog_name;

int printable = 1;

int s;

int continue_service;

int main(int argc, char *argv[]){
	char *port = NULL;

	prog_name = argv[0];

	P_parse_args_server_1_4(argc, argv, &port);

	//AF_INET6
	s = P_bindUDP(port, AF_INET);

	// limiting = 1
	p_service_server_1_4(s, 1);

	/* closing */
	Close(s);
	p_printf("close() succeded\n");

	return 0;
}
