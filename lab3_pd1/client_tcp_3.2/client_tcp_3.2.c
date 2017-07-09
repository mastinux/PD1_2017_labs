#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <ctype.h>

#include "../errlib.h"
#include "../sockwrap.h"
#include "../panta.h"

char *prog_name;

int printable = 1;

int s;

int continue_service = 1;

int main(int argc, char *argv[]){
	prog_name = argv[0];

	Signal(SIGINT, sig_handler);

	/* checking number of arguments */
	if (argc != 3)
		err_quit("usage: %s <address> <port>\n", argv[0]);

	//AF_UNSPEC
	s = P_connectTCP(argv[1], argv[2], AF_INET);
	p_printf("socket() and connect() succeeded\n");

	// type-ahead
	p_service_client_3_2(s);

	/* closing */
	Close(s);
	p_printf("close() succeeded\n");

	return 0;
}
