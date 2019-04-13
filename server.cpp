#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <vector>
#include <sstream>
#include <fstream>
#include <arpa/inet.h>
#include <cstring>

#define SERV_PORT 8080
#define BUFF_SIZE 3000

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"

using namespace std;

vector<string> split(string str, char delimiter);
string receiveMsg(int socket);
void storeFile(string filePath , string response);
void process(int socket, string reqType, string filePath , string reqMsg);
void log(string request , char clientAddr[] , int clientPort);

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
        string request = receiveMsg(connSocket);

        //parse request
        string reqLine = request.substr(0,request.find_first_of("\r\n")-1);
        vector<string> splitReqLine = split(reqLine , ' ');
        string reqType = splitReqLine[0];
        string filePath = splitReqLine[1];

        cout << reqLine << endl;

        //log request
        log(reqLine , clientStrAddr , clientAddr.sin_port);

        //do action
        process(connSocket, reqType, filePath , request);

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
void process(int socket, string reqType, string filePath , string reqMsg) {

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
            response += buffer;//data
            response += "\r\n";

            send(socket, response.c_str() , response.size() , 0);

        } else {
            //file not found
            string response = "HTTP/1.0 404 Not Found\r\n";//status line

            send(socket, response.c_str() , response.size(), 0);
        }


    } else { // POST request
        storeFile(filePath , reqMsg);

        //send response to client
        string response = "HTTP/1.0 200 OK\r\n";//status line

        send(socket, response.c_str() , response.size() , 0);

    }
}

void log(string request , char clientAddr[] , int clientPort){
    ofstream ofs ("../serverFiles/log.txt", ofstream::app);

    time_t curr_time = time(NULL);
    char *tm = ctime(&curr_time);
    tm[strlen(tm) - 1] = '\0';

    ofs << tm << "  (" << request << ")  " << clientAddr << "  " << to_string(clientPort) << endl;

    ofs.close();
}


string receiveMsg(int socket){

    char buffer[BUFF_SIZE];
    string request = "";
    int length = 0;

    while (true){
        length = recv(socket, &buffer , sizeof(buffer), 0);
        request.append(buffer,length);
        if(request[request.size()-1] == '\n'){
            break;
        }

    }

    if (length == -1){
        perror("recv error");
    }

    return request;
}



void storeFile(string filePath , string request){
    ofstream ofs ("../serverFiles/" + filePath, ofstream::out);


    int dataBegin = request.find_first_of('\n');
    int dataEnd = request.find_last_of('\n');

    int length = dataEnd - dataBegin;

    string data = request.substr(dataBegin + 1,length);

    ofs << data;

    ofs.close();

}