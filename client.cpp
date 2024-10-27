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

// same struct from PP1 to hold the character and its associated frequency and shannon code
struct CharCode {
    char character;
    int freq;
    std::string code;
};

// Struct for passing portno, hostname, and input line to threads, and passing back the encoded line and character info
struct ThreadData 
{
    int portno;
    std::string hostname; 
    std::string line;
    std::string encodedLine;
    std::vector<CharCode> charCodeVec;
};

// Function that each thread will run to communicate with the server
void *communicateWithServer(void *void_ptr) 
{
    ThreadData* curr_ptr = (ThreadData *) void_ptr;
    // Create a socket (Taken from Canvas Socket Practice)
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent* server;

    // assign hostname and portno from main arguments (Edited from Canvas Socket Practice)
    const char* hostnameChar = curr_ptr->hostname.c_str();
    portno = curr_ptr->portno;  
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    // Error checking for socket creation (Taken from Canvas Socket Practice)
    if (sockfd < 0) 
    {
        std::cerr << "ERROR opening socket" << std::endl;
        exit(0);
        return nullptr;
    }

    // Error checking for gethostbyname (Taken from Canvas Socket Practice)
    server = gethostbyname(hostnameChar); 
    if (server == NULL) {
        std::cerr << "ERROR, no such host" << std::endl;
        exit(0);
        return nullptr;
    }

    // Preparing serv_addr to connect with IPv4 (Taken from Canvas Socket Practice)
    bzero((char*)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);

    // Error checking for connection (Taken from Canvas Socket Practice)
    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "ERROR connecting" << std::endl;
        exit(0);
        return nullptr;
    }

    // Sending the line to encode to the server
    char message[32];
    strcpy(message, curr_ptr->line.c_str());

    // Error checking for writing to the socket (Taken from Canvas Socket Practice)
    n = write(sockfd, &message, sizeof(message));
    if (n < 0)
    {
        std::cerr << "ERROR writing to socket" << std::endl;
        exit(0);
        return nullptr;
    }

    // Receiving the charCodeVec size
    int charCodeVecSize;
    n = read(sockfd, &charCodeVecSize, sizeof(int));
    if (n < 0) {
        std::cerr << "ERROR reading from socket" << std::endl;
        exit(0);
        return nullptr;
    }

    // For storing the symbol, frequency, and Shannon code
    std::vector<CharCode> charCodeVec;

    // Receiving the CharCodeVec itself (symbol, frequency, Shannon code)
    for (int i = 0; i < charCodeVecSize; i++) {
        CharCode charCode;
    
        // Receive the character (always going to be a single char)
        n = read(sockfd, &charCode.character, sizeof(char));
        if (n < 0) {
            std::cerr << "ERROR reading from socket" << std::endl;
            exit(0);
        }

        // Receive the frequency (always going to be an int)
        n = read(sockfd, &charCode.freq, sizeof(int));
        if (n < 0) {
            std::cerr << "ERROR reading from socket" << std::endl;
            exit(0);
        }

        // Receive the Shannon code length (variable size char array)
        int codeLength;
        n = read(sockfd, &codeLength, sizeof(int));
        if (n < 0) {
            std::cerr << "ERROR reading from socket" << std::endl;
            exit(0);
        }

        // +1 for null terminator
        char* codeBuffer = new char[codeLength + 1];
        bzero(codeBuffer, codeLength + 1);

        // Receive the Shannon code itself
        n = read(sockfd, codeBuffer, codeLength);
        if (n < 0) {
            std::cerr << "ERROR reading from socket" << std::endl;
            exit(0);
        }
        // Copy the code to the CharCode struct
        charCode.code = std::string(codeBuffer);

        // Delete the buffer
        delete[] codeBuffer;

        // Store the CharCode in the vector
        charCodeVec.push_back(charCode);
    }

    // Store the charCode info in the ThreadData struct for displaying later
    curr_ptr->charCodeVec = charCodeVec;


    // Receiving encoded line size
    int encodedSize;
    n = read(sockfd, &encodedSize, sizeof(int));
    if (n < 0) {
        std::cerr << "ERROR reading from socket" << std::endl;
        exit(0);
        return nullptr;
    }

    // +1 for null terminator
    char *encodedBuffer = new char[encodedSize + 1];
    bzero(encodedBuffer, encodedSize + 1);

    // Receiving the encoded line itself
    n = read(sockfd, encodedBuffer, encodedSize);
    if (n < 0) {
        std::cerr << "ERROR reading from socket" << std::endl;
        exit(0);
        return nullptr;
    }

    // Store the encoded line in the ThreadData struct for displaying later
    curr_ptr->encodedLine = encodedBuffer;
    // Delete the buffer
    delete[] encodedBuffer;
    // Close the socket
    close(sockfd);
    // Exit the thread
    return nullptr;
}

int main(int argc, char *argv[]) 
{   
    // Error checking for the correct number of arguments (Taken from Canvas Socket Practice)
    if (argc != 3) 
    {
       std::cerr << "usage " << argv[0] << " hostname port" << std::endl;
       exit(0);
    }

    ThreadData input;
    //argv[1] is hostname, argv[2] is portno;
    input.hostname = argv[1];
    input.portno = atoi(argv[2]);

    std::vector<ThreadData> threadData; 

    // Read in the input lines from the user 
    while (std::getline(std::cin, input.line)) {
        threadData.push_back(input);
    }

    int threadSize = threadData.size(); 

    // Store the thread ID in tid
    pthread_t *tid = new pthread_t[threadSize];

    for (int i = 0; i < threadSize; i++)
    {
        // Create a thread for each ThreadData struct of the vector, and if an error is thrown return 1;
        if (pthread_create(&tid[i], nullptr, communicateWithServer, &threadData[i]))
        {
            std::cerr << "Error creating thread" << std::endl;
            return 1;
        }
    }

    // Join threads back together when they are finished
    for (int i = 0; i < threadSize; i++)
    {
        pthread_join(tid[i], nullptr);
    }

    // Display the character info and encoded line for each input line
    for (int i = 0; i < threadSize; i++) 
    {
        ThreadData currData = threadData[i];

        std::cout << "Message: " << currData.line << std::endl;
        std::cout << std::endl;

        std::cout << "Alphabet:" << std::endl;

        // Display the character info for each character
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
