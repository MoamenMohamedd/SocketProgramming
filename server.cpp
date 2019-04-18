#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <vector>
#include <sstream>
#include <fstream>
#include <arpa/inet.h>
#include <cstring>
#include <thread>
#include <regex>

#define SERV_PORT 8080
#define BUFF_SIZE 3000

#define HTTP_VERSION "HTTP/1.0"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"

using namespace std;

vector<string> split(string str, char delimiter);
tuple<string, string, string> receiveReq(int socket);
void storeFile(string filePath , string data);
void process(int socket , string reqLine , string headers , string data);
void log(string request , char clientAddr[] , int clientPort);
void serve(int connSocket);

int main() {

    //define server address
    struct sockaddr_in machineAddr;
    int machineAddrLen = sizeof(machineAddr);
    machineAddr.sin_family = AF_INET;
    machineAddr.sin_addr.s_addr = INADDR_ANY;
    machineAddr.sin_port = htons(SERV_PORT);

    //create listening server socket (welcoming socket)
    //returns socket file descriptor
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

        //assign a worker thread to the client
        thread workerThread(serve, connSocket);
        workerThread.detach();//work independently
    }


}

/**
 * thread function to handle client request
 *
 * @param connSocket
 */
void serve(int connSocket){

    //get client address
    struct sockaddr_in clientAddr;
    int clientAddrLen = sizeof(clientAddr);
    int retVal = getpeername(connSocket , (struct sockaddr *)&clientAddr , (socklen_t *)&clientAddrLen);
    if(retVal == -1){
        perror("Couldn't get client address");
        exit(EXIT_FAILURE);
    }

    //convert to human readable address
    char clientStrAddr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(clientAddr.sin_addr), clientStrAddr, INET_ADDRSTRLEN);

    //read request
    string reqLine , headers , data;
    tie(reqLine , headers , data) = receiveReq(connSocket);

    cout << "incoming request" << endl;
    cout << reqLine <<endl;

    //log request
    log(reqLine , clientStrAddr , clientAddr.sin_port);

    //do action
    process(connSocket , reqLine , headers , data);

    //close connection socket
    close(connSocket);
}

/**
 * Splits a given string on a delimiter
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
 * perform actions for GET/POST and send response
 *
 * @param socket
 * @param reqLine
 * @param headers
 * @param data
 */
void process(int socket , string reqLine , string headers , string data) {
    //parse request
    vector<string> splitReqMsg = split(reqLine , ' ');
    string reqType = splitReqMsg[0];
    string filePath = splitReqMsg[1];

    if (reqType == "GET") {
        //open an input stream
        ifstream fis("../serverFiles/" + filePath ,  ifstream::binary | ifstream::in );

        //continue if file is found
        if (fis) {

            //get file length
            fis.seekg(0, ios::end);
            int length = fis.tellg();
            fis.seekg(0, ios::beg);

            //allocate buffer to store file
            char *buffer = new char [length];

            //read file into buffer
            fis.read(buffer, length);

            //close input stream
            fis.close();

            //send response 200 ok to client
            string response = string(HTTP_VERSION) + " " + "200" + " " + "OK" + "\r\n";//status line
            response += "Content-Length: " + to_string(length) + "\r\n";
            response += "\r\n";

            send(socket, response.c_str(), response.size(), 0);
            send(socket, buffer, fis.gcount(), 0);//data

            cout << "server response " << endl;
            cout << response + "data" << endl;


        } else {
            //file not found
            //send response 404 not found
            string response = string(HTTP_VERSION) + " " + "404" + " " + "Not Found" + "\r\n";//status line

            cout << "server response " << endl;
            cout << response << endl;

            send(socket, response.c_str() , response.size(), 0);
        }


    } else { // POST request

        //store file
        storeFile(filePath , data);

        //send response 200 ok to client
        string response = string(HTTP_VERSION) + " " + "200" + " " + "Ok" + "\r\n";//status line

        cout << "server response " << endl;
        cout << response << endl;

        send(socket, response.c_str() , response.size() , 0);

    }
}

/**
 * logs request line of client request
 *
 * @param request
 * @param clientAddr
 * @param clientPort
 */
void log(string request , char clientAddr[] , int clientPort){
    ofstream ofs ("../serverFiles/log.txt", ofstream::app);

    time_t curr_time = time(NULL);
    char *tm = ctime(&curr_time);

    //remove trailing \n
    tm[strlen(tm) - 2] = '\0';

    ofs << tm << "  (" << request.substr(0, request.size()-2) << ")  " << clientAddr << "  " << to_string(clientPort) << endl;

    ofs.close();
}


/**
 * receives and parses http request of clients
 *
 * @param socket
 * @return
 */
tuple<string, string, string> receiveReq(int socket){

    regex getReg(R"(^(GET\s\S+\s\S+\r\n)([\S\s]+\r\n\r\n){0,1}$)");
    regex postReg(R"(^(POST\s\S+\s\S+\r\n)([\S\s]+\r\n\r\n)([\S\s]+)$)");
    smatch match;

    char buffer[BUFF_SIZE];
    string request = "";
    string reqLine , headers , data;
    int length = 0;

    while(true){
        length = recv(socket, &buffer , sizeof(buffer), 0);
        request.append(buffer,length);

        if(regex_match(request,match,getReg)){
            reqLine = match[1];
            headers = match[2];
            data = "";

            return make_tuple(reqLine, headers, data);

        }else if(regex_match(request,match,postReg)){
            reqLine = match[1];
            headers = match[2];
            data = match[3];

            //get content length
            regex fieldReg(R"(Content-Length: ([0-9]+)\r\n)");
            smatch fieldMatch;
            regex_search(headers,fieldMatch,fieldReg);
            int contentLen = atoi(string(fieldMatch[1]).c_str());

            while(data.size() != contentLen){
                length = recv(socket, &buffer, sizeof(buffer), 0);
                data.append(buffer,length);
            }

            return make_tuple(reqLine, headers, data);
        }
    }

}


/**
 * store uploaded file from client
 *
 * @param filePath
 * @param data
 */
void storeFile(string filePath , string data){
    ofstream ofs ("../serverFiles/" + filePath, ofstream::out | ofstream::binary);

    ofs << data;

    ofs.close();

}