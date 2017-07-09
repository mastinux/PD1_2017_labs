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
	int xdr_mode;
	char *host, *port;//, *filename;
	char filename[FILENAME_LEN];

	prog_name = argv[0];

	Signal(SIGINT, sig_handler);

	P_parse_args_client_3_4(argc, argv, &xdr_mode, &host, &port, filename);

	p_printf("%s(%d) %s %s %s (filename len %d)\n", xdr_mode == 0? "ASCII" : "XDR", xdr_mode, host, port, filename, strlen(filename));

	//AF_UNSPEC
	s = P_connectTCP(host, port, AF_INET);
	p_printf("socket() and connect() succeeded\n");

	if ( xdr_mode == 1 )
		p_service_xdr_client_3_4(s, filename);
	else{
		p_service_client_2_3(s, filename);

		/* closing */
		Close(s);
		p_printf("close() succeeded\n");
	}

	return 0;
}
