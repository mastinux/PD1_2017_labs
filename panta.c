#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <netdb.h>
#include <stdint.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <rpc/rpc.h>
#include <time.h>

#include "errlib.h"
#include "sockwrap.h"
#include "panta.h"

//#define ENABLE_XDR 	// comment this line to disable xdr functions
			// here and in panta.c

#ifdef ENABLE_XDR
#include "types.h"
#endif

extern char *prog_name;
extern int printable;
extern int s;
extern int continue_service;

unsigned long getTimestamp(){
	return (unsigned long)time(NULL);
}

void P_parse_args_client_3_4(int argc, char *argv[], int *xdr_mode, char **host, char **port, char *filename){
	int i;
	int port_index, filename_index;
	
	// checking number of arguments
	if (argc < 4 || argc > 5)
		err_quit("1 usage: %s [-x] <address> <port> <filename>\n", argv[0]);

	if ( argc == 4 ){	// address port filename
		*host = argv[1];

		port_index = 2;

		filename_index = 3;

		*xdr_mode = 0;
	}
	else{			// -x address port filename
		if ( argv[1][0] == '-' && argv[1][1] == 'x' && argv[1][2] == '\0')
                        *xdr_mode = 1;
                else
                        err_quit("2 usage: %s [-x] <address> <port> <filename>\n", argv[0]);

		*host = argv[2];
	
		port_index = 3;

		filename_index = 4;
	}

	// checking port
	for(i=0; i < strlen(argv[port_index]); i++){
		if ( !isdigit(argv[port_index][i]) )
			err_quit("3 usage: %s [-x] <address> <port> <filename>\n", argv[0]);
	}
	*port = argv[port_index];

	p_parse_truncate_string(argv[filename_index], strlen(argv[filename_index]), filename, FILENAME_LEN);
}

int p_upper_bound(int value, int bound){
	if ( value < 0 )
		return 1;

	if ( value < bound )
		return value;
	else
		return bound;
}

void sig_handler(int x){
	p_printf("\nentering sig_handler()\n");

	if (x == SIGINT){
		p_printf("handling SIGINT\n");
		close(s);
		exit(-1);
	}
	else if(x == SIGPIPE){	
		p_printf("handling SIGPIPE\n");
		continue_service = 0;
	}
	else{
		p_printf("!!! handling unknown signal\n");
	}
}

void p_stdin_readline(char *user_input, int buf_len){
	int i = 0;
	char c;
	char *base = user_input;

	while (	 (c = getc(stdin)) != '\n' ){
		*user_input = c;
		user_input++;
		i++;

		if ( i == (buf_len - 1) )
			return;

		if ( c == EOF ){
			memset(base, 0, buf_len);
			strcpy(base, QUIT_CMD);
			return;
		}
	}
}

int p_parse_truncate_string_old(char* src, char* dst, int len){
	int src_len, dst_len;

	memset(dst, 0, len);

	if (src == NULL)
		return -1;

	src_len = strlen(src);

	if ( src_len > (len - 1) )
		dst_len = len - 1;
	else
		dst_len = src_len;

	memcpy(dst, src, dst_len);
	dst[dst_len] = 0;

	return 0;
}

int p_parse_truncate_string(char* src, int src_len, char* dst, int dst_len){
	// NB. dst_len = len + 1 (for terminator '\0')
	memset(dst, 0, dst_len);

	for( int i = 0; i < src_len; i++){
		if (src + i == NULL)
			return -1;

		if (src[i] == '\0')
			return -2;
	}

	memcpy(dst, src, dst_len - 1);

	return 0;
}

void P_parse_truncate_string(char* src, int src_len, char* dst, int dst_len){
	if ( p_parse_truncate_string(src, src_len, dst, dst_len) < 0)
		err_sys ("(%s) error - p_parse_truncate_string() failed", prog_name);
}

void p_printf(const char *format, ...){
	if (printable){
		// as printf implementation but simplyfied
		va_list arg;

		va_start (arg, format);
		vfprintf (stdout, format, arg);
		va_end (arg);

		return;
	}
}

void p_printf_fflush(const char *format, ...){
//	original
/*	
	fprintf(stdout, "RESPONSE %d %d\n", code, file_size);
	fflush(stdout);
*/
	// as printf implementation but simplyfied
	va_list arg;

	va_start (arg, format);
	vfprintf (stdout, format, arg);
	va_end (arg);

	fflush(stdout);

	return;
}

int P_bindUDP(char *port, int protocol_family){
	uint16_t server_port;
	struct sockaddr_in server_address;
	struct sockaddr_in6 server_address6;
	socklen_t server_address_len;

	int i;

	/* port */
	for(i=0; i < strlen(port); i++)
		if ( !isdigit(port[i]) )
			err_quit("ERR: port inserted is not valid\n");
	server_port = atoi(port);

	switch(protocol_family){
		case PF_INET:{
			/* creating socket */
			s = Socket(protocol_family, SOCK_DGRAM, IPPROTO_UDP);
			p_printf("socket() succeded\n");

			/* specifying server address */
			server_address.sin_family = protocol_family;
			server_address.sin_addr.s_addr = INADDR_ANY;
			server_address.sin_port = htons(server_port);

			server_address_len = sizeof(server_address);

			/* binding */
			Bind(s, (struct sockaddr *) &server_address, server_address_len);
		}
		break;
		case PF_INET6:{
			/* creating socket */
			s = Socket(protocol_family, SOCK_DGRAM, IPPROTO_UDP);
			p_printf("socket() succeded\n");

			/* specifying server address */
			server_address6.sin6_family = protocol_family;
			server_address6.sin6_addr = in6addr_any;
			server_address6.sin6_port = htons(server_port);

			server_address_len = sizeof(server_address6);

			/* binding */
			Bind(s, (struct sockaddr *) &server_address6, server_address_len);
		}
		break;
		default:{
			err_quit("!!! unknown protocol family\n");
		}
		break;
	}

	return s;
}


int P_listenTCP(char *port, int address_family){
	int i;
	uint16_t server_port;
	struct sockaddr_in6 server_sockaddr6;
	struct sockaddr_in server_sockaddr;
	socklen_t server_sockaddr_len;

	// checking port	
	for(i=0; i < strlen(port); i++)
		if ( !isdigit(port[i]) )
			err_quit("ERR: port inserted is not valid\n");
	server_port = atoi(port);

	switch(address_family){
		case AF_INET:{
			s = Socket(address_family, SOCK_STREAM, IPPROTO_TCP);
			p_printf("socket() succeeded\n");

			memset(&server_sockaddr, 0, sizeof(struct sockaddr_in));

			server_sockaddr.sin_family = address_family;
			server_sockaddr.sin_port = htons(server_port);
			server_sockaddr.sin_addr.s_addr = INADDR_ANY;

			server_sockaddr_len = sizeof(struct sockaddr_in6);

			Bind(s, (struct sockaddr *)&server_sockaddr, server_sockaddr_len);
			p_printf("bind() succeeded\n");

			Listen(s, BACKLOG);
			p_printf("listen() succeeded\n");
		}
		break;
		case AF_INET6:{
			s = Socket(address_family, SOCK_STREAM, IPPROTO_TCP);
			p_printf("socket() succeeded\n");

			memset(&server_sockaddr6, 0, sizeof(struct sockaddr_in6));

			server_sockaddr6.sin6_family = address_family;
			server_sockaddr6.sin6_port = htons(server_port);
			server_sockaddr6.sin6_addr = in6addr_any;

			server_sockaddr_len = sizeof(struct sockaddr_in6);

			Bind(s, (struct sockaddr *)&server_sockaddr6, server_sockaddr_len);
			p_printf("bind() succeeded\n");

			Listen(s, BACKLOG);
			p_printf("listen() succeeded\n");
		}
		break;
		default:{
			err_quit("!!! unknown protocol family\n");
		}
		break;
	}


	return s;
}

int P_connectTCP(char *host, char *serv, int protocol_family) {
	struct addrinfo hints;
	struct addrinfo *res, *res0;
	int error, s;
	char *cause;

	memset(&hints, 0, sizeof(hints));

	hints.ai_socktype = SOCK_STREAM;

	hints.ai_family = protocol_family; 	// if PF_INET => IPv4
						// if PF_INET6 => IPV6
						// if AF_UNSPEC => IPv4 || IPv6

	if ((error = getaddrinfo(host, serv, &hints, &res0)))
		err_quit("%s","getaddrinfo() failed\n");

	s = -1;

	for (res = res0; res!=NULL; res = res->ai_next) {
		s = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

		if (s < 0){
			cause = "socket"; 
			continue;
		}

		if (connect(s, res->ai_addr, res->ai_addrlen) < 0) {
			cause = "connect"; 
			close(s);
			s = -1;
			continue;
		}

		break;
	}

	freeaddrinfo(res0);

	if (s < 0){
		p_printf("errno: %s\n", strerror(errno));
		err_quit("%s() failed\n", cause);
	}

	return s;
}

