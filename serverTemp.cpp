#include <unistd.h>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <signal.h>
#include <vector>
#include <string>
#include <map>
#include <algorithm>
#include <cmath>
#include <sys/wait.h>


struct ServerMessage 
{
    char line[32];
    char encodedLine[32];
};

struct CharCode {
    char character;
    int freq;
    std::string code;
};

struct EncodedMsg {
    std::string line;
    std::vector<CharCode> charCodeVec;
    std::string encodedLine;
};

bool compareFreqChar(CharCode a, CharCode b) {
    // first compares frequencies
    if (a.freq != b.freq)
        return a.freq > b.freq;
    
    // if frequencies are even, then compares characters
    return a.character > b.character;
}

std::string decimalToBinary(float decimal, int precision) {
    std::string binary = "";
    while (decimal > 0 && precision > 0) {
        double temp = decimal * 2;
        if (temp >= 1) {
            binary += "1";
            decimal = temp - 1;
        } else {
            binary += "0";
            decimal = temp;
        }
        --precision;
    }
    while (precision > 0) {
        binary += "0";
        --precision;
    }
    return binary;
}

void shannonCode(EncodedMsg& msg) {
    int lineSize = msg.line.length();
    std::map<char, int> charCountMap;
    for (char c : msg.line) {
        ++charCountMap[c];
    }

    CharCode temp;

    for (const auto& charCount : charCountMap) {
        temp.character = charCount.first;
        temp.freq = charCount.second;
        msg.charCodeVec.push_back(temp);
    }

    std::sort(msg.charCodeVec.begin(), msg.charCodeVec.end(), compareFreqChar);

    float cumulativeProbability = 0;
    std::map<char, std::string> charCodeMap;
    for (auto& charCode : msg.charCodeVec) {
        // probability = frequency / total freq (total freq is just the length of the line)
        float probability = ((float)charCode.freq/lineSize);

        // precision = ceiling of log base 2 (1/probability)
        int precision = ceil(log2(1/probability));

        // maps the character to its associated code
        std::string ShannonCode = decimalToBinary(cumulativeProbability, precision);
        charCode.code = ShannonCode;
        charCodeMap[charCode.character] = ShannonCode;

        // adds current probability to the total
        cumulativeProbability += probability;
    }

    // iterates over the intial line
    for (int i = 0; i < lineSize; ++i) {
        // Adds each encountered characters associated shannon code to the finished encoded line 
        msg.encodedLine += charCodeMap[msg.line[i]];
    }
}

void fireman(int) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

int main(int argc, char* argv[]) {
    int sockfd, newsockfd, portno, clilen;
    struct sockaddr_in serv_addr, cli_addr;
    int n;
    signal(SIGCHLD, fireman);
    if (argc < 2) {
        std::cerr << "ERROR, no port provided" << std::endl;
        exit(1);
    }
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        std::cerr << "Error opening socket" << std::endl;
        exit(1);
    }

    bzero((char*)&serv_addr, sizeof(serv_addr));
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
        newsockfd = accept(sockfd, (struct sockaddr*)&cli_addr, (socklen_t*)&clilen);
        if (fork() == 0) 
        {
            if (newsockfd < 0) 
            {
                std::cerr << "Error on accept" << std::endl;
                return 1;
            }

            ServerMessage message;
            n = read(newsockfd, &message, sizeof(message));
            if (n < 0)
            {
                std::cerr << "Error reading from socket" << std::endl;
                exit(1);
            }
            EncodedMsg msg;
            msg.line = message.line;
            shannonCode(msg);

            int alphabetSize = msg.charCodeVec.size();
            n = write(newsockfd, &alphabetSize, sizeof(int));
            if (n < 0) {
                std::cerr << "Error writing alphabet size to socket" << std::endl;
                exit(1);
            }
            
            for (auto& charCode : msg.charCodeVec) {

                n = write(newsockfd, &charCode.character, sizeof(char));
                if (n < 0) {
                    std::cerr << "Error writing character to socket" << std::endl;
                    exit(1);
                }

                n = write(newsockfd, &charCode.freq, sizeof(int));
                if (n < 0) {
                    std::cerr << "Error writing frequency to socket" << std::endl;
                    exit(1);
                }

                int codeLength = charCode.code.size();
                n = write(newsockfd, &codeLength, sizeof(int));
                if (n < 0) {
                    std::cerr << "Error writing code length to socket" << std::endl;
                    exit(1);
                }

                n = write(newsockfd, charCode.code.c_str(), codeLength);
                if (n < 0) {
                    std::cerr << "Error writing Shannon code to socket" << std::endl;
                    exit(1);
                }
            }

            int encodedSize = msg.encodedLine.size();
            char encodedLine[encodedSize + 1]; // +1 for null terminator
            strcpy(encodedLine, msg.encodedLine.c_str());

            n = write(newsockfd, &encodedSize, sizeof(int));
            if (n < 0)
            {
                std::cerr << "Error reading from socket" << std::endl;
                exit(1);
            }
            
            n = write(newsockfd, encodedLine, encodedSize);
            if (n < 0)
            {
                std::cerr << "Error reading from socket" << std::endl;
                exit(1);
            }

            close(newsockfd);
            _exit(0);
        }
        close(newsockfd);
    }

    close(sockfd);
    return 0;
}
