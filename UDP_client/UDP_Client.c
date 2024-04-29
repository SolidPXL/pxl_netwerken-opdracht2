//library for windows
//info for compiler :
//gcc udp_client.c -l ws2_32 -o udp_client.exe
//


#ifdef _WIN32
	#define _WIN32_WINNT _WIN32_WINNT_WIN7
	#include <winsock2.h> //for all socket programming
	#include <ws2tcpip.h> //for getaddrinfo, inet_pton, inet_ntop
	#include <stdio.h> //for fprintf, perror
	#include <unistd.h> //for close
	#include <stdlib.h> //for exit
	#include <string.h> //for memset
	#include <time.h>  // for random guess 

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
	
//library for LINUX

#else
	#include <sys/socket.h> //for sockaddr, socket, socket
	#include <sys/types.h> //for size_t
	#include <sys/time.h>
	#include <netdb.h> //for getaddrinfo
	#include <netinet/in.h> //for sockaddr_in
	#include <arpa/inet.h> //for htons, htonl, inet_pton, inet_ntop
	#include <errno.h> //for errno
	#include <stdio.h> //for fprintf, perror
	#include <unistd.h> //for close
	#include <stdlib.h> //for exit
	#include <string.h> //for memset
	void OSInit( void ) {}
	void OSCleanup( void ) {}
#endif


int initialization( struct sockaddr ** internet_address, socklen_t * internet_address_length );
void execution( int internet_socket, struct sockaddr * internet_address, socklen_t internet_address_length );
void cleanup( int internet_socket, struct sockaddr * internet_address );
void wait_for_response(int internet_socket, struct sockaddr *internet_address, socklen_t internet_address_length);
int asking_client_for_guesses();


#define TIMEOUT_SEC 16  //timeout after sending the last message.


int main( int argc, char * argv[] )
{
	OSInit();

	struct sockaddr * internet_address = NULL;
	socklen_t internet_address_length = 0;
	
	int internet_socket = initialization( &internet_address, &internet_address_length );
	

	execution( internet_socket, internet_address, internet_address_length );


	cleanup( internet_socket, internet_address );

	OSCleanup();

	return 0;
}

int initialization( struct sockaddr ** internet_address, socklen_t * internet_address_length )
{
	//Step 1.1
	struct addrinfo internet_address_setup;
	struct addrinfo * internet_address_result;
	memset( &internet_address_setup, 0, sizeof internet_address_setup );
	internet_address_setup.ai_family = AF_UNSPEC;
	internet_address_setup.ai_socktype = SOCK_DGRAM;
	int getaddrinfo_return = getaddrinfo( "127.0.0.1", "24042", &internet_address_setup, &internet_address_result );
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
			*internet_address_length = internet_address_result_iterator->ai_addrlen;
			*internet_address = (struct sockaddr *) malloc( internet_address_result_iterator->ai_addrlen );
			memcpy( *internet_address, internet_address_result_iterator->ai_addr, internet_address_result_iterator->ai_addrlen );
			break;
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

void execution(int internet_socket, struct sockaddr *internet_address, socklen_t internet_address_length)
{
   int total_guesses =  asking_client_for_guesses(); //create random guess between 0-100 do this the amount of times the client input in this function. 
	for(int i = 0; i < total_guesses; i++){
	int rand_nr =  rand()%100;
	char ch_guess[2] ;
	itoa(rand_nr,ch_guess,3);
	
	
    // Step 2.1
    int number_of_bytes_send = 0;
    number_of_bytes_send = sendto(internet_socket, ch_guess , sizeof(ch_guess-1), 0, internet_address, internet_address_length); // send a number to the server.
    if (number_of_bytes_send == -1)
    {
        perror("sendto");
        close(internet_socket);
        exit(EXIT_FAILURE);
    }
	else 
	{
		printf("Guess send to server.\n");
	}
	}
	
wait_for_response(internet_socket, internet_address, internet_address_length);

	wait_for_response(internet_socket, internet_address, internet_address_length);
}


void cleanup( int internet_socket, struct sockaddr * internet_address )
{
	//Step 3.2
	free( internet_address );

	//Step 3.1
	close( internet_socket );
}


void wait_for_response(int internet_socket, struct sockaddr *internet_address, socklen_t internet_address_length){
    // Set up the file descriptor set for select
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(internet_socket, &readfds);
    
    // Set up the timeout for select
    struct timeval tv;
    tv.tv_sec = TIMEOUT_SEC;
    tv.tv_usec = 0;	 
	
	//wait for response or timeout
     int retval = select(internet_socket + 1, &readfds, NULL, NULL, &tv);
    if (retval == -1)
    {
        perror("select()");
        close(internet_socket);
        exit(EXIT_FAILURE);
    }
    else if (retval == 0)
    {
        // Timeout occurred
    printf("I'm lost\n");	
    }
    else
    {
        // Response received
        if (FD_ISSET(internet_socket, &readfds))
        {
            int number_of_bytes_received = 0;
            char buffer[1000];
            number_of_bytes_received = recvfrom(internet_socket, buffer, sizeof(buffer) - 1, 0, internet_address, &internet_address_length);
            if (number_of_bytes_received < 0)
            {
                perror("recvfrom");
                close(internet_socket);
                exit(EXIT_FAILURE);
            }
            else
            {
                buffer[number_of_bytes_received] = '\0';
                printf("Received: %s\n", buffer);
			
            }
        }
	}

}

int asking_client_for_guesses(){
	int guess = 0;
	printf("How many times do you want to guess?: ");
	scanf("%d",&guess);
	
	return guess;
}