// libraries needed from server.cpp
#include <unistd.h>
#include <iostream>
#include <stdlib.h>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
// libraries needed from fireman.cpp
#include <sys/wait.h>
// libraries needed from shannon code project
#include <vector>
#include <map>
#include <algorithm>
#include <cmath>

//#include <signal.h>

struct CharCode {
    char character;
    int freq;
    std::string code;
};

// Function to handle zombie processes
void fireman(int) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

// Function to convert probability to binary
std::string decimalToBinary(float decimal, int precision) {
    std::string binary = "";
    while (decimal > 0 && precision > 0) {
        decimal *= 2;
        if (decimal >= 1) {
            binary += "1";
            decimal -= 1;
        } else {
            binary += "0";
        }
        --precision;
    }
    return binary;
}

// Shannon encoding function
std::string shannonEncode(const std::string &message, std::vector<CharCode> &alphabet) {
    std::map<char, int> frequencyMap;
    for (char c : message) {
        frequencyMap[c]++;
    }

    for (auto &entry : frequencyMap) {
        CharCode cc;
        cc.character = entry.first;
        cc.freq = entry.second;
        alphabet.push_back(cc);
    }

    std::sort(alphabet.begin(), alphabet.end(), [](CharCode a, CharCode b) {
        return a.freq > b.freq;
    });

    float cumulativeProbability = 0.0;
    for (auto &cc : alphabet) {
        float probability = static_cast<float>(cc.freq) / message.size();
        int precision = ceil(log2(1.0 / probability));
        cc.code = decimalToBinary(cumulativeProbability, precision);
        cumulativeProbability += probability;
    }

    std::string encodedMessage = "";
    for (char c : message) {
        for (const auto &cc : alphabet) {
            if (cc.character == c) {
                encodedMessage += cc.code;
                break;
            }
        }
    }
    return encodedMessage;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: ./server <port_no>" << std::endl;
        return 1;
    }

    int portno = atoi(argv[1]);
    int sockfd, newsockfd;
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t clilen;

    signal(SIGCHLD, fireman); // Prevent zombie processes

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        std::cerr << "Error opening socket" << std::endl;
        return 1;
    }

    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Error binding socket" << std::endl;
        return 1;
    }

    listen(sockfd, 5);
    clilen = sizeof(cli_addr);

    while (true) {
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (newsockfd < 0) {
            std::cerr << "Error accepting connection" << std::endl;
            continue;
        }

        if (fork() == 0) {
            close(sockfd);

            int msgSize;
            read(newsockfd, &msgSize, sizeof(int));
            char *message = new char[msgSize + 1];
            read(newsockfd, message, msgSize);
            message[msgSize] = '\0';

            std::string inputMessage = message;
            std::vector<CharCode> alphabet;
            std::string encodedMessage = shannonEncode(inputMessage, alphabet);

            // Send alphabet
            int alphabetSize = alphabet.size();
            write(newsockfd, &alphabetSize, sizeof(int));
            for (const auto &cc : alphabet) {
                write(newsockfd, &cc.character, sizeof(char));
                write(newsockfd, &cc.freq, sizeof(int));
                int codeSize = cc.code.size();
                write(newsockfd, &codeSize, sizeof(int));
                write(newsockfd, cc.code.c_str(), codeSize);
            }

            // Send encoded message
            int encodedSize = encodedMessage.size();
            write(newsockfd, &encodedSize, sizeof(int));
            write(newsockfd, encodedMessage.c_str(), encodedSize);

            delete[] message;
            close(newsockfd);
            exit(0);
        }
        close(newsockfd);
    }

    close(sockfd);
    return 0;
}