int P_connectUDP(char *host, char *serv, int protocol_family) {
	struct addrinfo hints;
	struct addrinfo *res, *res0;
	int error, s;
	char *cause;

	memset(&hints, 0, sizeof(hints));

	hints.ai_socktype = SOCK_DGRAM;

	hints.ai_family = protocol_family; 	// if PF_INET => IPv4
						// if PF_INET6 => IPV6
						// if AF_UNSPEC => IPv4 || IPv6

	if ((error = getaddrinfo(host, serv, &hints, &res0))){
		p_printf("%s\n", gai_strerror(error));
		err_quit("%s","getaddrinfo() failed\n");
	}

	s = -1;

	for (res = res0; res!=NULL; res = res->ai_next) {
		s = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

		if (s < 0){
			cause = "socket"; 
			continue;
		}
		
		if ( connect(s, res->ai_addr, res->ai_addrlen) < 0 ){
                	cause = "connect";
			close(s);
			s = -1;
			continue;
		}

		break;
	}

	freeaddrinfo(res0);

	if (s < 0){
		p_printf("errno: %s\n", strerror(errno));
		err_quit("%s() failed\n", cause);
	}

	return s;
}

void P_parse_args_client_1_3(int argc, char *argv[], char **host, char **port, uint16_t *x, uint16_t *y){
	/* checking number of arguments */
	if (argc != 5)
		err_quit("usage: %s <address> <port> <first n> <second n>\n", argv[0]);

	*host = argv[1];

	// checking port
	for(int i=0; i < strlen(argv[2]); i++)
		if ( !isdigit(argv[2][i]) )
			err_quit("usage: %s <address> <port> <first n> <second n>\n", argv[0]);
	*port = argv[2];

	// checking port
	for(int i=0; i < strlen(argv[3]); i++)
		if ( !isdigit(argv[3][i]) )
			err_quit("usage: %s <address> <port> <first n> <second n>\n", argv[0]);
	sscanf(argv[3], "%hd", x);

	// checking port
	for(int i=0; i < strlen(argv[4]); i++)
		if ( !isdigit(argv[4][i]) )
			err_quit("usage: %s <address> <port> <first n> <second n>\n", argv[0]);
	sscanf(argv[4], "%hd", y);
}

void P_parse_args_client_07_2014(int argc, char *argv[], char **host, char **port, uint32_t *id, uint32_t *x, uint32_t *y){
	/* checking number of arguments */
	if (argc != 6)
		err_quit("usage: %s <address> <port> <id> <first n> <second n>\n", argv[0]);

	*host = argv[1];

	// checking port
	for(int i=0; i < strlen(argv[2]); i++)
		if ( !isdigit(argv[2][i]) )
			err_quit("usage: %s <address> <port> <id> <first n> <second n>\n", argv[0]);
	*port = argv[2];

	// checking id
	for(int i=0; i < strlen(argv[3]); i++)
		if ( !isdigit(argv[3][i]) )
			err_quit("usage: %s <address> <port> <id> <first n> <second n>\n", argv[0]);
	sscanf(argv[3], "%d", id);

	// checking x
	for(int i=0; i < strlen(argv[4]); i++)
		if ( !isdigit(argv[4][i]) )
			err_quit("usage: %s <address> <port> <id> <first n> <second n>\n", argv[0]);
	sscanf(argv[4], "%d", x);

	// checking y
	for(int i=0; i < strlen(argv[5]); i++)
		if ( !isdigit(argv[5][i]) )
			err_quit("usage: %s <address> <port> <id> <first n> <second n>\n", argv[0]);
	sscanf(argv[5], "%d", y);
}

void P_parse_args_client_1_4(int argc, char *argv[], char **host, char **port, char* name){

	/* checking number of arguments */
	if (argc != 4)
		err_quit("usage: %s <address> <port> <name>\n", argv[0]);

	*host = argv[1];

	// checking port
	for(int i=0; i < strlen(argv[2]); i++)
		if ( !isdigit(argv[2][i]) )
			err_quit("usage: %s <address> <port> <name>\n", argv[0]);
	*port = argv[2];

	/* name */
	P_parse_truncate_string(argv[3], strlen(argv[3]), name, NAME_LEN);

	//p_printf("%s\n%d\n", argv[3], strlen(argv[3]));
	//p_printf("%s\n%d\n", name, strlen(name));
}

void P_parse_args_client_2_3(int argc, char *argv[], char **host, char **port, char* name){

	/* checking number of arguments */
	if (argc != 4)
		err_quit("usage: %s <address> <port> <name>\n", argv[0]);

	*host = argv[1];

	// checking port
	for(int i=0; i < strlen(argv[2]); i++)
		if ( !isdigit(argv[2][i]) )
			err_quit("usage: %s <address> <port> <name>\n", argv[0]);
	*port = argv[2];

	/* name */
	P_parse_truncate_string(argv[3], strlen(argv[3]), name, FILENAME_LEN);
}

void P_parse_args_server_1_4(int argc, char *argv[], char **port){

	/* checking number of arguments */
	if (argc != 2)
		err_quit("usage: %s <port>\n", argv[0]);

	// checking port
	for(int i=0; i < strlen(argv[1]); i++)
		if ( !isdigit(argv[1][i]) )
			err_quit("usage: %s <port>\n", argv[0]);
	*port = argv[1];
}

void P_parse_args_children_server_2_3(int argc, char *argv[], char **port, int *children){

	/* checking number of arguments */
	if (argc != 3)
		err_quit("usage: %s <port> <children>\n", argv[0]);

	// checking port
	for(int i=0; i < strlen(argv[1]); i++)
		if ( !isdigit(argv[1][i]) )
			err_quit("usage: %s <port> <children>\n", argv[0]);
	*port = argv[1];

	// checking children
	for(int i=0; i < strlen(argv[2]); i++)
		if ( !isdigit(argv[2][i]) )
			err_quit("usage: %s <port> <children>\n", argv[0]);
	*children = p_upper_bound(atoi(argv[2]), SERVER_MAX_CHLD_2_3);
}

void P_parse_args_children_server_3_4(int argc, char *argv[], char **port, int *children, int *xdr_mode){
	int port_index, children_index;

	/* checking number of arguments */
	if (argc < 3 || argc > 4)
		err_quit("usage: %s [-x] <port> <children>\n", argv[0]);

	if ( argc == 3 ){
		port_index = 1;

		children_index = 2;

		xdr_mode = 0;
	}
	else {
		if ( argv[1][0] == '-' && argv[1][1] == 'x' && argv[1][2] == '\0')
                        *xdr_mode = 1;
                else
			err_quit("usage: %s [-x] <port> <children>\n", argv[0]);

		port_index = 2;

		children_index = 3;
	}

	// checking port
	for(int i=0; i < strlen(argv[port_index]); i++)
		if ( !isdigit(argv[port_index][i]) )
			err_quit("usage: %s [-x] <port> <children>\n", argv[0]);
	*port = argv[port_index];

	// checking children
	for(int i=0; i < strlen(argv[children_index]); i++)
		if ( !isdigit(argv[children_index][i]) )
			err_quit("usage: %s [-x] <port> <children>\n", argv[0]);
	*children = p_upper_bound(atoi(argv[children_index]), SERVER_MAX_CHLD_2_3);
}

void P_parse_args_optional_children_server_3_4(int argc, char *argv[], char **port, int *children, int *xdr_mode){
	int port_index, children_index;

	p_printf("%d\n", argc);

	/* checking number of arguments */
	if (argc < 2 || argc > 4)
		err_quit("1 usage: %s [-x] <port> [<children>]\n", argv[0]);

	if ( argc == 2 ){
		*xdr_mode = 0;

		port_index = 1;

		children_index = -1;
	}
	else if ( argc == 3 ){
		if ( argv[1][0] == '-' && argv[1][1] == 'x' && argv[1][2] == '\0'){
                        *xdr_mode = 1;

			port_index = 2;

			children_index = -1;
		}
                else{
			*xdr_mode = 0;

			port_index = 1;

			children_index = 2;
		}
	}
	else {
		if ( argv[1][0] == '-' && argv[1][1] == 'x' && argv[1][2] == '\0')
                        *xdr_mode = 1;
                else
			err_quit("2 usage: %s [-x] <port> [<children>]\n", argv[0]);

		port_index = 2;

		children_index = 3;
	}

	// checking port
	for(int i=0; i < strlen(argv[port_index]); i++)
		if ( !isdigit(argv[port_index][i]) )
			err_quit("3 usage: %s [-x] <port> [<children>]\n", argv[0]);
	*port = argv[port_index];

	if ( children_index != -1 ){
		// checking children
		for(int i=0; i < strlen(argv[children_index]); i++)
			if ( !isdigit(argv[children_index][i]) )
				err_quit("4 usage: %s [-x] <port> [<children>]\n", argv[0]);
		*children = p_upper_bound(atoi(argv[children_index]), SERVER_MAX_CHLD_2_3);
	}
	else
		*children = 0;
}

void P_parse_args_optional_children_server_2_3(int argc, char *argv[], char **port, int *children){
	// if <children> is absent children = 0

	/* checking number of arguments */
	if (argc < 2 || argc > 3)
		err_quit("1 usage: %s <port> [<children>]\n", argv[0]);

	// checking port
	for(int i=0; i < strlen(argv[1]); i++)
		if ( !isdigit(argv[1][i]) )
			err_quit("2 usage: %s <port> [<children>]\n", argv[0]);
	*port = argv[1];

	if ( argc == 3 ){
		// checking port
		for(int i=0; i < strlen(argv[2]); i++)
			if ( !isdigit(argv[2][i]) )
				err_quit("3 usage: %s <port> [<children>]\n", argv[0]);
		*children = p_upper_bound(atoi(argv[2]), SERVER_MAX_CHLD_2_3);
	}
	else
		*children = 0;
}

