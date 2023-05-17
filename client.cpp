#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <iostream>
#include <vector>
#include <math.h>
using namespace std;

struct symbolData {
    float probability;
    float cumProb;
    char symbol;
    int pos;
    int length;
    string binary;

    symbolData(char s) : symbol(s) {}
};

struct sockaddr_in serv_addr;
struct hostent *server;

bool compareProbabilites(const symbolData &a, const symbolData &b) {
    return a.probability > b.probability;
}

//calculates cumutalive probability
float cumProb(int pos, float prob, vector<symbolData> aProb, float sum) {
    if(pos == 0) 
        return ((0.5)*prob)+sum;
    else {
        sum = sum + (aProb[pos-1].probability);
        return cumProb(pos-1, prob, aProb, sum);
    }
}

void* sendReceiveInfo(void *symData) {
    struct symbolData *sym_data_ptr = (struct symbolData *) symData;
    int n, nsockfd, m, l, b;
    //setup socket
    nsockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(nsockfd < 0)
        cout << "ERROR opening socket" << endl;
    if (connect(nsockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) 
        cout << "ERROR connecting client" << endl;
    
    //send info to server
    float prob = sym_data_ptr->probability;
    float cumprob = sym_data_ptr->cumProb;

    n = write(nsockfd, &prob, sizeof(float));
    if (n < 0) 
        cout << "ERROR writing to socket - client" << endl;
    m = write(nsockfd, &cumprob, sizeof(float));
    if (m < 0) 
        cout << "ERROR writing to socket - client" << endl;

    //receive binary size
    l = read(nsockfd, &sym_data_ptr->length, sizeof(int));
    if (l < 0) 
        cout << "ERROR writing to socket - client" << endl;

    //receive binary string
    char buffer[sym_data_ptr->length+1];
    b = read(nsockfd, &buffer, sym_data_ptr->length+1);
    for(int i = 0; i < sym_data_ptr->length; i++) {
        sym_data_ptr->binary += buffer[i];
    }
    if (b < 0) 
        cout << "ERROR writing to socket - server" << endl;

    close(nsockfd);
    return nullptr;
}

void print(vector<symbolData> information) {
    cout << "SHANNON-FANO-ELIAS Codes:" << endl << endl;

    for(int i=0; i<information.size(); i++) {
        cout << "Symbol " << information[i].symbol << ", Code: " << information[i].binary << endl;
    }
}

int main(int argc, char *argv[]) {
    vector<symbolData> info;
    string dataString;

    //accept input
    getline(cin, dataString);

    //identify all symbols
    string uniqueSym = dataString;
    sort(begin(uniqueSym), end(uniqueSym));
    auto last = unique(begin(uniqueSym), end(uniqueSym));
    uniqueSym.erase(last, end(uniqueSym));
    for(int i = 0; i < uniqueSym.size(); i++) {
        info.push_back(symbolData(uniqueSym[i]));
    }
    
    //calculate probability of symbols
    for(int i = 0; i < info.size(); i++) {
        int freq = 0;
        for(int j = 0; j < dataString.size(); j++) {
            if(info[i].symbol == dataString[j]){
                freq++;
            }
        }
        float prob = freq/ float(dataString.size());
        info[i].probability = prob;
    }
    //sort info based on probability largest to smallest
    sort(info.begin(), info.end(), compareProbabilites); 
    for(int i = 0; i < info.size(); i++) {
        info[i].pos = i;
    }
    //calculate cumutalive probability
    for(int i = 0; i < info.size(); i++) {
        info[i].cumProb = cumProb(info[i].pos, info[i].probability, info, 0);
    }
    //set up client, code from Carlos Rincon
    int sockfd, portno, n, m;

    if (argc < 3) {
       fprintf(stderr,"usage %s hostname port\n", argv[0]);
       exit(0);
    }
    portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        cout << "ERROR opening socket" << endl;
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) 
        cout << "ERROR connecting" << endl;
    
    //create m threads where m is the size of the alphabet
    pthread_t tid[info.size()];
    for(int m = 0; m < info.size(); m++) {
        if(pthread_create(&tid[m], NULL, sendReceiveInfo, &info[m])){
            fprintf(stderr, "Error creating thread\n");
            return 1;
        }
    }

    //join threads
    for(int m = 0; m < info.size(); m++) {
        pthread_join(tid[m], nullptr);
    }
     
    print(info);
}