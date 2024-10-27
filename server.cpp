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

// struct to hold the character and its associated frequency and shannon code
struct CharCode {
    char character;
    int freq;
    std::string code;
};

// Struct for passing portno, hostname, and input line to Shannon Code function (server doesnt use multithreading but instead forking),
// and passing back the encoded line and character info
struct EncodedMsg {
    std::string line;
    std::vector<CharCode> charCodeVec;
    std::string encodedLine;
};

// Function to help sort the CharCode vector by frequency and character (From PP1)
bool compareFreqChar(CharCode a, CharCode b) {
    // First compares frequencies
    if (a.freq != b.freq)
        return a.freq > b.freq;
    
    // If frequencies are even, then compares characters
    return a.character > b.character;
}

// Function to convert a decimal to binary (From PP1)
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

// Function to create the Shannon code for the given line (From PP1)
void shannonCode(EncodedMsg& msg) {
    int lineSize = msg.line.length();

    // Map to hold the character and its associated frequency
    std::map<char, int> charCountMap;
    for (char c : msg.line) {
        ++charCountMap[c];
    }

    CharCode temp;

    // Iterates over the map and stores the character and its associated frequency in the CharCode vector
    for (const auto& charCount : charCountMap) {
        temp.character = charCount.first;
        temp.freq = charCount.second;
        msg.charCodeVec.push_back(temp);
    }

    // Sorts the CharCode vector by frequency and character
    std::sort(msg.charCodeVec.begin(), msg.charCodeVec.end(), compareFreqChar);

    // Calculates the probability and precision for each character
    float cumulativeProbability = 0;
    std::map<char, std::string> charCodeMap;
    for (auto& charCode : msg.charCodeVec) {
        // Probability = frequency / total freq (total freq is just the length of the line)
        float probability = ((float)charCode.freq/lineSize);

        // Precision = ceiling of log base 2 (1/probability)
        int precision = ceil(log2(1/probability));

        // Maps the character to its associated code
        std::string ShannonCode = decimalToBinary(cumulativeProbability, precision);
        charCode.code = ShannonCode;
        charCodeMap[charCode.character] = ShannonCode;

        // Adds current probability to the total
        cumulativeProbability += probability;
    }

    // Iterates over the intial line
    for (int i = 0; i < lineSize; ++i) {
        // Adds each encountered characters associated shannon code to the finished encoded line 
        msg.encodedLine += charCodeMap[msg.line[i]];
    }
}

// Function to clean up zombie processes (Taken from Canvas Socket Practice)
void fireman(int) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

int main(int argc, char* argv[]) {

    // Error checking for the correct number of arguments (Taken from Canvas Socket Practice)
    if (argc < 2) {
        std::cerr << "ERROR, no port provided" << std::endl;
        exit(0);
    }

    // Create a socket (Taken from Canvas Socket Practice)
    int sockfd, newsockfd, portno, clilen;
    struct sockaddr_in serv_addr, cli_addr;
    int n;

    // Error checking for socket creation (Taken from Canvas Socket Practice)
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        std::cerr << "Error opening socket" << std::endl;
        exit(0);
    }

    // Preparing serv_addr to connect with IPv4 (Taken from Canvas Socket Practice)
    bzero((char*)&serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    // Error checking for binding (Taken from Canvas Socket Practice)
    if (bind(sockfd, (struct sockaddr *)&serv_addr,
        sizeof(serv_addr)) < 0)
    {
        std::cerr << "Error on binding" << std::endl;
        exit(0);
    }

    listen(sockfd, 5);
    clilen = sizeof(cli_addr);
    // Signal handler to clean up zombie processes (Taken from Canvas Socket Practice)
    signal(SIGCHLD, fireman);
    while (true) 
    {
        newsockfd = accept(sockfd, (struct sockaddr*)&cli_addr, (socklen_t*)&clilen);
        // Forking to handle the new connection (Taken from Canvas Socket Practice)
        if (fork() == 0) 
        {   
            // Error checking for accepting connection (Taken from Canvas Socket Practice)
            if (newsockfd < 0) 
            {
                std::cerr << "Error on accept" << std::endl;
                return 1;
            }

            // Receive the line to encode from the client
            char message[32];
            // Error checking for reading from the socket (Taken from Canvas Socket Practice)
            n = read(newsockfd, &message, sizeof(message));
            if (n < 0)
            {
                std::cerr << "Error reading from socket" << std::endl;
                exit(1);
            }

            // Create an EncodedMsg struct to hold the line, character info, and encoded line
            EncodedMsg msg;
            msg.line = message;
            
            // Create the Shannon code for the line
            shannonCode(msg);

            // Sending the charCodeVec size
            int charCodeVecSize = msg.charCodeVec.size();
            n = write(newsockfd, &charCodeVecSize, sizeof(int));
            if (n < 0) {
                std::cerr << "Error writing alphabet size to socket" << std::endl;
                exit(1);
            }
            
            // Sending the CharCodeVec itself (symbol, frequency, Shannon code)
            for (auto& charCode : msg.charCodeVec) {

                // Send the character (always going to be a single char)
                n = write(newsockfd, &charCode.character, sizeof(char));
                if (n < 0) {
                    std::cerr << "Error writing character to socket" << std::endl;
                    exit(1);
                }

                // Send the frequency (always going to be an int)
                n = write(newsockfd, &charCode.freq, sizeof(int));
                if (n < 0) {
                    std::cerr << "Error writing frequency to socket" << std::endl;
                    exit(1);
                }

                // Send the Shannon code length (variable size char array)
                int codeLength = charCode.code.size();
                n = write(newsockfd, &codeLength, sizeof(int));
                if (n < 0) {
                    std::cerr << "Error writing code length to socket" << std::endl;
                    exit(1);
                }

                // Send the Shannon code itself
                n = write(newsockfd, charCode.code.c_str(), codeLength);
                if (n < 0) {
                    std::cerr << "Error writing Shannon code to socket" << std::endl;
                    exit(1);
                }
            }

            // +1 for null terminator
            int encodedSize = msg.encodedLine.size();
            char encodedLine[encodedSize + 1];
            strcpy(encodedLine, msg.encodedLine.c_str());

            // Sending the encoded line size
            n = write(newsockfd, &encodedSize, sizeof(int));
            if (n < 0)
            {
                std::cerr << "Error reading from socket" << std::endl;
                exit(1);
            }
            
            // Sending the encoded line itself
            n = write(newsockfd, encodedLine, encodedSize);
            if (n < 0)
            {
                std::cerr << "Error reading from socket" << std::endl;
                exit(1);
            }
            
            // Close the child socket
            close(newsockfd);
            // Exit the child process
            _exit(0);
        }
    }

    // Close the parent socket
    close(sockfd);
    return 0;
}
