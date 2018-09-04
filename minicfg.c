
#define MY_IP "127.0.0.1"
#define MY_PORT 27016

#define BB_DISPLAY "127.0.0.1"
#define BB_PORT 1984
#define BB_HOSTS "./bb-hosts"

#ifdef WINDOWS
#define _WIN32_WINNT 0x0500

#include <windows.h>
#include <winsock2.h>
#include <stdio.h>

static WSADATA wsaData;
static int ws_started = 0;

int start_winsock(void)
{
	int n;

	if (!ws_started) {
		n = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (n != NO_ERROR) {
			printf("Error at WSAStartup()");
		} else {
			ws_started = 1;
		}
	}
	return ws_started;
}

void stop_winsock(void)
{
	if (ws_started) WSACleanup();
	ws_started = 0;
}
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SOCKET socklen_t
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)
#define SOCKADDR struct sockaddr
#define closesocket close

int start_winsock(void)
{
	return 1;
}

int stop_winsock(void)
{
	return 0;
}

int WSAGetLastError(void)
{
	return errno;
}

#endif	/* WINDOWS */

int main(int argc, char **argv) {
    // Initialize Winsock.
    int n, m, x;
    FILE *fp;
    struct sockaddr_in cli_addr;
    int clilen;
    char *cli;
    char b[1024], ipaddr[1024], machine[1024];
    char *my_ip = MY_IP;
    int my_port = MY_PORT;
    char *bb_display = BB_DISPLAY;
    int bb_port = BB_PORT;
    char *bb_hosts = BB_HOSTS;
    if (argc > 1) my_ip = argv[1];
    if (argc > 2) my_port = atoi(argv[2]);
    if (argc > 3) bb_display = argv[3];
    if (argc > 4) bb_port = atoi(argv[4]);
    if (argc > 5) bb_hosts = argv[5];

    if (!start_winsock()) return 0;

    // Create a socket.
    SOCKET m_socket;
    m_socket = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );

    if ( m_socket == INVALID_SOCKET ) {
        printf( "Error at socket(): %d\n", WSAGetLastError() );
	stop_winsock();
        return 0;
    }

    // Bind the socket.
    struct sockaddr_in service;

    service.sin_family = AF_INET;
    service.sin_addr.s_addr = inet_addr( my_ip );
    service.sin_port = htons( my_port );

    if ( bind( m_socket, (SOCKADDR*) &service, sizeof(service) ) == SOCKET_ERROR ) {
        printf( "bind() failed.\n" );
        closesocket(m_socket);
	stop_winsock();
        return 0;
    }
    
    // Listen on the socket.
    if ( listen( m_socket, 1 ) == SOCKET_ERROR )
        printf( "Error listening on socket.\n");

    // Accept connections.
    int s;

    for (;;) {
        printf( "Waiting for a client to connect...\n" );
        while (1) {
            s = SOCKET_ERROR;
            while ( s == SOCKET_ERROR ) {
		clilen = sizeof cli_addr;
                s = accept( m_socket, (struct sockaddr *)&cli_addr, &clilen );
            }
	    cli = inet_ntoa(cli_addr.sin_addr);
            printf( "Client Connected from %s.\n", cli);
	    fflush(stdout);
            break;
        }

        // Send and receive data.
        char sendbuf[32000] = "Server: Sending Data.";
 
	sprintf(machine, "brokencfg-%s", cli);
	fp = fopen(bb_hosts, "r");
	if (fp) {
		while (fgets(b, sizeof b, fp)) {
			n = sscanf(b, "%s %s", ipaddr, machine);
			if (n != 3) continue;
			if (!strcmp(ipaddr, cli)) break;
		}
	}
	snprintf(sendbuf, sizeof sendbuf,
		"machine %s\n"
		"display %s\n"
		"port %d\n",
		machine, bb_display, bb_port);
        n = strlen(sendbuf);
        m = 0;
        while ((m < n) && (x = send(s, sendbuf+m, n-m, 0)) > 0) {
	    m += x;
        }
        closesocket(s);
    }

    return 0;
}
