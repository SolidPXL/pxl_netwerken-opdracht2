#define _WIN32_WINNT                  0x0A00 // Windows 10
#define INCL_WINSOCK_API_PROTOTYPES 1
#define WIN32_LEAN_AND_MEAN
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <stdio.h> //for fprintf, perror
#include <stdlib.h> //for exit
#include <string.h> //for memset
#include <time.h>
#include <stdint.h>

#pragma comment(lib,"ws2_32")

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

<<<<<<< HEAD

#if(_WIN32_WINNT >= 0x0600)
typedef struct pollfd {
        SOCKET  fd;
        SHORT   events;
        SHORT   revents;
} WSAPOLLFD, *PWSAPOLLFD, FAR *LPWSAPOLLFD;
WINSOCK_API_LINKAGE int WSAAPI WSAPoll(LPWSAPOLLFD fdArray, ULONG fds, INT timeout);
#endif // (_WIN32_WINNT >= 0x0600)
=======
// #if(_WIN32_WINNT >= 0x0600)
// typedef struct pollfd {
//         SOCKET  fd;
//         SHORT   events;
//         SHORT   revents;
// } WSAPOLLFD, *PWSAPOLLFD, FAR *LPWSAPOLLFD;
// WINSOCK_API_LINKAGE int WSAAPI WSAPoll(LPWSAPOLLFD fdArray, ULONG fds, INT timeout);
// #endif // (_WIN32_WINNT >= 0x0600)
>>>>>>> ed918de3e82554f4ee2e6fa40deffa0bfe13f6cd


int initialization();
void connexecution(int internet_socket);
void closefdSocket(struct pollfd** fdsArr, uint32_t** toguessArr,int* size, int toRemove);
void cleanup( int internet_socket);
void log_guess(int internet_socket , const char *ip_adresss, uint32_t guessed_number);

int main( int argc, char * argv[] )
{
	//////////////////
	//Initialization//
	//////////////////

	OSInit();

	int internet_socket = initialization();

	//////////////
	//Connection + Execution//
	//////////////

	connexecution(internet_socket);

	// printf("fdsArray now has listening socket on %d\n", fdsArr[0].fd);

	/////////////
	//Execution//
	/////////////

	// execution( client_internet_socket );


	////////////
	//Clean up//
	////////////

	cleanup( internet_socket);

	OSCleanup();

	return 0;
}

