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
#include <sys/stat.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>

#include "../errlib.h"
#include "../sockwrap.h"
#include "../panta.h"

#define BACKLOG 10

char *prog_name;

int printable;

int s;

int continue_service;

int main(int argc, char *argv[]){
	char *port;
	int cs;
	struct sockaddr_storage client_sockaddr;
	socklen_t client_sockaddr_len; 
	char client_address_string[INET6_ADDRSTRLEN]; 
	uint16_t client_address_port;

	int children = -1;	// -1 no fork, 0 fork on request, n (< MAX_CHLD) pre-fork
	int pid;

	int xdr_mode = -123456;

	prog_name = argv[0];

	printable = 1;

	Signal(SIGINT, sig_handler);
	Signal(SIGPIPE, sig_handler);	// SIGPIPE called when write on socket failed 
	Signal(SIGCHLD, SIG_IGN);	// SIG_IGN ignores child return value, avoids zombie

	P_parse_args_server_3_4(argc, argv, &port, &xdr_mode);

//	P_parse_args_children_server_3_4(argc, argv, &port, &children, &xdr_mode);

//	P_parse_args_optional_children_server_3_4(argc, argv, &port, &children, &xdr_mode);

	p_printf("%s(%d) %s %d\n", xdr_mode == 0? "ASCII" : "XDR", xdr_mode, port, children);

	//AF_INET6
	s = P_listenTCP(port, AF_INET);

	client_sockaddr_len = sizeof(struct sockaddr_storage);

	if ( children == -1 ){	// no fork
		while(1){
			p_printf("waiting connection\n");

			memset(&client_sockaddr, 0, sizeof(struct sockaddr_storage));

			cs = accept(s, (struct sockaddr *)&client_sockaddr, &client_sockaddr_len);

			if ( cs < 0 ){
				p_printf("!!! accept() failed\n");
				p_printf("errno: %s\n", strerror(errno));
			}
			else{
				p_printf("accept() succeeded\n");

				memset(client_address_string, 0, BUF_LEN);
				client_address_port = 0;

				if ( p_parse_sockaddr_storage_to_address_string((struct sockaddr *)&client_sockaddr, client_sockaddr_len, client_address_string, INET6_ADDRSTRLEN, &client_address_port) == 0 )
					p_printf("connection from %s:%d\n", client_address_string, client_address_port);

				if ( xdr_mode == 1 )
					p_service_xdr_server_3_4(cs);
				else{
					p_service_server_3_3(cs);
					close(cs);
				}

				close(cs);
				p_printf("client socket closed\n");
			}
		}
	}
	else if ( children == 0 ){ // on demand
		while(1){
			p_printf("waiting connection\n");

			memset(&client_sockaddr, 0, sizeof(struct sockaddr_storage));

			cs = accept(s, (struct sockaddr *)&client_sockaddr, &client_sockaddr_len);

			if ( cs < 0 ){
				p_printf("!!! accept() failed\n");
					p_printf("errno: %s\n", strerror(errno));
			}
			else{
				pid = fork();

				if ( pid < 0 ){
					p_printf("!!! fork() failed\n");
					p_printf("errno: %s\n", strerror(errno));
				}
				else if ( pid == 0 ){ // child
					close(s);
					
					p_printf("accept() succeeded\n");

					memset(client_address_string, 0, BUF_LEN);
					client_address_port = 0;

					if ( p_parse_sockaddr_storage_to_address_string((struct sockaddr *)&client_sockaddr, client_sockaddr_len, client_address_string, INET6_ADDRSTRLEN, &client_address_port) == 0 )
						p_printf("connection from %s:%d\n", client_address_string, client_address_port);

					if ( xdr_mode == 1 )
						p_service_xdr_server_3_4(cs);
					else{
						p_service_server_3_3(cs);
						close(cs);
					}

					close(cs);
					p_printf("client socket closed\n");
					exit(0);
				}
				else{	// parent
					close(cs);
				}
			}
		}
	}
	else{	// pre-forking
		for(int i = 0; i < children; i++){
			pid = fork();

			if ( pid < 0 ){
				p_printf("!!! fork() failed\n");
				p_printf("errno: %s\n", strerror(errno));
			}
			else if ( pid == 0 ){	// child
				while(1){
					p_printf("waiting connection\n");

					memset(&client_sockaddr, 0, sizeof(struct sockaddr_storage));

					cs = accept(s, (struct sockaddr *)&client_sockaddr, &client_sockaddr_len);

					if ( cs < 0 ){
						p_printf("!!! accept() failed\n");
						p_printf("errno: %s\n", strerror(errno));
					}
					else{
						p_printf("accept() succeeded\n");

						memset(client_address_string, 0, BUF_LEN);
						client_address_port = 0;

						if ( p_parse_sockaddr_storage_to_address_string((struct sockaddr *)&client_sockaddr, client_sockaddr_len, client_address_string, INET6_ADDRSTRLEN, &client_address_port) == 0 )
							p_printf("connection from %s:%d\n", client_address_string, client_address_port);

						if ( xdr_mode == 1 )
							p_service_xdr_server_3_4(cs);
						else{
							p_service_server_3_3(cs);
							close(cs);
						}

						close(cs);
						p_printf("client socket closed\n");
					}
				}
			}
			else{	// parent

			}
		}

		close(s);
		wait(NULL);
	}

	/* closing */
	Close(s);
	p_printf("close() succeeded\n");

	return 0;
}
