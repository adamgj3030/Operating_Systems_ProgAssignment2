#include <iostream>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <cstring>
#include <unistd.h>

struct CharCode {
    char character;
    int freq;
    std::string code;
};

int main(int argc, char *argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: ./client <hostname> <port_no> <input_file>" << std::endl;
        return 1;
    }

    int portno = atoi(argv[2]);
    std::string hostname = argv[1];

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        std::cerr << "Error opening socket" << std::endl;
        return 1;
    }

    struct hostent *server = gethostbyname(hostname.c_str());
    if (!server) {
        std::cerr << "Error, no such host" << std::endl;
        return 1;
    }

    struct sockaddr_in serv_addr;
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Error connecting to server" << std::endl;
        return 1;
    }

    // Read input message from file
    std::string inputMessage;
    std::ifstream inputFile(argv[3]);
    if (!inputFile.is_open()) {
        std::cerr << "Error opening file" << std::endl;
        return 1;
    }
    std::getline(inputFile, inputMessage);
    inputFile.close();

    // Send input message size and message
    int msgSize = inputMessage.size();
    write(sockfd, &msgSize, sizeof(int));
    write(sockfd, inputMessage.c_str(), msgSize);

    // Receive alphabet size
    int alphabetSize;
    read(sockfd, &alphabetSize, sizeof(int));

    // Receive alphabet data
    std::vector<CharCode> alphabet(alphabetSize);
    for (int i = 0; i < alphabetSize; i++) {
        read(sockfd, &alphabet[i].character, sizeof(char));
        read(sockfd, &alphabet[i].freq, sizeof(int));
        int codeSize;
        read(sockfd, &codeSize, sizeof(int));
        char *code = new char[codeSize + 1];
        read(sockfd, code, codeSize);
        code[codeSize] = '\0';
        alphabet[i].code = code;
        delete[] code;
    }

    // Receive encoded message size and message
    int encodedSize;
    read(sockfd, &encodedSize, sizeof(int));
    char *encodedMessage = new char[encodedSize + 1];
    read(sockfd, encodedMessage, encodedSize);
    encodedMessage[encodedSize] = '\0';

    // Print the result
    std::cout << "Message: " << inputMessage << std::endl;
    std::cout << "Alphabet:" << std::endl;
    for (const auto &cc : alphabet) {
        std::cout << "Symbol: " << cc.character << ", Frequency: " << cc.freq << ", Shannon code: " << cc.code << std::endl;
    }
    std::cout << "Encoded message: " << encodedMessage << std::endl;

    delete[] encodedMessage;
    close(sockfd);
    return 0;
}
