#include <unistd.h>
#include <iostream>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
// added this library manually
#include <cstring> 

struct symbol
{
    char name;
    int frequency;
    double probability;
    char code[10];
};

int main(int argc, char *argv[])
{
    int sockfd, portno, n;
    std::string buffer;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    if (argc != 3) 
    {
       std::cerr << "usage " << argv[0] << " hostname port" << std::endl;
       exit(0);
    }
    portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
    {
        std::cerr << "ERROR opening socket" << std::endl;
        exit(0);
    }
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        std::cerr << "ERROR, no such host" << std::endl;
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) 
    {
        std::cerr << "ERROR connecting" << std::endl;
        exit(0);
    }

    // added function to send a struct
    symbol s;
    s.name = 'a';
    s.frequency = 1;
    s.probability = 1;
    strcpy(s.code, "adam\0");

    n = write(sockfd,&s,sizeof(s));
    if (n < 0) 
    {
        std::cerr << "ERROR writing to socket" << std::endl;
        exit(0);
    }

    close(sockfd);
    return 0;
}