void P_parse_args_server_3_4(int argc, char *argv[], char **port, int *xdr_mode){
	int port_index;

	/* checking number of arguments */
	if (argc < 2 || argc > 3)
		err_quit("usage: %s [-x] <port>\n", argv[0]);

	if(argc == 2){
		*xdr_mode = 0;

		port_index = 1;
	}
	else{
		if ( argv[1][0] == '-' && argv[1][1] == 'x' && argv[1][2] == '\0')
                        *xdr_mode = 1;
                else
                        err_quit("usage: %s [-x] <port>\n", argv[0]);

		port_index = 2;
	}

	// checking port
	for(int i=0; i < strlen(argv[port_index]); i++)
		if ( !isdigit(argv[port_index][i]) )
			err_quit("usage: %s [-x] <port>\n", argv[0]);
	*port = argv[port_index];
}

void P_parse_args_client_2_4(int argc, char *argv[], char **host, char **port, int *x, int *y){
	/* checking number of arguments */
	if (argc != 5)
		err_quit("usage: %s <address> <port> <first n> <second n>\n", argv[0]);

	*host = argv[1];

	// checking port
	for(int i=0; i < strlen(argv[2]); i++)
		if ( !isdigit(argv[2][i]) )
			err_quit("usage: %s <address> <port> <first n> <second n>\n", argv[0]);
	*port = argv[2];

	// checking port
	for(int i=0; i < strlen(argv[3]); i++)
		if ( !isdigit(argv[3][i]) )
			err_quit("usage: %s <address> <port> <first n> <second n>\n", argv[0]);
	sscanf(argv[3], "%d", x);

	// checking port
	for(int i=0; i < strlen(argv[4]); i++)
		if ( !isdigit(argv[4][i]) )
			err_quit("usage: %s <address> <port> <first n> <second n>\n", argv[0]);
	sscanf(argv[4], "%d", y);
}

void P_parse_args_server_3_3(int argc, char *argv[], char **port, int *children){

	/* checking number of arguments */
	if (argc != 3)
		err_quit("usage: %s <port> <children>\n", argv[0]);

	// checking port
	for(int i=0; i < strlen(argv[1]); i++)
		if ( !isdigit(argv[1][i]) )
			err_quit("usage: %s <port> <children>\n", argv[0]);
	*port = argv[1];

	// checking children
	for(int i=0; i < strlen(argv[2]); i++)
		if ( !isdigit(argv[2][i]) )
			err_quit("usage: %s <port> <children>\n", argv[0]);
	*children = p_upper_bound(atoi(argv[2]), SERVER_MAX_CHLD_3_3);
}

void P_service_client_1_3(int s, uint16_t x, uint16_t y){
	uint16_t result;
        char request[BUF_LEN] = "", response[BUF_LEN] = "", tmp[BUF_LEN];
	int res;

	memset(request, 0, BUF_LEN);
	memset(tmp, 0, BUF_LEN);

	/* preparing request string */
        sprintf(request, "%hu", x);
        strcat(request, " ");
	sprintf(tmp, "%hu", y);
	strcat(request, tmp);
	strcat(request, "\r\n");

	/* sending request */
	p_printf("sending request\n");
	p_printf("%s\n", request);
        Send(s, request, strlen(request), 0);
        p_printf("request sent\n");

	/* getting response */
	p_printf("getting response\n");
        res = p_socket_readline(s, response, BUF_LEN);
        p_printf("response get\n");

	if ( res == -2 ){
		p_printf("!!! response too long\n");
	}
	else if ( res == -1 ){
		p_printf("!!! recv() failed\n");
		p_printf("!!! errno: %s\n", strerror(errno));
	}
	else if ( res == 0 ){
		p_printf("!!! server shutdown\n");
	}
	else{
		/* parsing response */
		if ( (sscanf(response, "%hd", &result)) == 1 ){
			/* numeric response*/
			p_printf("response: %u\n", result);
		}
		else{
			/* overflow or incorrect operands */
			p_printf("response: %s", response);
		}
	}
}

void p_service_server_1_3(int cs){
	uint16_t x, y, result;
        char request[BUF_LEN] = "", response[BUF_LEN] = "";
	int res;
	
	res = p_socket_readline(cs, request, BUF_LEN);
	memset(response, 0, BUF_LEN);

	if ( sscanf(request, "%hu %hu\r\n", &x, &y) != 2 ){
		p_printf("!!! bad request\n");
		sprintf(response, "incorrect operands\r\n");
	}
	else{
		p_printf("good request\n%hu %hu\n", x, y);
		result = x + y;

		if (result < x || result < y){
			p_printf("!!! overflow\n");
			sprintf(response, "overflow\r\n");
		}
		else{
			p_printf("result\n%hu\n", result);
			sprintf(response, "%hu\r\n", result);
		}
	}

	res = send(cs, response, strlen(response), 0);

	if (res < strlen(response))
		p_printf("send() failed\n");
	else
		p_printf("send() succeeded\n");
}

void P_service_client_07_2014(int s, uint32_t id, uint32_t x, uint32_t y){
        char request[BUF_LEN], response[BUF_LEN] = "", tmp[BUF_LEN] = "";
	int res;
	int int_out;
	uint32_t result, result_id;

	memset(request, 0, BUF_LEN);

	id = htonl(id);
	x = htonl(x);
	y = htonl(y);

	/* preparing request string */
        sprintf(tmp, "%d", id);
        strcat(request, tmp);

        sprintf(tmp, "%d", x);
        strcat(request, tmp);

        sprintf(tmp, "%d", y);
        strcat(request, tmp);

	/* sending request */
	p_printf("sending request\n");
        Send(s, request, strlen(request), 0);
        p_printf("request sent\n");

        struct timeval tval;
        fd_set cset;

	/* preparing select */
	FD_ZERO(&cset);
        FD_SET(s, &cset);
        tval.tv_sec = 3;
        tval.tv_usec = 0;

	// selecting
	p_printf("waiting for datagram\n");
        if ( Select(FD_SETSIZE, &cset, NULL, NULL, &tval) == 0 ){
		/* no response */
		p_printf("!!! no response after %d seconds\n", 3);

		int_out = 2;
	}
        else{
		/* getting response */
		p_printf("getting response\n");
		res = p_socket_readline(s, response, BUF_LEN);
		p_printf("response get\n");

		if ( res == -2 ){
			p_printf("!!! response too long\n");

			int_out = 1;
		}
		else if ( res == -1 ){
			p_printf("!!! recv() failed\n");
			p_printf("!!! errno: %s\n", strerror(errno));

			int_out = 1;
		}
		else if ( res == 0 ){
			p_printf("!!! server shutdown\n");

			int_out = 1;
		}
		else{
			memset(tmp, 0, BUF_LEN);
			memcpy(tmp, response, sizeof(uint32_t));
			sscanf(tmp, "%d", &result_id);

			result_id = ntohl(result_id);

			if ( result_id == id ){
				memset(tmp, 0, BUF_LEN);
				memcpy(tmp, response + sizeof(uint32_t), sizeof(uint32_t));
				sscanf(tmp, "%d", &result);

				result = ntohl(result);

				int_out = 0;
			}
			else
				int_out = 1;
		}

        }

	FILE *fd;
	char filename[] = "output.txt";

	/* opening file */
	p_printf("opening file %s...\n", filename);
	if( (fd = fopen(filename, "rb")) == NULL){
		p_printf("fopen() failed\n");
		return;
	}
	p_printf("file %s opened\n", filename);
	
	memset(tmp, 0, BUF_LEN);
	sprintf(tmp, "%d", int_out);
	fwrite(tmp, sizeof(int), 1, fd);
	
	if (int_out == 0){
		memset(tmp, 0, BUF_LEN);
		sprintf(tmp, "%hu", result);
		fwrite(tmp, sizeof(uint32_t), 1, fd);
	}

	/* closing file */
	p_printf("closing file %s...\n", filename);
	fclose(fd);
	p_printf("file %s closed\n", filename);
}

void p_service_server_1_4(int s, int limiting){
        char request[NAME_LEN], response[NAME_LEN];

	struct sockaddr_storage client_address;
	char client_address_string[INET6_ADDRSTRLEN];
	uint16_t client_address_port;
	socklen_t client_address_len;

	struct my_client clients[MAX_MANAGED_CLIENTS_2_2];
	int clients_len = 0;
	int clients_index = 0;

	client_address_len = sizeof(client_address);

	memset(request, 0, NAME_LEN);

	int request_len;

	while(1){
		p_printf("waiting for datagram\n");

		/* receiving */
		if ( (request_len = recvfrom(s, request, NAME_LEN, 0, (struct sockaddr *) &client_address, &client_address_len)) < 0 ){
			p_printf("error while receiving data\n");
		}
		else{
			p_printf("datagram received\n");

			//TEST
			//sleep(14);

			for(int i = 0; i < request_len; i++)
				p_printf("%c", request[i]);
			p_printf("\n%d\n", request_len);
		
			if (!(limiting == 1)||p_check_clients(clients, &clients_len, &clients_index, MAX_MANAGED_CLIENTS_2_2, client_address, MAX_REQS_PER_CLIENT_2_2) == 0){
	
				if ( p_parse_sockaddr_storage_to_address_string((struct sockaddr *)&client_address, client_address_len, client_address_string, INET6_ADDRSTRLEN, &client_address_port) == 0 )
					p_printf("from %s:%d\n", client_address_string, client_address_port);

				int parse_res;
				if ( (parse_res = p_parse_truncate_string(request, request_len, response, NAME_LEN)) == 0 ){
					p_printf("%s\n", response);
					p_printf("%d\n", strlen(response));
					p_printf("sending datagram\n");

					/* sending */
					if (sendto(s, response, strlen(response), 0, (struct sockaddr *) &client_address, client_address_len)<0)
						p_printf("error while sending datagram\n");
					else
						p_printf("datagram sent\n");
				}
				else {
					p_printf("datagram contains NULL or a terminator\n");
				}
			}
		}
	}
}

