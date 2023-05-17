#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h> 
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <vector>
#include <math.h>
using namespace std;

struct symbolData {
    float cumProb;
    float probability;
    int length;
};

//converts decimal numbers to binary
string decimalToBinary(float x, int length) {
    string b = "";
    while(b.length() < length) {
        x *= 2;
        string sx = to_string(x);
        if(sx[0] == '0') {
            b.append("0");
        } else {
            b.append("1");
        }
        x = stof(sx.substr(sx.find('.'), sx.size()));
    }
    return b;
}

string shannonFanoElias(struct symbolData *sb) {
    //receive probability from client
    float lob = ceil(log2(1.0/sb->probability) + 1);
    sb->length = lob;
    //covert cum prob to binary and return
    return decimalToBinary(sb->cumProb, lob);
}

void fireman(int) {
   while (waitpid(-1, NULL, WNOHANG) > 0);
}

int main(int argc, char *argv[]) {
    struct symbolData info;

    //set up server, code from Carlos Rincon
    int sockfd, newsockfd, portno, clilen, nsockfd;
    struct sockaddr_in serv_addr, cli_addr;
    int n, m, l, b;
    if (argc < 2) {
        fprintf(stderr,"ERROR, no port provided\n");
        exit(1);
    }
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        cout << "ERROR opening socket" << endl;
    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
        cout << "ERROR on binding" << endl;
    listen(sockfd,5);
    clilen = sizeof(cli_addr);
    
    nsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, (socklen_t *)&clilen);

    signal(SIGCHLD, fireman);
    while(true) {
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, (socklen_t *)&clilen);
         if (newsockfd < 0) 
            cout << "ERROR on accept" << endl;
    
        if(fork() == 0) {
            //receive prob and cumprob from client
            n = read(newsockfd, &info.probability, sizeof(float));
            if(n < 0)
                cout << "ERROR on reading from socket - server" << endl;

            m = read(newsockfd, &info.cumProb, sizeof(float));
            if (m < 0) 
                cout << "ERROR writing to socket - server" << endl;
            
            //calculate binary
            string binary = shannonFanoElias(&info);
            //write length of binary to client
            l = write(newsockfd, &info.length, sizeof(int));
            if (n < 0) 
                cout << "ERROR writing to socket - server" << endl;
            
            //use buffer to write binary string back to client
            char buffer[info.length+1];
            for(int i = 0; i < binary.size(); i++) {
                buffer[i] = binary[i];
            }
            b = write(newsockfd, &buffer, sizeof(info.length)+1);
            if(b < 0)
                cout << "ERROR writing to socket - server" << endl;

            close(sockfd);
            _exit(0);
        }
        close(newsockfd);
    }

    wait(0);
}
