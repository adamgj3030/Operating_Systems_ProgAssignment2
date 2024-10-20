#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <string>
#include <vector>
#include <map>
#include <cmath>
#include <algorithm>
#include <signal.h>
#include <sys/wait.h>

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