int P_service_client_1_4(int s, char* request){
	char response[NAME_LEN];
	int t = TIMEOUT_1_4;

	memset(response, 0, NAME_LEN);

	int request_len = strlen(request);

	/* sending */
        p_printf("sending datagram\n%s\n%d\n", request, request_len);
	Send(s, request, request_len, 0);
        p_printf("datagram sent\n");
	
        struct timeval tval;
        fd_set cset;
	/* preparing select */
	FD_ZERO(&cset);
        FD_SET(s, &cset);
        tval.tv_sec = t;
        tval.tv_usec = 0;

	/* selecting */
        p_printf("waiting for datagram\n");
        if ( Select(FD_SETSIZE, &cset, NULL, NULL, &tval) == 0 ){
		/* no response */
		p_printf("!!! no response after %d seconds\n", t);
		return -1;
	}
        else{
		/* response get*/
		Recv(s, response, NAME_LEN, 0);
	        p_printf("datagram received\n");
		p_printf("%s\n%d\n", response, strlen(response));
		return 0;
        }
}

void p_service_server_2_3(int cs){
	char request[BUF_LEN];
	char command[CMD_LEN], filename[FILENAME_LEN];

	int res;
		
	memset(request, 0, BUF_LEN);
	memset(command, 0, CMD_LEN);
	memset(filename, 0, FILENAME_LEN);

	res = p_socket_readline(cs, request, BUF_LEN);

	if ( res == -2 ){
		p_printf("!!! request too long\n");
		p_send_err(cs);
		return;
	}
	else if ( res == -1 ){
		p_printf("!!! recv() failed\n");
		p_printf("!!! errno: %s\n", strerror(errno));
		return;
	}
	else if ( res == 0 ){
		p_printf("!!! client shutdown\n");
		return;
	}
	else{
		if ( p_parse_client_request(request, command, CMD_LEN - 1, filename, FILENAME_LEN) < 0 ){
			/* -1 as both CMD_LEN and FILENAME_LEN have +1 for terminator */
			p_printf("!!! bad request received\n");
			p_send_err(cs);
		}
		else{
			if ( strcmp(command, GET_CMD) == 0 ){
				p_printf("request for file: %s\n", filename);

				p_search_send_file(cs, filename);
			}
			else{
				//quit
				p_printf("received QUIT\\r\\n\n");
			}
		}
	}
}

void p_service_server_3_3(int cs){
	char request[BUF_LEN];
	char command[CMD_LEN], filename[FILENAME_LEN];

	int res;

        fd_set cset;
        struct timeval tval;

	while( strcmp(command, QUIT_CMD) != 0 ){
		memset(request, 0, BUF_LEN);
		memset(command, 0, CMD_LEN);
		memset(filename, 0, FILENAME_LEN);

		/* preparing select */
		FD_ZERO(&cset);
		FD_SET(cs, &cset);
		tval.tv_sec = TIMEOUT;
		tval.tv_usec = 0;

		res = select(FD_SETSIZE, &cset, NULL, NULL, &tval);

		if( res < 0 ){
			// select error
			p_printf("!!! select() failed\n");
			break;
		}
		if ( res == 0 ){
			// timeout
			p_printf("!!! connection timeout\n");
			break;
		}
		else{
			res = p_socket_readline(cs, request, BUF_LEN);

			if ( res == -2 ){
				p_printf("!!! request too long\n");
				p_send_err(cs);
				break;
			}
			else if ( res == -1 ){
				p_printf("!!! recv() failed\n");
				p_printf("!!! errno: %s\n", strerror(errno));
				break;
			}
			else if ( res == 0 ){
				p_printf("!!! client shutdown\n");
				break;
			}
			else{
				if ( p_parse_client_request(request, command, CMD_LEN - 1, filename, FILENAME_LEN) < 0 ){
					/* -1 as both CMD_LEN and FILENAME_LEN have +1 for terminator */
					p_printf("!!! bad request received\n");
					p_send_err(cs);
				}
				else{
					if ( strcmp(command, GET_CMD) == 0 ){
						p_printf("request for file: %s\n", filename);

						p_search_send_file(cs, filename);
					}
					else{
						//quit
						p_printf("received QUIT\\r\\n\n");
					}
				}
			}
		}
	}
}

void p_service_client_2_3(int s, char *filename){
	char command[BUF_LEN];
	uint32_t file_size, file_mtime;

	P_send_get(s, filename, BUF_LEN);

	if (p_parse_server_response(s, command, &file_size, &file_mtime, BUF_LEN) == 0){
		if ( strcmp(command, ERR_CMD) == 0 ){
			p_printf("-ERR\\r\\n response received\n");
		}
		else if ( strcmp(command, OK_CMD) == 0 ){
			p_printf("+OK\\r\\nB1B2B3B4T1T2T3T4 response received\n");

			p_printf("file size: %u\n", file_size);
			p_printf("file last modification time: %u\n", file_mtime);

			p_recv_file(s, filename, file_size);
		}
		else{
			p_printf("!!! bad response received\n");
		}
	}
}

void p_service_client_3_2(int s){
	// type-ahead
	char user_input[BUF_LEN], request[BUF_LEN], response[BUF_LEN], command[BUF_LEN], filename[BUF_LEN];
	uint32_t file_size, file_mtime, tot_b_red;

	FILE* fd;

        fd_set cset;

	int receiving_file = 0;
	int quit_after_recv_file = 0;
	continue_service = 1;
	
	memset(user_input, 0, BUF_LEN);

	while( continue_service ){
		/* preparing select */
		FD_ZERO(&cset);
		FD_SET(s, &cset);
		FD_SET(0, &cset); // 0 as stdin

		Select(FD_SETSIZE, &cset, NULL, NULL, NULL);

		if ( FD_ISSET(s, &cset) ){
			// data from socket
			//p_printf("socket selected\n");

			if ( receiving_file == 0 ){
				// receiving first part of response
				//p_printf("receiving first part of response\n");
				if (p_parse_server_response(s, command, &file_size, &file_mtime, BUF_LEN) == 0){
					if ( strcmp(command, ERR_CMD) == 0 ){
						p_printf("-ERR\\r\\n response received\n");
						continue_service = 0;
					}
					else if ( strcmp(command, OK_CMD) == 0 ){
						p_printf("+OK\\r\\nB1B2B3B4T1T2T3T4 response received\n");

						p_printf("file size: %u\n", file_size);
						p_printf("file last modification time: %u\n", file_mtime);

						/* opening file*/
						p_printf("creating file %s\n", filename);
						if( (fd = fopen(filename, "wb")) == NULL)
							err_quit("fopen() failed\n");
						p_printf("file %s created\n", filename);

						receiving_file = 1;

						/* receiving file */
						p_printf("receiving file %s...\n", filename);

						tot_b_red = 0;
					}
					else{
						p_printf("!!! bad response received\n");
					}
				}
				else
					continue_service = 0;
			}
			else{
				// receiving partial data of file
				//p_printf("receiving partial data of file\n");
				if ( tot_b_red < file_size && continue_service )
					p_recv_fill_file(s, response, BUF_LEN, fd, file_size, &tot_b_red);

				if ( tot_b_red == file_size ){
					fclose(fd);

					if ( continue_service == 0 )
						p_printf("!!! server shutdown\n");
					else{
						p_printf("file %s received\n", filename);
						receiving_file = 0;

						if ( quit_after_recv_file == 1 ){
							P_send_quit(s);
							continue_service = 0;
						}
					}
				}
			}
		}
		else{
			// data from stdin
			//p_printf("stdin selected\n");
			p_stdin_readline(user_input, BUF_LEN);

			if ( strcmp(user_input, QUIT_CHAR) == 0 ){
				if ( !receiving_file ){
					continue_service = 0;
					P_send_quit(s);
				}
				else
					quit_after_recv_file = 1;
			}
			else if ( strcmp(user_input, ABORT_CHAR) == 0 ){
				continue_service = 0;
				p_printf("aborted\n");
			}
			else if ( sscanf(user_input, "%s %s", command, filename) == 2 ){
				if ( strcmp(command, GET_CMD) == 0 ){
					P_parse_truncate_string(filename, strlen(filename), request, BUF_LEN - strlen(GET_CMD) - strlen(" ") - strlen(CMD_TAIL));
					P_send_get(s, request, BUF_LEN);

					receiving_file = 0;
				}
				else
					p_printf("!!! bad command 1\n");
			}
			else{
				p_printf("!!! bad command 2\n");
			}

			memset(user_input, 0, BUF_LEN);
		}
	}
}

