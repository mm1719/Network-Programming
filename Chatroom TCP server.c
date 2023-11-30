#include "unp.h"
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <stdbool.h>

// Define a new listening 
#define LISTENq 2

int main() {
	// Create listening socket
	int listenfd;
	struct sockaddr_in servaddr;
	listenfd = Socket(AF_INET, SOCK_STREAM, 0);
	
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(SERV_PORT + 3);
	
	Bind(listenfd, (SA *) &servaddr, sizeof(servaddr));
	
	Listen(listenfd, LISTENq);
	
	// The server will infinitely work
	for(;;) {
		struct sockaddr_in cliaddr[2];
		socklen_t cliLen[2];
		int clientsSocket[2];
		char clientsName[2][MAXLINE];
		char buf[MAXLINE];
		char ip_addr[2][INET_ADDRSTRLEN];	
		
		// Wait till 2 clients connect
		for (int i = 0; i < 2; i++) {
			cliLen[i] = sizeof(cliaddr[i]);
			clientsSocket[i] = Accept(listenfd, (SA *) &cliaddr[i], &cliLen[i]);
			
			Read(clientsSocket[i], clientsName[i], MAXLINE);
			
			if (i == 0) {
				snprintf(buf, sizeof(buf), "You are the 1st user. Wait for the second one!\n");
				Write(clientsSocket[0], buf, strlen(buf));
			} else {
				inet_ntop(AF_INET, &(cliaddr[1].sin_addr), ip_addr[1], INET_ADDRSTRLEN);
				snprintf(buf, sizeof(buf), "The second user is %s from %s\n", clientsName[1], ip_addr[1]);
				Write(clientsSocket[0], buf, strlen(buf));
				
				snprintf(buf, sizeof(buf), "You are the 2nd user\n");
				Write(clientsSocket[1], buf, strlen(buf));
				
				inet_ntop(AF_INET, &(cliaddr[0].sin_addr), ip_addr[0], INET_ADDRSTRLEN);
				snprintf(buf, sizeof(buf), "The first user is %s from %s\n", clientsName[0], ip_addr[0]);
				Write(clientsSocket[1], buf, strlen(buf));
			}
		}
		
		pid_t pid;
		pid = fork();
		if(pid == 0) { //Child process take over the chat room and the disconnection
			Close(listenfd);
			
			fd_set rset;
			int maxfdp1;
			ssize_t n;
			bool clients_disconn[2] = {false, false};
			char recvline[MAXLINE], sendline[MAXLINE];
			FD_ZERO(&rset);
			
			for(;;) {
				if (!clients_disconn[0]) FD_SET(clientsSocket[0], &rset);
				if (!clients_disconn[1]) FD_SET(clientsSocket[1], &rset);
				
				maxfdp1 = max(clientsSocket[0], clientsSocket[1]) + 1;
				
				Select(maxfdp1, &rset, NULL, NULL, NULL);
				
				for (int curr = 0 ; curr < 2; curr++) {
					int other = 1 - curr;
					if(!clients_disconn[curr] && FD_ISSET(clientsSocket[curr], &rset)) {
						if ((n = Read(clientsSocket[curr], recvline, MAXLINE)) == 0) { // If one of them disconnect
							clients_disconn[curr] = true;
							
							
							if (!clients_disconn[other]) { // First client leaves firsst
								snprintf(sendline, sizeof(sendline), "(%s left the room. Press Ctrl+D to leave.)\n", clientsName[curr]);
								Write(clientsSocket[other], sendline, strlen(sendline)); 
								
								shutdown(clientsSocket[other], SHUT_WR); // Using shutdown instead of close
								
							} else {
								snprintf(sendline, sizeof(sendline), "(%s left the room.)\n", clientsName[curr]);
								Write(clientsSocket[other], sendline, strlen(sendline));
								
								shutdown(clientsSocket[other], SHUT_WR); // Using shutdown instead of close
								break;
							}
						} else {
							recvline[n] = '\0'; // Remember to add '\0' to read the line
							snprintf(sendline, sizeof(sendline), "(%s) %s\0", clientsName[curr], recvline); // Using '\0' instead of '\n'
							Write(clientsSocket[other], sendline, strlen(sendline));
						}
						
					}
				}
				

				
				if (clients_disconn[0] && clients_disconn[1]) { // Close the connection in the end
					Close(clientsSocket[0]);
					Close(clientsSocket[1]);
					break;
				};
			}
			
			exit(0);
			
		} else if(pid > 0) { // Also close the connection from the parent process, or the chat room won't really shutdown
			Close(clientsSocket[0]);
			Close(clientsSocket[1]);
			int status;
	    		waitpid(pid, &status, 0); // Kill the child process
		}
		
	}
}
