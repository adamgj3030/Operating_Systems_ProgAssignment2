#include <unistd.h>
#include <iostream>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <cstring>
#include <pthread.h>
#include <vector>

using namespace std;

struct CharCode {
    char character;
    int freq;
    std::string code;
};

// Struct for passing data to threads
struct ThreadData 
{
    int portno;
    string hostname; 
    string line;
    string encodedLine;
    std::vector<CharCode> charCodeVec;
};

struct ServerMessage 
{
    char line[32];
};

// Function that each thread will run to communicate with the server
void *communicateWithServer(void *void_ptr) 
{
    ThreadData* curr_ptr = (ThreadData *) void_ptr;
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent* server;

    const char* hostnameChar = curr_ptr->hostname.c_str();
    portno = curr_ptr->portno;  
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) 
    {
        std::cerr << "ERROR opening socket" << std::endl;
        exit(0);
        return nullptr;
    }

    server = gethostbyname(hostnameChar); 
    if (server == NULL) {
        std::cerr << "ERROR, no such host" << std::endl;
        exit(0);
        return nullptr;
    }

    bzero((char*)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);

    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "ERROR connecting" << std::endl;
        exit(0);
        return nullptr;
    }

    ServerMessage message;
    strcpy(message.line, curr_ptr->line.c_str());

    n = write(sockfd, &message, sizeof(message));
    if (n < 0)
    {
        std::cerr << "ERROR writing to socket" << std::endl;
        exit(0);
        return nullptr;
    }

    // Receiving the alphabet size
    int alphabetSize;
    n = read(sockfd, &alphabetSize, sizeof(int));
    if (n < 0) {
        std::cerr << "ERROR reading alphabet size from socket" << std::endl;
        exit(0);
        return nullptr;
    }

    // For storing the symbol, frequency, and Shannon code
    vector<CharCode> charCodeVec;

    for (int i = 0; i < alphabetSize; i++) {
        CharCode charCode;
    
        // Receive the character
        n = read(sockfd, &charCode.character, sizeof(char));
        if (n < 0) {
            std::cerr << "ERROR reading character from socket" << std::endl;
            exit(0);
        }

        // Receive the frequency
        n = read(sockfd, &charCode.freq, sizeof(int));
        if (n < 0) {
            std::cerr << "ERROR reading frequency from socket" << std::endl;
            exit(0);
        }

        // Receive the Shannon code length
        int codeLength;
        n = read(sockfd, &codeLength, sizeof(int));
        if (n < 0) {
            std::cerr << "ERROR reading code length from socket" << std::endl;
            exit(0);
        }

        // Receive the Shannon code itself
        char* codeBuffer = new char[codeLength + 1];
        bzero(codeBuffer, codeLength + 1);
        n = read(sockfd, codeBuffer, codeLength);
        if (n < 0) {
            std::cerr << "ERROR reading Shannon code from socket" << std::endl;
            exit(0);
        }
        charCode.code = std::string(codeBuffer);
        delete[] codeBuffer;

        // Store the CharCode in the vector
        charCodeVec.push_back(charCode);
    }

    // Store the alphabet in the ThreadData for displaying later
    curr_ptr->charCodeVec = charCodeVec;


    // Receiving encoded message size
    int encodedSize;
    n = read(sockfd, &encodedSize, sizeof(int));
    if (n < 0) {
        std::cerr << "ERROR reading from socket" << std::endl;
        exit(0);
        return nullptr;
    }

    char *encodedBuffer = new char[encodedSize + 1];
    bzero(encodedBuffer, encodedSize + 1);

    n = read(sockfd, encodedBuffer, encodedSize);
    if (n < 0) {
        std::cerr << "ERROR reading from socket" << std::endl;
        exit(0);
        return nullptr;
    }

    curr_ptr->encodedLine = encodedBuffer;
    delete[] encodedBuffer;
    close(sockfd);
    return nullptr;
}

int main(int argc, char *argv[]) 
{
    if (argc != 3) 
    {
       std::cerr << "usage " << argv[0] << " hostname port" << std::endl;
       exit(0);
    }

    ThreadData input;
    input.hostname = argv[1];
    input.portno = atoi(argv[2]);

    std::vector<ThreadData> threadData; 

    // take in the input 
    while (std::getline(std::cin, input.line)) {
        threadData.push_back(input);
    }

    int threadSize = threadData.size(); 

    // store the thread id in tid
    pthread_t *tid = new pthread_t[threadSize];

    for (int i = 0; i < threadSize; i++)
    {
        // create a thread for each ThreadData struct of the vector, and if an error is thrown return 1;
        if (pthread_create(&tid[i], nullptr, communicateWithServer, &threadData[i]))
        {
            std::cerr << "Error creating thread" << std::endl;
            return 1;
        }
    }

    // join threads back together when they are finished
    for (int i = 0; i < threadSize; i++)
    {
        pthread_join(tid[i], nullptr);
    }

    for (int i = 0; i < threadSize; i++) 
    {
        ThreadData currData = threadData[i];

        std::cout << "Message: " << currData.line << std::endl;
        std::cout << std::endl;

        std::cout << "Alphabet:" << std::endl;

        for (const auto& charCode : currData.charCodeVec) 
        {
            std::cout << "Symbol: "<< charCode.character
            << ", Frequency: " << charCode.freq 
            << ", Shannon code: " << charCode.code << std::endl;
        }

        std::cout << std::endl;

        std::cout << "Encoded message: " << currData.encodedLine << std::endl;

        std::cout << std::endl;
    }

    return 0;
}