int p_parse_sockaddr_storage_to_address_string(const struct sockaddr *sa, size_t maxlen, char *address_string, int string_len, uint16_t *address_port){
	int exit_code = 0;
	uint16_t tmp_port;

	memset(address_string, 0, string_len);

	p_printf("switching %d\n", sa->sa_family);

	switch(sa->sa_family) {
        	case AF_INET:{
			p_printf("AF_INET\n");
			inet_ntop(AF_INET, &(((struct sockaddr_in *)sa)->sin_addr), address_string, maxlen);
			tmp_port = (((struct sockaddr_in *)sa)->sin_port);
			*address_port = ntohs(tmp_port);
		}
		break;

		case AF_INET6:{
			p_printf("AF_INET6\n");
			inet_ntop(AF_INET6, &(((struct sockaddr_in6 *)sa)->sin6_addr), address_string, maxlen);
			tmp_port = (((struct sockaddr_in6 *)sa)->sin6_port);
			*address_port = ntohs(tmp_port);
		}
		break;

		default:{
			p_printf("!!! Unknown AF\n");
			strcpy(address_string, "Unknown AF");
			exit_code = -1;
		}
		break;
	}

	return exit_code;
}

int p_check_clients(struct my_client clients[], int *clients_len, int *clients_index, int max_clients_len, struct sockaddr_storage new_client_storage, int max_reqs_per_client){
	// client_index is used as the head of the queue
	int new_client_index;

	for (int i = 0; i < *clients_len; i++){
		if ( clients[i].address.ss_family == new_client_storage.ss_family ) {
			switch( new_client_storage.ss_family ){
				case AF_INET:{
					//p_printf("AF_INET");
					struct sockaddr_in *x = (struct sockaddr_in *)&new_client_storage;
					struct sockaddr_in *y = (struct sockaddr_in *)&clients[i].address;

					unsigned char *xx = (unsigned char *)&x->sin_addr.s_addr;
					unsigned char *yy = (unsigned char *)&y->sin_addr.s_addr;

					if( memcmp(xx, yy, 4*sizeof(unsigned char)) == 0 ){
						if (clients[i].requests < max_reqs_per_client){
							(clients[i].requests)++;

							p_printf("old client served\n");

							return 0;
						}
						else{
							p_printf("!!! client exceeded max_reqs_per_client\n");

							return -1;
						}
					}
				}
				break;

				case AF_INET6:{
					//p_printf("AF_INET");
					struct sockaddr_in6 *x = (struct sockaddr_in6 *)&new_client_storage;
					struct sockaddr_in6 *y = (struct sockaddr_in6 *)&clients[i].address;

					unsigned char *xx = (unsigned char *)&x->sin6_addr.s6_addr;
					unsigned char *yy = (unsigned char *)&y->sin6_addr.s6_addr;

					if( memcmp(xx, yy, 16*sizeof(unsigned char)) == 0 ){
						if (clients[i].requests < max_reqs_per_client){
							(clients[i].requests)++;

							p_printf("old client served\n");

							return 0;
						}
						else{
							p_printf("!!! client exceeded max_reqs_per_client\n");

							return -1;
						}
					}
				}
				break;
			}
		}
	}

	if ( *clients_len == max_clients_len ){
		if ( *clients_index == max_clients_len )
			*clients_index = 0;

		new_client_index = *clients_index;
		*clients_index += 1;
	}
	else{
		new_client_index = *clients_len;
		*clients_len += 1;
	}

	struct my_client c;
	c.address = new_client_storage;
	c.requests = 1;

	clients[new_client_index] = c;

	p_printf("new client served\n");

	return 0;
}

void P_send_quit(int s){
	char request[BUF_LEN];
	
	memset(request, 0, BUF_LEN);

	strcpy(request, QUIT_CMD);
	strcat(request, CMD_TAIL);

	Send(s, request, strlen(request), 0);
	p_printf("QUIT\\r\\n sent\n");
}

void P_send_get(int s, char* filename, int buf_len){
	char request[BUF_LEN];

	memset(request, 0, buf_len);

	strcpy(request, GET_CMD);
	strcat(request, " ");
	strcat(request, filename);
	strcat(request, CMD_TAIL);

	Send(s, request, strlen(request), 0);
	p_printf("GET %s\\n\\r sent\n", filename);
}

int p_socket_readline(int s, char *buffer, int buffer_len){
	char c = 'x';	// init value
	int res;
	int c_red = 0;

	memset(buffer, 0, buffer_len);

	while( c != '\n' ){
		res = recv(s, &c, 1, 0);

		if ( res < 0 )
			return -1;
		else if ( res == 0 )
			return 0;
		else{
			*buffer = c;
			buffer++;
			c_red++;

			if( c_red > buffer_len )
				return -2;
		}
	}

	return c_red;
}

int p_parse_client_request(char *request, char *command, int command_len, char *filename, int filename_len){
	// return -1 in case of bad request
	if ( request == NULL ){
		p_printf("received empty request\n");
		return -1;
	}

	//p_printf("request len %d\n", strlen(request));
	//p_printf("available len %d\n", command_len + filename_len);

	if ( strlen(request) >= command_len + filename_len ){
		p_printf("received too long request\n");
		return -1;
	}

	if ( (sscanf(request, "%s %s\r\n", command, filename)) == 2 ){
		if ( strcmp(command, GET_CMD) == 0 ){
			if (strlen(filename) == 0)
				return -1;
			else	
				return 0;
		}
		else
			return -1;
	}

	if ( (sscanf(request, "%s\r\n", command)) == 1 ){
		if ( strcmp(command, QUIT_CMD) == 0 )
			return 0;
		else
			return -1;
	}

	return -1;
}

int p_send_err(int s){
	char request[BUF_LEN];
	
	memset(request, 0, BUF_LEN);

	strcpy(request, ERR_CMD);
	strcat(request, CMD_TAIL);

	if ( send(s, request, strlen(request), 0) == strlen(request) ) {
		p_printf("ERR\\r\\n sent\n");
		return 0;
	}
	else{
		p_printf("errno: %s\n", strerror(errno));
		return -1;
	}
}

int p_send_file(int s, char *filename, uint32_t file_size){
	FILE *fd;
	char response[BUF_LEN];
	uint32_t b_red, tot_b_transferred;

	continue_service = 1;

	/* opening file */
	p_printf("opening file %s\n", filename);
	if( (fd = fopen(filename, "rb")) == NULL){
		p_printf("fopen() failed\n");
		return -1;
	}
	p_printf("file %s opened\n", filename);

	p_printf("transferring file %s...\n", filename);
	memset(response, 0, BUF_LEN);
	tot_b_transferred = 0;

	continue_service = 1;

	/* reading file */
	while( !feof(fd) && continue_service ){
		fread(response, BUF_LEN - 1, 1, fd);
		b_red = strlen(response);

		// sending response
		writen(s, response, b_red);

		tot_b_transferred += b_red;

		p_printf("%s now:%u\ttran: %u\trem: %u\ttot: %u\n", filename, b_red, tot_b_transferred, file_size - tot_b_transferred, file_size);
		memset(response, 0, BUF_LEN);

		//TEST
		//sleep(1);
	}

	/* closing file */
	fclose(fd);

	if ( continue_service == 0 ){
		p_printf("!!! connection with client lost\n");
		return 0;
	}
	else{
		p_printf("file %s transferred\n", filename);
		return -1;
	}
}

int p_search_send_file(int s, char *filename){
	struct stat file_info;
	uint32_t file_size, file_mtime;

	/* searching file */
	if( stat(filename, &file_info) != 0 ){
		p_printf("file not found\n");
		/* file does not exist */
		p_send_err(s);

		return -1;
	}
	else{
		p_printf("file found\n");
		/* file exists */
		file_size = file_info.st_size;
		file_mtime = file_info.st_mtime;

		p_printf("file size: %u\n", file_size);
		p_printf("file last modification time: %u\n", file_mtime);

		p_send_ok(s, file_size, file_mtime);

		p_send_file(s, filename, file_size);

		return 0;
	}
}

int p_send_ok(int s, uint32_t file_size, uint32_t file_mtime){
	char response[BUF_LEN];
	uint32_t file_size_n, file_mtime_n;
	int base = 0;

	memset(response, 0, BUF_LEN);

	strcat(response, OK_CMD);
	strcat(response, CMD_TAIL);

	base = strlen(response);

	file_size_n = htonl(file_size);
	file_mtime_n = htonl(file_mtime);

	memcpy(response + base, &file_size_n, sizeof(uint32_t));
	memcpy(response + base + sizeof(uint32_t), &file_mtime_n, sizeof(uint32_t));

	if ( send(s, response, base + sizeof(uint32_t) + sizeof(uint32_t), 0) == (base + sizeof(uint32_t) + sizeof(uint32_t)) ) {
		p_printf("+OK\\r\\nB1B2B3B4T1T2T3T4 sent\n");
		return 0;
	}
	else{
		p_printf("errno: %s\n", strerror(errno));
		return -1;
	}
}

