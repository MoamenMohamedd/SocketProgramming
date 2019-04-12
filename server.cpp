#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <vector>
#include <sstream>
#include <string.h>
#include <fstream>
#define REQ_BUF_LEN 2048

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"

using namespace std;

vector<string> split(string str, char delimiter);
void process(int socket , string reqType , string filePath);

int main() {

    //define server port number
    int serverPort = 8080;
    struct sockaddr_in machineAddr;
    int machineAddrLen = sizeof(machineAddr);

    //create listening server socket (welcoming socket)
    //returns socket file discriptor
    int socketFD = socket(AF_INET, SOCK_STREAM, 0);
    if(socketFD == -1){
        perror("Couldn't create socket");
        exit(EXIT_FAILURE);
    }

    machineAddr.sin_family = AF_INET;
    machineAddr.sin_addr.s_addr = INADDR_ANY;
    machineAddr.sin_port = htons(serverPort);

    //bind server port number to server socket
    //ip address of server machine
    bind(socketFD, (struct sockaddr *)&machineAddr, sizeof(machineAddr));

    //listen to incoming requests
    listen(socketFD, 1);

    //always on
    while(true){

        //assign a new connection socket to the client
        int connSocket = accept(socketFD, (struct sockaddr *)&machineAddr, (socklen_t *) &machineAddrLen);

        //read request
        char reqBuffer[REQ_BUF_LEN] = {0};
        recv(connSocket , reqBuffer, REQ_BUF_LEN,NULL);

        //parse request
        vector<string> splitted = split(reqBuffer,' ');
        string reqType = splitted[0];
        string requestedFile = splitted[1];

        process(connSocket , reqType , requestedFile);

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

    while(getline(ss, tok, delimiter)) {
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
void process(int socket , string reqType , string filePath){

    if(reqType == "GET"){
        //open an input stream
        ifstream fis("../serverFiles/" + filePath);

        //continue if file is found
        if (fis) {

            //get file length
            fis.seekg(0, fis.end);
            int length = fis.tellg();
            fis.seekg(0, fis.beg);

            //allocate buffer to store file
            char buffer [length];

            //read file into buffer
            fis.read(buffer, length);

            cout << length << endl;
            cout << buffer << endl;

            //close input stream
            fis.close();

            //send file buffer to client
            send(socket, buffer, length, 0);

        }else{
            //file not found
            cout << "file not found";
        }


    }else{ // POST request

    }
}
