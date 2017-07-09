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
	char *host, *port;
	char filename[FILENAME_LEN];

	prog_name = argv[0];

	Signal(SIGINT, sig_handler);

	P_parse_args_client_2_3(argc, argv, &host, &port, filename);

	//AF_UNSPEC
	s = P_connectTCP(host, port, AF_INET);
	p_printf("socket() and connect() succeeded\n");

	p_service_client_2_3(s, filename);

	/* closing */
	Close(s);
	p_printf("close() succeeded\n");

	return 0;
}
