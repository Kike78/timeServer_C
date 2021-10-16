/* time_server.c */
/* go to the browser and write this: http://127.0.0.1:8080 */

/* to begin with, we include the needed headers */
#if defined(_WIN32)
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif

#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>

#endif

/* we also define some macros, which abstract out some of the difference between the berkeley socket and
 * winsock apis: */
#if defined(_WIN32)
#define ISVALIDSOCKET(s) ((s) != INVALID_SOCKET)
#define CLOSESOCKET(s) closesocket(s)
#define GETSOCKETERRNO() (WSAGetLastError())

#else 
#define ISVALIDSOCKET(s) ((s) >= 0)
#define CLOSESOCKET(s) close(s)
#define SOCKET int
#define GETSOCKETERRNO() (errno)
#endif

/* we need a couple of standard C headers */
#include <stdio.h>
#include <string.h>
#include <time.h>

/* The first thing the main() function will do is initialize winsock if we are compiling on windows: */
int main()
{
#if defined(_WIN32)
   WSADATA d;
   if (WASAStartup(MAKEWORD(2, 2), &d))
   {
      fprintf(stderr, "Failed to initialize.\n");
      return 1;
   }
#endif

   /* we must now figure out the local address that our web server should bind to: */
   printf("Configuring local address...\n");
   struct addrinfo hints;

   memset(&hints, 0, sizeof(hints));
   hints.ai_family = AF_INET;
   hints.ai_socktype = SOCK_STREAM;
   hints.ai_flags = AI_PASSIVE;

   struct addrinfo *bind_address;
   getaddrinfo(0, "8080", &hints, &bind_address);

   /* Now that we've figure out our local address info, we can create the socket: */
   printf("Creating socket...\n");
   SOCKET socket_listen;
   socket_listen = socket(bind_address -> ai_family, bind_address -> ai_socktype, bind_address -> ai_protocol);

   /* We should check taht the call to socket() was successful */
   if (!ISVALIDSOCKET(socket_listen))
   {
      fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
      return 1;
   }

   /* After the socket has been created successfully, we can call bind() to associate it with our
    * address from getaddrinfo(): */
   printf("Binding socket to local address...\n");
   if (bind(socket_listen, bind_address -> ai_addr, bind_address -> ai_addrlen))
   {
      fprintf(stderr, "bind() failed. (%d)\n", GETSOCKETERRNO());
      return 1;
   }
   freeaddrinfo(bind_address);   //we can call the freeaddrinfo() function to release the address memory.

   /* Once the socket has been created and bound to a local address, we can cause it to start listening
    * for connections with the listen() function: */
   printf("Listening...\n");
   if (listen(socket_listen, 10) < 0)
   {
      fprintf(stderr, "listen() failed. (%d)\n", GETSOCKETERRNO());
      return 1;   // error handling for listen() is done the same way as we did for bind() and socket().
   }

   /* After the socket has begun listening for connections, we can accept any incoming connection with the
    * accept() function: */
   printf("Waiting for connection...\n");
   struct sockaddr_storage client_address;
   socklen_t client_len = sizeof(client_address);

   SOCKET socket_client = accept(socket_listen, (struct sockaddr*) &client_address, &client_len);
   if (!ISVALIDSOCKET(socket_client))
   {
      fprintf(stderr, "accept() failed. (%d)\n", GETSOCKETERRNO());
      return 1;
   }

   /* At this point, a TCP connection has been established to a remote client. We can print the client's
    * address to the console: */
   printf("Client is connected...");
   char address_buffer[100];

   getnameinfo((struct sockaddr*) &client_address, client_len, address_buffer, sizeof(address_buffer), 0, 0, NI_NUMERICHOST);
   printf("%s\n", address_buffer); /* this step is optional, but it is good practice to log network connections somewhere */

   /* As we are programming a web server, we expect the client(for example, a web browser) to send us an HTTP 
    * request. We read this request using the recv() function: */
   printf("Reading request...\n");
   char request[1024];
   int bytes_received = recv(socket_client, request, 1024, 0);
   printf("Received %d bytes.\n", bytes_received);

   /* Now that the web browser has sent its request, we can send our response back: */
   printf("Sendig response...\n");
   const char *response = "HTTP/1.1 200 OK\r\n"
                          "Connection: close\r\n"
                          "Content-Type: text/plain\r\n\r\n"
                          "Local time is: ";
   int bytes_sent = send(socket_client, response, strlen(response), 0);
   printf("Sent %d of %d bytes.\n", bytes_sent, (int)strlen(response));

   /* Now we can send the actual time: */
   time_t timer;
   time(&timer);
   char *time_msg = ctime(&timer);
   bytes_sent = send(socket_client, time_msg, strlen(time_msg), 0);
   printf("Sent %d of %d bytes.\n", bytes_sent, (int)strlen(time_msg));

   /* We must the close the client connection to indicate to the browser that we've sent all our data: */
   printf("Closing connection...\n");
   CLOSESOCKET(socket_client);

   /* We will close the listening socket too and terminate the program: */
   printf("Closing listening socket...\n");
   CLOSESOCKET(socket_listen);

#if defined(_WIN32)
   WSACleanup();
#endif

   printf("Finished.\n");

   return 0;
}
