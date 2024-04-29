
#ifdef _WIN32
	#define _WIN32_WINNT _WIN32_WINNT_WIN7
	#include <winsock2.h> //for all socket programming
	#include <ws2tcpip.h> //for getaddrinfo, inet_pton, inet_ntop
	#include <stdio.h> //for fprintf, perror
	#include <stdlib.h> //for exit
	#include <string.h> //for memset
	#include <math.h>
	#include <time.h>
	
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
	int OSInit( void ) {}
	int OSCleanup( void ) {}
#endif

int initialization();
void execution( int internet_socket );
void cleanup( int internet_socket );
// int addrCompare(struct sockaddr_storage left,struct sockaddr_storage right);

int main( int argc, char * argv[] )
{
	//////////////////
	//Initialization//
	//////////////////

	printf("Program start\n");
	OSInit();
	printf("OSInit end\n");
	int internet_socket = initialization();
	printf("Socket init end\n");

	/////////////
	//Execution//
	/////////////

	execution( internet_socket );
	printf("Execution end\n");

	////////////
	//Clean up//
	////////////

	cleanup( internet_socket );
	printf("Cleanup end\n");

	OSCleanup();
	printf("OS cleanup end\n");

	return 0;
}

int initialization()
{
	//Step 1.1
	struct addrinfo internet_address_setup;
	struct addrinfo * internet_address_result;
	memset( &internet_address_setup, 0, sizeof internet_address_setup );
	internet_address_setup.ai_family = AF_INET;
	internet_address_setup.ai_socktype = SOCK_DGRAM;
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
				closesocket( internet_socket );
				perror( "bind" );
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

	//Seed rng
	srand(time(NULL));

	return internet_socket;
}

struct Guesser {
	struct sockaddr_storage adress;
	int guess;
	int dif;
};

void compareGuess(){

}

void execution( int internet_socket )
{
	//rng positive int less than 1000
	const int number_to_guess = rand()%100;
	//Step 2.1
	int number_of_bytes_received = 0;
	char buffer[50];
	struct sockaddr_storage client_internet_address;
	socklen_t client_internet_address_length = sizeof client_internet_address;
	struct Guesser highest_guess;

	int timeout = 8000;
	printf("Waiting for first valid guess\n");
	while(1){
		number_of_bytes_received = recvfrom( internet_socket, buffer, ( sizeof buffer ) - 1, 0, (struct sockaddr *) &client_internet_address, &client_internet_address_length );
		if(number_of_bytes_received != -1){
			buffer[number_of_bytes_received] = '\0';
			int guessed_number = atoi(buffer);
			if(guessed_number==0) continue;
			int guessed_number_dif = abs(number_to_guess-guessed_number);

			highest_guess.adress = client_internet_address;
			highest_guess.guess = guessed_number;
			highest_guess.dif = guessed_number_dif;

			memset(buffer,0,sizeof(buffer));
			break;
		}
	}

	while (1)
	{
		printf("Inside of loop\n");
		setsockopt(internet_socket,SOL_SOCKET,SO_RCVTIMEO,(const char*)&timeout,sizeof(timeout));
		number_of_bytes_received = recvfrom( internet_socket, buffer, ( sizeof buffer ) - 1, 0, (struct sockaddr *) &client_internet_address, &client_internet_address_length );
		if(number_of_bytes_received != -1){
			//Compare if this is the newest closest guess
			buffer[number_of_bytes_received] = '\0';
			int guessed_number = atoi(buffer);
			if(guessed_number==0) continue;
			int guessed_number_dif = abs(number_to_guess-guessed_number);

			if(guessed_number_dif<highest_guess.dif){
				highest_guess.adress = client_internet_address;
				highest_guess.guess = guessed_number;
				highest_guess.dif = guessed_number_dif;
			}
			memset(buffer,0,sizeof(buffer));
			timeout = timeout/2;


		} else {
			//Request probably timed out, break
			break;
		}
	}
	char addrBuf[50] = {'\0'};
	char portBuf[20] = {'\0'};
	getnameinfo((void*)(&highest_guess.adress),sizeof(highest_guess.adress),addrBuf,sizeof(addrBuf),portBuf,sizeof(portBuf),NI_NUMERICHOST);
	printf("Highest guess:\nip:\t%s\nguess:\t%d\ndif:\t%d\nport:\t%s\n",addrBuf,highest_guess.guess,highest_guess.dif,portBuf);

	//'Lines closed' send message to possible winner and initiate no message timer
	const char message[] = "You won ?";
	long unsigned int lineTimeout = 16000;
	const long int endTime = (long int)time(NULL)+16;
	sendto(internet_socket,message,sizeof(message)-1,0,(struct sockaddr*)&highest_guess.adress,sizeof(highest_guess.adress));
	while(1){
		lineTimeout = (endTime - (long int)time(NULL))*1000;
		printf("linetimeout: %ld\n",lineTimeout);
		setsockopt(internet_socket,SOL_SOCKET,SO_RCVTIMEO,(const char*)&lineTimeout,sizeof(lineTimeout));
		number_of_bytes_received = recvfrom( internet_socket, buffer, ( sizeof buffer ) - 1, 0, (struct sockaddr *) &client_internet_address, &client_internet_address_length );

		if(lineTimeout<1){
			//Safe case, break out of loop if wait time is too short
			break;
		}
		if(number_of_bytes_received == -1){
			if(WSAGetLastError()==WSAETIMEDOUT){
				//Bytes received is -1 because it timed out

				//Add check to match if the incomming message is from the highest guesser
				const char winMessage[] = "You won !";
				sendto(internet_socket,winMessage,sizeof(winMessage)-1,0,(struct sockaddr*)&highest_guess.adress,sizeof(highest_guess.adress));
				break;
			}
		} else{
			//Check if sender is the one with highest guess, if so send you lost and restart game, else send you lost to sender and continue the  16second timer
			if(memcmp(&highest_guess.adress,&client_internet_address,sizeof(struct sockaddr_storage))==0){
				printf("Memcompared is 0\n");
				const char failMessage[] = "You Lost !";
				(internet_socket,failMessage,sizeof(failMessage)-1,0,(struct sockaddr*)&highest_guess.adress,sizeof(highest_guess.adress));
				break;
			} else{
				printf("Memcompared is different\n");
				const char failMessage[] = "You Lost !";
				sendto(internet_socket,failMessage,sizeof(failMessage)-1,0,(struct sockaddr*)&client_internet_address,sizeof(client_internet_address));
			}
			
		}
	}

}

void cleanup( int internet_socket )
{
	//Step 3.1
	closesocket( internet_socket );
}