#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <vector>
#include <sstream>
#include <fstream>
#include <arpa/inet.h>
#include <cstring>

#define REQ_BUF_LEN 2048
#define SERV_PORT 8080

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"

using namespace std;

vector<string> split(string str, char delimiter);
void process(int socket, string reqType, string filePath);
void log(char request[] , char clientAddr[] , int clientPort);

int main() {

    //define server address
    struct sockaddr_in machineAddr;
    int machineAddrLen = sizeof(machineAddr);
    machineAddr.sin_family = AF_INET;
    machineAddr.sin_addr.s_addr = INADDR_ANY;
    machineAddr.sin_port = htons(SERV_PORT);

    //create listening server socket (welcoming socket)
    //returns socket file discriptor
    int socketFD = socket(AF_INET, SOCK_STREAM, 0);
    if (socketFD == -1) {
        perror("Couldn't create socket");
        exit(EXIT_FAILURE);
    }

    //bind server port number to server socket
    //add port number to server address
    int retVal = bind(socketFD, (struct sockaddr *) &machineAddr, sizeof(machineAddr));
    if(retVal == -1){
        perror("Couldn't bind port to socket");
        exit(EXIT_FAILURE);
    }

    //listen to incoming requests
    retVal = listen(socketFD, 1);
    if(retVal == -1){
        perror("Couldn't listen on incoming requests");
        exit(EXIT_FAILURE);
    }

    //always on
    while (true) {

        //assign a new connection socket to the client
        int connSocket = accept(socketFD, (struct sockaddr *) &machineAddr, (socklen_t *) &machineAddrLen);
        if (connSocket == -1) {
            perror("Couldn't create socket");
            exit(EXIT_FAILURE);
        }

        //get client address
        struct sockaddr_in clientAddr;
        int clientAddrLen = sizeof(clientAddr);
        retVal = getpeername(connSocket , (struct sockaddr *)&clientAddr , (socklen_t *)&clientAddrLen);
        if(retVal == -1){
            perror("Couldn't get client address");
            exit(EXIT_FAILURE);
        }
        //convert to human readable address
        char clientStrAddr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(clientAddr.sin_addr), clientStrAddr, INET_ADDRSTRLEN);

        //read request
        char req[REQ_BUF_LEN] = {0};
        recv(connSocket, req, REQ_BUF_LEN, NULL);

        //log request
        log(req , clientStrAddr , clientAddr.sin_port);

        //parse request
        vector<string> splitReq = split(req, ' ');
        string reqType = splitReq[0];
        string requestedFile = splitReq[1];

        //do action
        process(connSocket, reqType, requestedFile);

        //close connection socket
        close(connSocket);


    }


}

/**
 * Splits a given string
 *
 * @param str
 * @param delimiter
 * @return
 */
vector<string> split(string str, char delimiter) {
    vector<string> internal;
    stringstream ss(str);
    string tok;

    while (getline(ss, tok, delimiter)) {
        internal.push_back(tok);
    }

    return internal;
}

/**
 *
 *
 * @param socket
 * @param reqType
 * @param filePath
 */
void process(int socket, string reqType, string filePath) {

    if (reqType == "GET") {
        //open an input stream
        ifstream fis("../serverFiles/" + filePath);

        //continue if file is found
        if (fis) {

            //get file length
            fis.seekg(0, fis.end);
            int length = fis.tellg();
            fis.seekg(0, fis.beg);

            //allocate buffer to store file
            char buffer[length];

            //read file into buffer
            fis.read(buffer, length);

            //close input stream
            fis.close();

            //send file buffer to client
            string response = "HTTP/1.0 200 OK\r\n";//status line
            response += "Content-Length: " + to_string(length) + "\r\n";
            response += "\r\n";
            response += buffer;//data

            send(socket, response.c_str() , response.size() , 0);

        } else {
            //file not found
            string response = "HTTP/1.0 404 Not Found\r\n";//status line

            send(socket, response.c_str() , response.size(), 0);
        }


    } else { // POST request


    }
}

void log(char request[] , char clientAddr[] , int clientPort){
    ofstream ofs ("../serverFiles/log.txt", ofstream::app);

    time_t curr_time = time(NULL);
    char *tm = ctime(&curr_time);
    tm[strlen(tm) - 1] = '\0';

    ofs << tm << "  (" << request << ")  " << clientAddr << "  " << to_string(clientPort) << endl;

    ofs.close();

}