int p_parse_server_response(int s, char *command, uint32_t *file_size, uint32_t *file_mtime, int buf_len){
	// if bad response => command is empty

	int res;
	char response[BUF_LEN];
	uint32_t file_size_n, file_mtime_n;
	
	memset(command, 0, buf_len);
	memset(response, 0, BUF_LEN);

	res = recv(s, response, strlen(OK_CMD) + strlen(CMD_TAIL) + 2*sizeof(uint32_t), 0);

	if ( res < 0 )
		p_printf("errno: %s\n", strerror(errno));
	else if ( res == 0 )
		p_printf("server shutdown\n");
	else{
		strncpy(command, response, strlen(OK_CMD));

		if ( strcmp(command, OK_CMD) == 0 ){
			memcpy(&file_size_n, response + strlen(OK_CMD) + strlen(CMD_TAIL), sizeof(uint32_t));
			memcpy(&file_mtime_n, response + strlen(OK_CMD) + strlen(CMD_TAIL) + sizeof(uint32_t), sizeof(uint32_t));

			*file_size = ntohl(file_size_n);
			*file_mtime = ntohl(file_mtime_n);
		}
		else{
			strncpy(command, response, strlen(ERR_CMD));

			if ( strcmp(command, ERR_CMD) != 0 )
				memset(command, 0, buf_len);
		}

		return 0;
	}

	return -1;
}

void p_recv_fill_file(int s, char *response, int buf_len, FILE* fd, uint32_t file_size, uint32_t *tot_b_red){
	uint32_t b_to_read = buf_len;
	uint32_t b_red;

	memset(response, 0, buf_len);

	if( file_size - *tot_b_red < b_to_read )
   		b_to_read = file_size - *tot_b_red;

	b_red = recv(s, response, b_to_read, 0);

	if ( b_red == 0 )
		continue_service = 0;

	*tot_b_red += b_red;
	fwrite(response, b_red, 1, fd);

	//p_printf("now: %u\trec: %u\trem: %u\ttot: %u\n", b_red, *tot_b_red, file_size - *tot_b_red, file_size);
}

int p_recv_file(int s, char *filename, uint32_t file_size){
	char response[BUF_LEN];
	FILE *fd;
	uint32_t tot_b_red;
	
	/* opening file*/
	p_printf("creating file %s\n", filename);
	if( (fd = fopen(filename, "wb")) == NULL)
		err_quit("fopen() failed\n");
	p_printf("file %s created\n", filename);

	/* receiving file */
	p_printf("receiving file %s...\n", filename);

	tot_b_red = 0;

	while ( tot_b_red < file_size && continue_service )
		p_recv_fill_file(s, response, BUF_LEN, fd, file_size, &tot_b_red);

	/* closing file */
	fclose(fd);

	if ( continue_service == 0 )
		p_printf("!!! server shutdown\n");
	else
		p_printf("file %s received\n", filename);

	return 0;
}

void P_service_client_2_4(int s, int x, int y){
	int result;

	//-----------------------------------------------------------//
	//---------------------XDR-INIT------------------------------//
	//------------------READ-AND-WRITE---------------------------//
	//-----------------------------------------------------------//
	XDR write_xdrs, read_xdrs;
	FILE *write_stream, *read_stream;
	// preparing output xdr stream
	if ( (write_stream = fdopen(s, "w")) == NULL )
		err_quit("(%s) error - fdopen() failed", prog_name);
	xdrstdio_create(&write_xdrs, write_stream, XDR_ENCODE);
	// preparing input xdr stream
	if ( (read_stream = fdopen(s, "r")) == NULL )
		err_quit ("(%s) error - fdopen() failed", prog_name);
	xdrstdio_create(&read_xdrs, read_stream, XDR_DECODE);
	//-----------------------------------------------------------//
	//-----------------------------------------------------------//
	//-----------------------------------------------------------//
	//-----------------------------------------------------------//

	/* sending request */
	p_printf("sending request\n");
	// putting operands
	if ( !xdr_int(&write_xdrs, &x) )
		err_quit("(%s) error - xdr_int() failed", prog_name);
	if ( !xdr_int(&write_xdrs, &y) )
		err_quit("(%s) error - xdr_int() failed", prog_name);

	//-----------------------------------------------------------//
	//--------------------------XDR------------------------------//
	//------------------FLUSHING-WRITE-STREAM--------------------//
	//-----------------------------------------------------------//
	// forcing write on output xdr stream
	fflush(write_stream);
	//-----------------------------------------------------------//
	//-----------------------------------------------------------//
	//-----------------------------------------------------------//
	//-----------------------------------------------------------//

        p_printf("request sent\n");
	/* getting response */
	p_printf("getting response\n");
	if ( ! xdr_int(&read_xdrs, &result) )
		err_quit ("(%s) error - xdr_int() failed", prog_name);
        p_printf("response get\n");

	p_printf("result: %d\n", result);

	//-----------------------------------------------------------//
	//---------------------XDR-CLEAN-UP--------------------------//
	//--------------------READ-AND-WRITE-------------------------//
	//-----------------------------------------------------------//
	xdr_destroy(&write_xdrs);
	// closing output stream
	fclose(write_stream);
	// freeing intput xdr stream
	xdr_destroy(&read_xdrs);
	// closing input stream
	// NB: Close read streams only after writing operations have also been done
	fclose(read_stream);
	//-----------------------------------------------------------//
	//-----------------------------------------------------------//
	//-----------------------------------------------------------//
	//-----------------------------------------------------------//
}

void p_service_server_2_4(int s){
	int result, x, y;

	//-----------------------------------------------------------//
	//---------------------XDR-INIT------------------------------//
	//------------------READ-AND-WRITE---------------------------//
	//-----------------------------------------------------------//
	XDR write_xdrs, read_xdrs;
	FILE *write_stream, *read_stream;
	// preparing output xdr stream
	if ( (write_stream = fdopen(s, "w")) == NULL )
		err_quit("(%s) error - fdopen() failed", prog_name);
	xdrstdio_create(&write_xdrs, write_stream, XDR_ENCODE);
	// preparing input xdr stream
	if ( (read_stream = fdopen(s, "r")) == NULL )
		err_quit ("(%s) error - fdopen() failed", prog_name);
	xdrstdio_create(&read_xdrs, read_stream, XDR_DECODE);
	//-----------------------------------------------------------//
	//-----------------------------------------------------------//
	//-----------------------------------------------------------//
	//-----------------------------------------------------------//

	/* sending request */
	p_printf("getting request\n");
	// putting operands
	if ( !xdr_int(&read_xdrs, &x) )
		err_quit("(%s) error - xdr_int() failed", prog_name);
	if ( !xdr_int(&read_xdrs, &y) )
		err_quit("(%s) error - xdr_int() failed", prog_name);
        p_printf("request get\n");

	p_printf("%d %d\n", x, y);
	result = x + y;
	p_printf("%d\n", result);

	/* getting response */
	p_printf("sending response\n");
	if ( ! xdr_int(&write_xdrs, &result) )
		err_quit ("(%s) error - xdr_int() failed", prog_name);
        p_printf("response sent\n");

	//-----------------------------------------------------------//
	//--------------------------XDR------------------------------//
	//------------------FLUSHING-WRITE-STREAM--------------------//
	//-----------------------------------------------------------//
	// forcing write on output xdr stream
	fflush(write_stream);
	//-----------------------------------------------------------//
	//-----------------------------------------------------------//
	//-----------------------------------------------------------//
	//-----------------------------------------------------------//

	//-----------------------------------------------------------//
	//---------------------XDR-CLEAN-UP--------------------------//
	//--------------------READ-AND-WRITE-------------------------//
	//-----------------------------------------------------------//
	xdr_destroy(&write_xdrs);
	// closing output stream
	fclose(write_stream);
	// freeing intput xdr stream
	xdr_destroy(&read_xdrs);
	// closing input stream
	// NB: Close read streams only after writing operations have also been done
	fclose(read_stream);
	//-----------------------------------------------------------//
	//-----------------------------------------------------------//
	//-----------------------------------------------------------//
	//-----------------------------------------------------------//
}

#ifdef ENABLE_XDR

void Send_xdr_quit(int s, XDR *write_xdrs, FILE *write_stream){
	message msg;

	memset(&msg, 0, sizeof(message));

	msg.tag = XDR_QUIT;

	if ( !xdr_message(write_xdrs, &msg) )
		err_quit("(%s) error - xdr_message() failed", prog_name);

	//-----------------------------------------------------------//
	//--------------------------XDR------------------------------//
	//------------------FLUSHING-WRITE-STREAM--------------------//
	//-----------------------------------------------------------//
	// forcing write on output xdr stream
	fflush(write_stream);
	//-----------------------------------------------------------//
	//-----------------------------------------------------------//
	//-----------------------------------------------------------//
	//-----------------------------------------------------------//
	
	p_printf("XDR QUIT sent\n");
}

int p_send_xdr_err(int cs, XDR *write_xdrs, FILE *write_stream){
	message msg;

	memset(&msg, 0, sizeof(message));

	msg.tag = XDR_ERR;

	if ( !xdr_message(write_xdrs, &msg) ){
		p_printf("!!! xdr_message() failed\n");
		return -1;
	}

	//-----------------------------------------------------------//
	//--------------------------XDR------------------------------//
	//------------------FLUSHING-WRITE-STREAM--------------------//
	//-----------------------------------------------------------//
	// forcing write on output xdr stream
	fflush(write_stream);
	//-----------------------------------------------------------//
	//-----------------------------------------------------------//
	//-----------------------------------------------------------//
	//-----------------------------------------------------------//
	
	p_printf("XDR ERR sent\n");

	return 0;
}

