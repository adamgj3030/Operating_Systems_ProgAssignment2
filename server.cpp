#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
// added this library manually
#include <cstring>
#include <sys/wait.h>
#include <signal.h> 

struct symbol {
  char name;
  int frequency;
  double probability;
  char code[10];
};


void fireman(int)
{
  while (waitpid(-1, NULL, WNOHANG) > 0);
}

int main(int argc, char *argv[])
{
  int sockfd, newsockfd, portno, clilen;
  struct sockaddr_in serv_addr, cli_addr;
  int n;
  signal(SIGCHLD, fireman);
  if (argc < 2)
   {
      std::cerr << "ERROR, no port provided" << std::endl;
      exit(1);
   }
   sockfd = socket(AF_INET, SOCK_STREAM, 0);
   if (sockfd < 0)
   {
      std::cerr << "Error opening socket" << std::endl;
      exit(1);
   }
   bzero((char *)&serv_addr, sizeof(serv_addr));
   portno = atoi(argv[1]);
   serv_addr.sin_family = AF_INET;
   serv_addr.sin_addr.s_addr = INADDR_ANY;
   serv_addr.sin_port = htons(portno);
   if (bind(sockfd, (struct sockaddr *)&serv_addr,
       sizeof(serv_addr)) < 0)
   {
      std::cerr << "Error on binding" << std::endl;
      exit(1);
   }
   listen(sockfd, 5);
   clilen = sizeof(cli_addr);
   while (true)
   {
     newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, (socklen_t *)&clilen);
     if (fork() == 0)
     {
       if (newsockfd < 0)
       {
         std::cerr << "Error on accept" << std::endl;
         exit(1);
       }
       int size;
       n = read(newsockfd, &size, sizeof(int));
       if (n < 0)
       {
         std::cerr << "Error reading from socket" << std::endl;
         exit(1);
       }
       char *buffer = new char[size + 1];
       bzero(buffer, size + 1);
       n = read(newsockfd, buffer, size);
       if (n < 0)
       {
         std::cerr << "Error reading from socket" << std::endl;
         exit(1);
       }
       symbol s;
       n = read(newsockfd, &s, sizeof(symbol));
       if (n < 0)
       {
         std::cerr << "Error reading from socket" << std::endl;
         exit(1);
       }
       std::cout << "Name: " << s.name << std::endl;
       std::cout << "Frequency: " << s.frequency << std::endl;
       std::cout << "Probability: " << s.probability << std::endl;
       std::cout << "Code: " << s.code << std::endl;
       
       char message[] = "You got a 100!";
       int sMessage = strlen(message);
       n = write(newsockfd, &sMessage, sizeof(int));
       if (n < 0)
       {
         std::cerr << "Error reading from socket" << std::endl;
         exit(1);
       }
       n = write(newsockfd, message, sMessage);
       if (n < 0)
       {
         std::cerr << "Error reading from socket" << std::endl;
         exit(1);
       }
       delete[] buffer;
       close(newsockfd);
       _exit(0);
     }
  }
  close(sockfd);
  return 0;
}