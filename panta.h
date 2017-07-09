#include <rpc/rpc.h>

#define FILENAME_LEN 256 + 1
#define BUF_LEN 1024

//#define ENABLE_XDR 	// comment this line to disable xdr functions
			// here and in panta.c

// client1.4
#define TIMEOUT_1_4 3

// server1.4 client1.4
#define NAME_LEN 31 + 1

// client2.1
#define MAX_REQUESTS_2_1 5

// server2.2
#define MAX_REQS_PER_CLIENT_2_2 3
#define MAX_MANAGED_CLIENTS_2_2 10

// server2.3
#define BACKLOG 10
#define SERVER_MAX_CHLD_2_3 10

// server2.3 client2.3
#define GET_CMD "GET"
#define QUIT_CMD "QUIT"
#define OK_CMD "+OK"
#define ERR_CMD "-ERR"
#define CMD_TAIL "\r\n"
#define CMD_LEN 6 + 1

// client3.2
#define QUIT_CHAR "Q"
#define ABORT_CHAR "A"

// server3.3
#define TIMEOUT 2*60
#define SERVER_MAX_CHLD_3_3 10

// server3.4 client3.4
#define	XDR_GET 0
#define	XDR_OK 1
#define	XDR_QUIT 2
#define	XDR_ERR 3
#define SERVER_MAX_CHLD 10

struct my_client{
	struct sockaddr_storage address;
	int requests;
};

int p_upper_bound(int value, int bound);

void sig_handler(int x);

void p_printf(const char *format, ...);

void p_printf_fflush(const char *format, ...);

void p_stdin_readline(char *user_input, int buf_len);

int p_socket_readline(int s, char *buffer, int buf_len);

int p_parse_truncate_string_old(char* src, char* dst, int len);
void P_parse_truncate_string_old(char* src, char* dst, int len);

int p_parse_truncate_string(char* src, int src_len, char* dst, int dst_len);

unsigned long getTimestamp();	// printable with "%lu"



// AF_UNSPEC or AF_INET or AF_INET6 => localhost or ip6-localhost or 127.0.0.1 or 0.0.0.0 or ::1
int P_connectTCP(char *host, char *serv, int protocol_family);

int P_connectUDP(char *host, char *serv, int protocol_family);
// AF_INET or AF_INET6
int P_bindUDP(char *port, int protocol_family);
// AF_INET or AF_INET6
int P_listenTCP(char *port, int protocol_family);



void P_parse_args_client_1_3(int argc, char *argv[], char **host, char **port, uint16_t *x, uint16_t *y);

void P_parse_args_client_07_2014(int argc, char *argv[], char **host, char **port, uint32_t *id, uint32_t *x, uint32_t *y);

void P_parse_args_client_1_4(int argc, char *argv[], char **host, char **port, char* name);

void P_parse_args_server_1_4(int argc, char *argv[], char **port);

void P_parse_args_children_server_2_3(int argc, char *argv[], char **port, int *children);
// if <children> is absent children = 0
void P_parse_args_optional_children_server_2_3(int argc, char *argv[], char **port, int *children);

void P_parse_args_client_2_3(int argc, char *argv[], char **host, char **port, char* name);

void P_parse_args_client_2_4(int argc, char *argv[], char **host, char **port, int *x, int *y);

void P_parse_args_server_3_3(int argc, char *argv[], char **port, int *children);

void P_parse_args_server_3_4(int argc, char *argv[], char **port, int *xdr_mode);

void P_parse_args_children_server_3_4(int argc, char *argv[], char **port, int *children, int *xdr_mode);
// if <children> is absent children = 0
void P_parse_args_optional_children_server_3_4(int argc, char *argv[], char **port, int *children, int *xdr_mode);

void P_parse_args_client_3_4(int argc, char *argv[], int *xdr_mode, char **host, char **port, char *filename);

/*
void P_check_args_chld_server_3_4(int argc, char *argv[], int *xdr_mode, int *port, int *children);
void P_check_args_optional_chld_server_3_4(int argc, char *argv[], int *xdr_mode, int *port, int *children);
*/

void p_service_server_1_3(int cs);

void P_service_client_1_3(int s, uint16_t x, uint16_t y);

void P_service_client_07_2014(int s, uint32_t id, uint32_t x, uint32_t y);

void p_service_server_1_4(int s, int limiting);

int P_service_client_1_4(int s, char* request);

void p_service_server_2_3(int cs);

void p_service_client_2_3(int s, char* filename);

void P_service_client_2_4(int s, int x, int y);

void p_service_server_2_4(int cs);

void p_service_client_3_2(int s);

void p_service_server_3_3(int s);



int p_check_clients(struct my_client *clients, int *clients_len, int *clients_index, int max_clients_len, struct sockaddr_storage new_client, int max_reqs_per_client);



int p_parse_sockaddr_storage_to_address_string(const struct sockaddr *sa, size_t maxlen, char *address_string, int string_len, uint16_t *address_port);



void P_send_quit(int s);

void P_send_get(int s, char* filename, int buf_len);

int p_send_err(int s);

int p_send_ok(int s, uint32_t file_size_n, uint32_t file_mtime_n);



int p_parse_client_request(char *request, char *command, int command_len, char *filename, int filename_len);

int p_parse_server_response(int s, char *command, uint32_t *file_size, uint32_t *file_mtime, int buf_len);



int p_search_send_file(int s, char *filename);

int p_recv_file(int s, char *user_input, uint32_t file_size);

void p_recv_fill_file(int s, char *response, int buf_len, FILE* fd, uint32_t file_size, uint32_t *tot_b_red);

#ifdef ENABLE_XDR

void Send_xdr_get(int s, char *request, XDR *write_xdrs, FILE *write_stream);

void Send_xdr_quit(int s, XDR *write_xdrs, FILE *write_stream);

int p_send_xdr_err(int cs, XDR *write_xdrs, FILE *write_stream);

int p_send_xdr_ok(int s, uint32_t file_size, uint32_t file_mtime, XDR *write_xdrs, FILE *write_stream);



void p_service_xdr_server_3_4(int cs);

void p_service_xdr_client_3_4(int s, char *filename);



int p_search_send_xdr_file(int s, char *filename, XDR *write_xdrs, FILE *write_stream);

void p_service_type_ahead_xdr_client_3_4(int s);

int p_send_xdr_buffered_file(int s, char* filename, uint32_t file_size, uint32_t file_mtime, XDR *write_xdrs, FILE *write_stream);

#endif