void Send_xdr_get(int s, char *request, XDR *write_xdrs, FILE *write_stream){
	message msg;

	memset(&msg, 0, sizeof(message));

	msg.tag = XDR_GET;
	msg.message_u.filename = request;
	
	if ( !xdr_message(write_xdrs, &msg) )
		err_quit("!!! xdr_message() failed\n");
	
	//-----------------------------------------------------------//
	//--------------------------XDR------------------------------//
	//------------------FLUSHING-WRITE-STREAM--------------------//
	//-----------------------------------------------------------//
	// forcing write on output xdr stream
	fflush(write_stream);
	//-----------------------------------------------------------//
	//-----------------------------------------------------------//
	//-----------------------------------------------------------//
	//-----------------------------------------------------------//
	
	p_printf("XDR GET sent\n");
}

void p_service_xdr_server_3_4(int cs){
	char request[BUF_LEN];
	char filename[FILENAME_LEN];

	int res;

        fd_set cset;
        struct timeval tval;

	message msg;

	//-----------------------------------------------------------//
	//---------------------XDR-INIT------------------------------//
	//------------------READ-AND-WRITE---------------------------//
	//-----------------------------------------------------------//
	XDR write_xdrs, read_xdrs;
	FILE *write_stream, *read_stream;
	// preparing output xdr stream
	if ( (write_stream = fdopen(cs, "w")) == NULL )
		err_quit("(%s) error - fdopen() failed", prog_name);
	xdrstdio_create(&write_xdrs, write_stream, XDR_ENCODE);
	// preparing input xdr stream
	if ( (read_stream = fdopen(cs, "r")) == NULL )
		err_quit ("(%s) error - fdopen() failed", prog_name);
	xdrstdio_create(&read_xdrs, read_stream, XDR_DECODE);
	//-----------------------------------------------------------//
	//-----------------------------------------------------------//
	//-----------------------------------------------------------//
	//-----------------------------------------------------------//

	memset(request, 0, BUF_LEN);
	memset(filename, 0, FILENAME_LEN);

	/* preparing select */
	FD_ZERO(&cset);
	FD_SET(cs, &cset);
	tval.tv_sec = TIMEOUT;
	tval.tv_usec = 0;

	res = select(FD_SETSIZE, &cset, NULL, NULL, &tval);

	if( res < 0 ){
		// select error
		p_printf("!!! select() failed\n");
	}
	else if ( res == 0 ){
		// timeout
		p_printf("!!! connection timeout\n");
	}
	else{
		memset(&msg, 0, sizeof(message));

		if ( !xdr_message(&read_xdrs, &msg) ){
			// connection lost
			p_printf("!!! xdr_message() failed\n");
		}
		else{
			switch( msg.tag ){
				case XDR_QUIT:{
					p_printf("XDR QUIT received\n");
				}
				break;
				case XDR_GET:{
					strcpy(filename, msg.message_u.filename);
					p_printf("request for file: %s\n", filename);

					p_search_send_xdr_file(cs, filename, &write_xdrs, write_stream);
				}
				break;
				default:{
					p_printf("!!! bad request received\n");
					p_send_xdr_err(cs, &write_xdrs, write_stream);
				}
				break;
			}
		}
	}

	//-----------------------------------------------------------//
	//---------------------XDR-CLEAN-UP--------------------------//
	//--------------------READ-AND-WRITE-------------------------//
	//-----------------------------------------------------------//
	xdr_destroy(&write_xdrs);
	// closing output stream
	fclose(write_stream);
	// freeing intput xdr stream
	xdr_destroy(&read_xdrs);
	// closing input stream
	// NB: Close read streams only after writing operations have also been done
	fclose(read_stream);
	//-----------------------------------------------------------//
	//-----------------------------------------------------------//
	//-----------------------------------------------------------//
	//-----------------------------------------------------------//

}

void p_service_xdr_client_3_4(int s, char *filename){
	uint32_t file_size, file_mtime;
	char *file_content;

	FILE* fd;

	continue_service = 1;

	message msg;

	//-----------------------------------------------------------//
	//---------------------XDR-INIT------------------------------//
	//------------------READ-AND-WRITE---------------------------//
	//-----------------------------------------------------------//
	XDR write_xdrs, read_xdrs;
	FILE *write_stream, *read_stream;
	// preparing output xdr stream
	if ( (write_stream = fdopen(s, "w")) == NULL )
		err_quit("(%s) error - fdopen() failed", prog_name);
	xdrstdio_create(&write_xdrs, write_stream, XDR_ENCODE);
	// preparing input xdr stream
	if ( (read_stream = fdopen(s, "r")) == NULL )
		err_quit ("(%s) error - fdopen() failed", prog_name);
	xdrstdio_create(&read_xdrs, read_stream, XDR_DECODE);
	//-----------------------------------------------------------//
	//-----------------------------------------------------------//
	//-----------------------------------------------------------//
	//-----------------------------------------------------------//

	
	Send_xdr_get(s, filename, &write_xdrs, write_stream);

	memset(&msg, 0, sizeof(message));

	if ( !xdr_message(&read_xdrs, &msg) )
		err_quit("xdr_message() failed\n");
	else{

		switch( msg.tag ){
			case XDR_OK:{
				p_printf("XDR OK FILE received\n");
				file_size = msg.message_u.fdata.contents.contents_len;
				file_content = msg.message_u.fdata.contents.contents_val;
				file_mtime = msg.message_u.fdata.last_mod_time;

				p_printf("file size: %u\n", file_size);
				p_printf("file last modification time: %u\n", file_mtime);

				/* opening file*/
				if( (fd = fopen(filename, "wb")) == NULL)
					err_quit("fopen() failed\n");

				fwrite(file_content, sizeof(char), file_size, fd);

				/* closing file */
				fclose(fd);

				free(msg.message_u.fdata.contents.contents_val);

				p_printf("file %s received\n", filename);
			}
			break;
			case XDR_ERR:{
				p_printf("XDR ERR received\n");
				continue_service = 0;
			}
			break;
			default:{
				p_printf("!!! bad response received\n");
			}
			break;
		}
	}

	//-----------------------------------------------------------//
	//---------------------XDR-CLEAN-UP--------------------------//
	//--------------------READ-AND-WRITE-------------------------//
	//-----------------------------------------------------------//
	xdr_destroy(&write_xdrs);
	// closing output stream
	fclose(write_stream);
	// freeing intput xdr stream
	xdr_destroy(&read_xdrs);
	// closing input stream
	// NB: Close read streams only after writing operations have also been done
	fclose(read_stream);
	//-----------------------------------------------------------//
	//-----------------------------------------------------------//
	//-----------------------------------------------------------//
	//-----------------------------------------------------------//
}

void p_service_type_ahead_xdr_client_3_4(int s){
	char user_input[BUF_LEN], request[BUF_LEN], command[BUF_LEN], filename[BUF_LEN];
	uint32_t file_size, file_mtime;
	char *file_content;

	FILE* fd;

        fd_set cset;

	int receiving_file = 0;
//	int quit_after_recv_file = 0;
	continue_service = 1;

	message msg;

	//-----------------------------------------------------------//
	//---------------------XDR-INIT------------------------------//
	//------------------READ-AND-WRITE---------------------------//
	//-----------------------------------------------------------//
	XDR write_xdrs, read_xdrs;
	FILE *write_stream, *read_stream;
	// preparing output xdr stream
	if ( (write_stream = fdopen(s, "w")) == NULL )
		err_quit("(%s) error - fdopen() failed", prog_name);
	xdrstdio_create(&write_xdrs, write_stream, XDR_ENCODE);
	// preparing input xdr stream
	if ( (read_stream = fdopen(s, "r")) == NULL )
		err_quit ("(%s) error - fdopen() failed", prog_name);
	xdrstdio_create(&read_xdrs, read_stream, XDR_DECODE);
	//-----------------------------------------------------------//
	//-----------------------------------------------------------//
	//-----------------------------------------------------------//
	//-----------------------------------------------------------//
	
	memset(user_input, 0, BUF_LEN);

	while( continue_service ){
		/* preparing select */
		FD_ZERO(&cset);
		FD_SET(s, &cset);
		FD_SET(0, &cset); // 0 as stdin

		Select(FD_SETSIZE, &cset, NULL, NULL, NULL);

		if ( FD_ISSET(s, &cset) ){
			// data from socket
			//p_printf("socket selected\n");

			memset(&msg, 0, sizeof(message));

			if ( !xdr_message(&read_xdrs, &msg) )
				err_quit("xdr_message() failed\n");
			else{

				switch( msg.tag ){
					case XDR_OK:{
						p_printf("XDR OK received\n");
						file_size = msg.message_u.fdata.contents.contents_len;
						file_content = msg.message_u.fdata.contents.contents_val;
						file_mtime = msg.message_u.fdata.last_mod_time;

						p_printf("file size: %u\n", file_size);
						p_printf("file last modification time: %u\n", file_mtime);

						/* opening file*/
//						p_printf("creating file %s\n", filename);
						if( (fd = fopen(filename, "wb")) == NULL)
							err_quit("fopen() failed\n");
//						p_printf("file %s created\n", filename);

//						p_printf("writing on file %s\n", filename);
						fwrite(file_content, file_size, 1, fd);
//						p_printf("writed on file %s\n", filename);

						/* closing file */
//						p_printf("closing file %s\n", filename);
						fclose(fd);
//						p_printf("file %s closed\n", filename);
						p_printf("file %s received\n", filename);
					}
					break;
					case XDR_ERR:{
						p_printf("XDR ERR received\n");
					}
					break;
					default:{
						p_printf("!!! bad response received\n");
					}
					break;
				}
			}
		}
		else{
			// data from stdin
			//p_printf("stdin selected\n");
			p_stdin_readline(user_input, BUF_LEN);

			if ( strcmp(user_input, QUIT_CHAR) == 0 ){
				if ( !receiving_file ){
					continue_service = 0;
					Send_xdr_quit(s, &write_xdrs, write_stream);
				}
				else{
					p_printf("set quit_after_recv_file...\n");
//					quit_after_recv_file = 1;
				}
			}
			else if ( strcmp(user_input, ABORT_CHAR) == 0 ){
				continue_service = 0;
				p_printf("aborted\n");
			}
			else if ( sscanf(user_input, "%s %s", command, filename) == 2 ){
				if ( strcmp(command, GET_CMD) == 0 ){
					P_parse_truncate_string(filename, strlen(filename), request, BUF_LEN - strlen(GET_CMD) - strlen(" ") - strlen(CMD_TAIL));
					Send_xdr_get(s, request, &write_xdrs, write_stream);

				receiving_file = 0;
				}
				else
					p_printf("!!! bad command 1\n");
			}
			else{
				p_printf("!!! bad command 2\n");
			}

			memset(user_input, 0, BUF_LEN);
		}
	}

	//-----------------------------------------------------------//
	//---------------------XDR-CLEAN-UP--------------------------//
	//--------------------READ-AND-WRITE-------------------------//
	//-----------------------------------------------------------//
	xdr_destroy(&write_xdrs);
	// closing output stream
	fclose(write_stream);
	// freeing intput xdr stream
	xdr_destroy(&read_xdrs);
	// closing input stream
	// NB: Close read streams only after writing operations have also been done
	fclose(read_stream);
	//-----------------------------------------------------------//
	//-----------------------------------------------------------//
	//-----------------------------------------------------------//
	//-----------------------------------------------------------//
}

