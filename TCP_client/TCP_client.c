#ifdef _WIN32
	#define _WIN32_WINNT _WIN32_WINNT_WIN7
	#include <winsock2.h> //for all socket programming
	#include <ws2tcpip.h> //for getaddrinfo, inet_pton, inet_ntop
	#include <stdio.h> //for fprintf, perror
	#include <unistd.h> //for close
	#include <stdlib.h> //for exit
	#include <string.h> //for memset
	#include <stdint.h>
	void OSInit( void )
	{
		WSADATA wsaData;
		int WSAError = WSAStartup( MAKEWORD( 2, 0 ), &wsaData ); 
		if( WSAError != 0 )
		{
			fprintf( stderr, "WSAStartup errno = %d\n", WSAError );
			exit( -1 );
		}
	}
	void OSCleanup( void )
	{
		WSACleanup();
	}
	#define perror(string) fprintf( stderr, string ": WSA errno = %d\n", WSAGetLastError() )
#else
	#include <sys/socket.h> //for sockaddr, socket, socket
	#include <sys/types.h> //for size_t
	#include <netdb.h> //for getaddrinfo
	#include <netinet/in.h> //for sockaddr_in
	#include <arpa/inet.h> //for htons, htonl, inet_pton, inet_ntop
	#include <errno.h> //for errno
	#include <stdio.h> //for fprintf, perror
	#include <unistd.h> //for close
	#include <stdlib.h> //for exit
	#include <string.h> //for memset
	#include <stdint.h>
	void OSInit( void ) {}
	void OSCleanup( void ) {}
#endif

int initialization();
void execution( int internet_socket );
void cleanup( int internet_socket );
int32_t message_to_send();
void check_for_win(char* buffer, int internet_socket);

int main( int argc, char * argv[] )
{
	
	//////////////////
	//Initialization//
	//////////////////

	OSInit();

	int internet_socket = initialization();

	/////////////
	//Execution//
	/////////////

		
	execution( internet_socket );
	
	

	////////////
	//Clean up//
	////////////

	cleanup( internet_socket );

	OSCleanup();

	return 0;
}

int initialization()
{
	//Step 1.1
	struct addrinfo internet_address_setup;
	struct addrinfo * internet_address_result;
	memset( &internet_address_setup, 0, sizeof internet_address_setup );
	internet_address_setup.ai_family = AF_UNSPEC;
	internet_address_setup.ai_socktype = SOCK_STREAM;
	int getaddrinfo_return = getaddrinfo( "127.0.0.1", "24042", &internet_address_setup, &internet_address_result ); //localhost IPv4 : 127.0.0.1  for IPv6 : 0:0:1
	if( getaddrinfo_return != 0 )
	{
		fprintf( stderr, "getaddrinfo: %s\n", gai_strerror( getaddrinfo_return ) );
		exit( 1 );
	}

	int internet_socket = -1;
	struct addrinfo * internet_address_result_iterator = internet_address_result;
	while( internet_address_result_iterator != NULL )
	{
		//Step 1.2
		internet_socket = socket( internet_address_result_iterator->ai_family, internet_address_result_iterator->ai_socktype, internet_address_result_iterator->ai_protocol );
		if( internet_socket == -1 )
		{
			perror( "socket" );
		}
		else
		{
			//Step 1.3
			int connect_return = connect( internet_socket, internet_address_result_iterator->ai_addr, internet_address_result_iterator->ai_addrlen );
			if( connect_return == -1 )
			{
				perror( "connect" );
				close( internet_socket );
			}
			else
			{
				break;
			}
		}
		internet_address_result_iterator = internet_address_result_iterator->ai_next;
	}

	freeaddrinfo( internet_address_result );

	if( internet_socket == -1 )
	{
		fprintf( stderr, "socket: no valid socket address found\n" );
		exit( 2 );
	}

	return internet_socket;
}

void execution( int internet_socket )
{
	//Step 2.1
	int32_t number = htonl(message_to_send());
	
	int number_of_bytes_send = 0;
	number_of_bytes_send = send( internet_socket, &number , sizeof(uint32_t) , 0 );
	if( number_of_bytes_send == -1 )
	{
		perror( "send" );
	}

	//Step 2.2
	int number_of_bytes_received = 0;
	char  buffer[1000] = {'\0'};
	number_of_bytes_received = recv( internet_socket, &buffer, sizeof(buffer) - 1, 0 );
	if( number_of_bytes_received == -1 )
	{
		perror( "recv" );
	}
	else
	{
		buffer[number_of_bytes_received] = '\0';
		printf( "Received : %s\n", buffer );
		//check for win : 
		check_for_win(buffer , internet_socket);
	}
}

void cleanup( int internet_socket )
{
	//Step 3.2
	int shutdown_return = shutdown( internet_socket, SD_SEND );
	if( shutdown_return == -1 )
	{
		perror( "shutdown" );
	}

	//Step 3.1
	close( internet_socket );
}

int32_t message_to_send()
{	
	int32_t number;
	printf("Make your guess between 0 and 1000000: ");
	scanf("%d",&number);
	return number;
}

void check_for_win( char* buffer, int internet_socket)
{
	//printf("do you come here?\n"); //for debugging i printed this message here
	printf("Buffer = %s", buffer);
	if (strcmp(buffer, "correct")==0)
	{
		char play_again;								// when i won ask client if he wants to play again of close the program by calling the cleanup function.
		printf("Do you want to play again?(Y/N)");
		scanf("%s",play_again);
		if (strcmp(play_again,"Y")==0)
		{
			execution( internet_socket );
		}
		else if (strcmp(play_again, "N") == 0)
		{
			cleanup( internet_socket) ;
		}
	}
	else 												//number not yet right so re-run the execution over and over.
	{
		//printf("not yet won\n"); //for debugging i printed this message
		execution( internet_socket );
	}
}