int initialization()
{
	//Step 1.1
	srand(time(NULL));
	struct addrinfo internet_address_setup;
	struct addrinfo * internet_address_result;
	memset( &internet_address_setup, 0, sizeof internet_address_setup );
	internet_address_setup.ai_family = AF_INET;
	internet_address_setup.ai_socktype = SOCK_STREAM;
	internet_address_setup.ai_flags = AI_PASSIVE;
	int getaddrinfo_return = getaddrinfo( NULL, "24042", &internet_address_setup, &internet_address_result );
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
			int bind_return = bind( internet_socket, internet_address_result_iterator->ai_addr, internet_address_result_iterator->ai_addrlen );
			if( bind_return == -1 )
			{
				perror( "bind" );
				closesocket( internet_socket );
			}
			else
			{
				//Step 1.4
				int listen_return = listen( internet_socket, 1 );
				if( listen_return == -1 )
				{
					closesocket( internet_socket );
					perror( "listen" );
				}
				else
				{
					break;
				}
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
	int timeout = 1;
	setsockopt(internet_socket,SOL_SOCKET,SO_RCVTIMEO,(const char*)&timeout,sizeof(timeout));

	return internet_socket;
}

void connexecution(int internet_socket){
	struct pollfd* fdsArr = (struct pollfd*)malloc(sizeof(struct pollfd));
	uint32_t* toguessArr = malloc(sizeof(uint32_t));
	int arraySize = 1;
	fdsArr[0].fd = internet_socket;
	fdsArr[0].events = 0x0100|0x200;
	toguessArr[0] = 0;

	int pollRes; 
	while(1){
		pollRes = WSAPoll(fdsArr,arraySize,-1);
		if (pollRes == -1)
		{
			perror("Polling");
			break;
		}
		if(pollRes>0){
			if(fdsArr[0].revents & 0x0100){
				//New connection needs to be made
				//Step 2.1
				struct sockaddr_storage client_internet_address;
				socklen_t client_internet_address_length = sizeof client_internet_address;
				int client_socket = accept( internet_socket, (struct sockaddr *) &client_internet_address, &client_internet_address_length );
				if( client_socket == -1 )
				{
					perror( "accept" );
					closesocket( internet_socket );
					exit( 3 );
				} else {
					arraySize++;
					fdsArr = realloc(fdsArr, arraySize*sizeof(struct pollfd));
					toguessArr = realloc(toguessArr, arraySize*sizeof(uint32_t));
					fdsArr[arraySize-1].fd = client_socket;
					fdsArr[arraySize-1].events = 0x0100;
					fdsArr[arraySize-1].revents = 0;
					toguessArr[arraySize-1] = (rand()<<6)%1000000;
				}
			}
			for(int i=1;i<arraySize;i++){
				if(fdsArr[i].revents & 0x0002){
					//Socket was closed
					closefdSocket(&fdsArr,&toguessArr,&arraySize,i);
				}
				if(fdsArr[i].revents & 0x0100){
					uint32_t guessed_number;
					int number_of_bytes_received = recv( fdsArr[i].fd, (char*)&guessed_number, ( sizeof guessed_number ), 0 );
					if( number_of_bytes_received == -1 )
					{
						perror( "recv" );
						break;
					}
					else
					{
						//Go from network byte order to 'normal' byte order
						guessed_number = htonl(guessed_number);
					}

					//Step 3.2

					//Get client IP out of socket.
					struct sockaddr_storage client_addr;
					socklen_t addr_len = sizeof(client_addr);
					if (getpeername(fdsArr[i].fd, (struct sockaddr *)&client_addr, &addr_len) == -1) {
						perror("getpeername");
						closesocket(fdsArr[i].fd);
						return;
					}
					char client_ip[INET_ADDRSTRLEN]; //macro only for IPv4
					inet_ntop(AF_INET, &((struct sockaddr_in *)&client_addr)->sin_addr, client_ip, INET_ADDRSTRLEN); //converts to human readable language
					// char client_ip[INET_ADDRSTRLEN] = inet_ntoa(*((struct in_addr*)&client_addr));


					int number_of_bytes_send = 0;
					if(guessed_number < toguessArr[i]){
						number_of_bytes_send = send( fdsArr[i].fd, "Higher!", 7, 0 );
						log_guess(internet_socket , client_ip, guessed_number);
						if( number_of_bytes_send == -1 )
						{
							perror( "send" );
						}
					} else if (guessed_number > toguessArr[i]){
						number_of_bytes_send = send( fdsArr[i].fd, "Lower!", 6, 0 );
						log_guess(internet_socket , client_ip, guessed_number);
						if( number_of_bytes_send == -1 )
						{
							perror( "send" );
						}
					} else {
						number_of_bytes_send = send( fdsArr[i].fd, "Correct!", 8, 0 );
						log_guess(internet_socket , client_ip, guessed_number);
						if( number_of_bytes_send == -1 )
						{
							perror( "send" );
						}
					}
				}
			}
		}
		
		

	}
}

void closefdSocket(struct pollfd** fdsArr, uint32_t** toguessArr,int* size,int toRemove){
	//close socket
	closesocket((*fdsArr)[toRemove].fd);
	//Shift each item in array to their left
	for(int i=toRemove;i<*size-1;i++){
		(*fdsArr)[i] = (*fdsArr)[i+1];
		(*toguessArr)[i] = (*toguessArr)[i+1];
		
	}
	*size=*size-1;
	//Reallocate memory to proper size
	*fdsArr = realloc(*fdsArr,*size*sizeof(struct pollfd));
	*toguessArr = realloc(*toguessArr,*size*sizeof(uint32_t));
}

void cleanup( int internet_socket)
{
	//Step 4.1
	closesocket( internet_socket );
}

void log_guess(int internet_socket , const char *ip_adresss, uint32_t guessed_number)
{
	time_t current_time;
	char time_stamp[20];
	struct tm *time_info;
	
	time(&current_time);
	time_info = localtime(&current_time);
	strftime(time_stamp,sizeof(time_stamp), "%Y-%m-%d %H:%M:%S", time_info);
	
	FILE *file = fopen("file.log", "a");
	if (file != NULL)
	{
		fprintf(file, "[%s] socket : %d\tIP: %s\tguessed : %u\n",time_stamp,internet_socket,ip_adresss,guessed_number);
		fclose(file);
	}
}