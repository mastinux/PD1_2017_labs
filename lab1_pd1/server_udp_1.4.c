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

	int children = -1;// -1 no fork, 0 fork on request, n (< MAX_CHLD) pre-fork
	int pid;

	prog_name = argv[0];

	Signal(SIGINT, sig_handler);
	Signal(SIGPIPE, sig_handler);	// SIGPIPE called when write on socket failed 
	Signal(SIGCHLD, SIG_IGN);	// SIG_IGN ignores child return value, avoids zombie

	P_parse_args_server_1_4(argc, argv, &port);

//	P_parse_args_children_server_2_3(argc, argv, &port, &children);

	if (children == 0)
		children = 1;

	//AF_INET6
	s = P_bindUDP(port, AF_INET);

	if ( children == -1 ){	// no fork
		p_printf("no fork\n");
		while(1){
			// limiting = 1
			p_service_server_1_4(s, 0);
		}
	}
	else{	// pre-forking
		p_printf("pre-fork\n");
		for(int i = 0; i < children; i++){
			pid = fork();

			if ( pid < 0 ){
				p_printf("!!! fork() failed\n");
				//p_printf("errno: %s\n", strerror(errno));
			}
			else if ( pid == 0 ){	// child
				while(1){
					// limiting = 1
					p_service_server_1_4(s, 0);
				}
			}
			else{	// parent
				
			}
		}

		wait(NULL);
	}

	/* closing */
	Close(s);
	p_printf("close() succeded\n");

	return 0;
}