int p_send_xdr_buffered_file(int s, char* filename, uint32_t file_size, uint32_t file_mtime, XDR *write_xdrs, FILE *write_stream){
	message msg;
	char* file_content;
	FILE *fd;
	
	// REMEMBER: alloc and free
	if ( (file_content = malloc(file_size)) == NULL ){
		p_printf("!!! unable to malloc buffer for file content\n");
		p_send_xdr_err(s, write_xdrs, write_stream);
		return -1;
	}

	/* opening file */
	p_printf("opening file %s...\n", filename);
	if( (fd = fopen(filename, "rb")) == NULL){
		p_printf("fopen() failed\n");
		p_send_xdr_err(s, write_xdrs, write_stream);
		return -1;
	}
	p_printf("file %s opened\n", filename);

	memset(file_content, 0, file_size);
	
	// reading file
	p_printf("copying file %s in buffer...\n", filename);
	fread(file_content, file_size, 1, fd);
	p_printf("file copied %s in buffer\n", filename);

	/* closing file */
	p_printf("closing file %s...\n", filename);
	fclose(fd);
	p_printf("file %s closed\n", filename);

	memset(&msg, 0, sizeof(message));

	msg.tag = XDR_OK;
	msg.message_u.fdata.contents.contents_len = file_size;
	msg.message_u.fdata.contents.contents_val = file_content;
	msg.message_u.fdata.last_mod_time = file_mtime;

	if ( !xdr_message(write_xdrs, &msg) ){
		free(file_content);
		p_printf("!!! xdr_message() failed\n");
		return -1;
	}

	//-----------------------------------------------------------//
	//--------------------------XDR------------------------------//
	//------------------FLUSHING-WRITE-STREAM--------------------//
	//-----------------------------------------------------------//
	// forcing write on output xdr stream
	fflush(write_stream);
	//-----------------------------------------------------------//
	//-----------------------------------------------------------//
	//-----------------------------------------------------------//
	//-----------------------------------------------------------//
	
	free(file_content);

	p_printf("XDR OK FILE sent\n");

	return 0;
}

int p_send_xdr_ok_tag(int s, XDR *write_xdrs, FILE *write_stream){
	message msg;
	
	memset(&msg, 0, sizeof(message));

	msg.tag = XDR_OK;

	if ( !xdr_tagtype(write_xdrs, &msg.tag) ){
		p_printf("!!! xdr_message() failed for OK\n");
		return -1;
	}

	//-----------------------------------------------------------//
	//--------------------------XDR------------------------------//
	//------------------FLUSHING-WRITE-STREAM--------------------//
	//-----------------------------------------------------------//
	// forcing write on output xdr stream
	fflush(write_stream);
	//-----------------------------------------------------------//
	//-----------------------------------------------------------//
	//-----------------------------------------------------------//
	//-----------------------------------------------------------//

	p_printf("XDR OK sent\n");

	return 0;
}
int p_send_xdr_file_size(int s, uint32_t file_size, XDR *write_xdrs, FILE *write_stream){

	if ( !xdr_u_int(write_xdrs, &file_size) ){
		p_printf("!!! xdr_message() failed for file size\n");
		return -1;
	}

	//-----------------------------------------------------------//
	//--------------------------XDR------------------------------//
	//------------------FLUSHING-WRITE-STREAM--------------------//
	//-----------------------------------------------------------//
	// forcing write on output xdr stream
	fflush(write_stream);
	//-----------------------------------------------------------//
	//-----------------------------------------------------------//
	//-----------------------------------------------------------//
	//-----------------------------------------------------------//

	p_printf("XDR u_int sent\n");

	return 0;
}

int upper_mod_4_bound(int value){

	if ( value % 4  == 1 )
		return value + 3;

	if ( value % 4  == 2 )
		return value + 2;

	if ( value % 4  == 3 )
		return value + 1;

	return value;
}

int p_send_xdr_file_content(int s, char *filename, uint32_t file_size, XDR *write_xdrs, FILE *write_stream){
	FILE *fd;
	char response[BUF_LEN + 3];	// avoid memset(response, ...) to overwrite other data

	/* opening file */
	p_printf("opening file %s...\n", filename);
	if( (fd = fopen(filename, "rb")) == NULL){
		p_printf("fopen() failed\n");
		p_send_xdr_err(s, write_xdrs, write_stream);
		return -1;
	}
	p_printf("file %s opened\n", filename);

	int bytes_red = 0, bytes_remaining = file_size, buffer_len = upper_mod_4_bound(BUF_LEN);// as XDR base unit is 4 bytes long

	while ( bytes_remaining > 0 ){
		memset(response, 0, buffer_len);

		if ( bytes_remaining > buffer_len)
			bytes_red = fread(response, sizeof(char), buffer_len, fd);
		else
			bytes_red = fread(response, sizeof(char), bytes_remaining, fd);

		bytes_red = upper_mod_4_bound(bytes_red);

		if( bytes_red > 0 ){
			int res = sendn(s, response, bytes_red, MSG_NOSIGNAL);

			if ( res != bytes_red ) {
				p_printf("!!! error while sending file content\n");
				return -1;
			}
			else
				bytes_remaining -= bytes_red;

			p_printf("now: %u\tsent: %u\trem: %u\ttot: %u\n", buffer_len, file_size - bytes_remaining, bytes_remaining, file_size);
		}
		else{
			printf("(%s) Error: connection closed\n",prog_name);
			return -1;
		}
	}

	/* closing file */
	p_printf("closing file %s...\n", filename);
	fclose(fd);
	p_printf("file %s closed\n", filename);

	return 0;
}

int p_search_send_xdr_file(int s, char *filename, XDR *write_xdrs, FILE *write_stream){
	struct stat file_info;
	uint32_t file_size, file_mtime;

	/* searching file */
	if( stat(filename, &file_info) != 0 ){
		p_printf("file not found\n");
		/* file does not exist */
		p_send_xdr_err(s, write_xdrs, write_stream);

		return -1;
	}
	else{
		p_printf("file found\n");
		/* file exists */
		file_size = file_info.st_size;
		file_mtime = file_info.st_mtime;

		p_printf("file size: %u\n", file_size);
		p_printf("file last modification time: %u\n", file_mtime);

		// send all file in one message
		p_send_xdr_buffered_file(s, filename, file_size, file_mtime, write_xdrs, write_stream);

		// frist send OK tag, then file size, then file content, then file mod time
		// p_send_xdr_file_size() is used both for file_size and file_mtime as it sends an uint32_t
		/*
		if( p_send_xdr_ok_tag(s, write_xdrs, write_stream) == 0 ){
			if ( p_send_xdr_file_size(s, file_size, write_xdrs, write_stream) == 0 ){
				if ( p_send_xdr_file_content(s, filename, file_size, write_xdrs, write_stream) == 0 ){
					if(p_send_xdr_file_size(s, file_mtime, write_xdrs, write_stream) == 0)
						p_printf("file transferred\n");
				}
			}
		}
		*/

		return 0;
	}
}

#endif
