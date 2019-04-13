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

#define SERV_PORT 8080
#define BUFF_SIZE 3000

#define HTTP_VERSION "HTTP/1.0"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"

using namespace std;

vector<string> split(string str, char delimiter);
string receiveReq(int socket);
void storeFile(string filePath , string request);
void process(int socket , string reqMsg);
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

        thread thread_obj(serve, connSocket);
        thread_obj.join();

    }


}

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
    string request = receiveReq(connSocket);

    cout << "incoming request" << endl;
    cout << request <<endl;

    //parse request
    string reqLine = request.substr(0,request.find("\r\n"));

    //log request
    log(reqLine , clientStrAddr , clientAddr.sin_port);

    //do action
    process(connSocket , request);

    //close connection socket
    close(connSocket);
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
void process(int socket , string reqMsg) {
    vector<string> splitReqMsg = split(reqMsg , ' ');
    string reqType = splitReqMsg[0];
    string filePath = splitReqMsg[1];

    if (reqType == "GET") {
        //open an input stream
        ifstream fis("../serverFiles/" + filePath ,  ifstream::binary | ifstream::in );

        //continue if file is found
        if (fis) {

            //get file length
            fis.seekg(0, fis.end);
            int length = fis.tellg();
            fis.seekg(0, fis.beg);

            cout << "length of file = " << to_string(length) << endl;

            //allocate buffer to store file
            char *buffer = new char [length];

            //read file into buffer
            fis.read(buffer, length);
            
            cout << "buffer content" << endl;
            cout << buffer << endl;

            //close input stream
            fis.close();

            //send file buffer to client
            string response = string(HTTP_VERSION) + " " + "200" + " " + "OK" + "\r\n";//status line
            response += "Content-Length: " + to_string(length) + "\r\n";
            response += "\r\n";
            response += buffer;//data

            cout << "server response " << endl;
            cout << response << endl;

            cout << fis.gcount() << endl;
            cout << sizeof(buffer) << endl;
            cout << strlen(response.c_str()) << endl;
            cout << response.size() << endl;

//            send(socket, response.c_str(), length+42, 0);
            send(socket, response.c_str(), response.size(), 0);



        } else {
            //file not found
            string response = string(HTTP_VERSION) + " " + "404" + " " + "Not Found" + "\r\n";//status line

            cout << "server response " << endl;
            cout << response << endl;

            send(socket, response.c_str() , response.size(), 0);
        }


    } else { // POST request
        storeFile(filePath , reqMsg);

        //send response to client
        string response = string(HTTP_VERSION) + " " + "200" + " " + "Ok" + "\r\n";//status line

        cout << "server response " << endl;
        cout << response << endl;

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


string receiveReq(int socket){

    //get request type
    char reqType[3];
    recv(socket, &reqType, sizeof(reqType), MSG_PEEK);

    char buffer[BUFF_SIZE];
    string request = "";
    int length = 0;

    if(string(reqType) == "GET"){
        //end at \r\n\r\n if headers else \r\n if not
        while (true){
            length = recv(socket, &buffer , sizeof(buffer), 0);
            request.append(buffer,length);
            if(request.find("\r\n") != -1){
                break;
            }

        }
    }else{
        int beginData;
        while (true){
            length = recv(socket, &buffer , sizeof(buffer), 0);
            request.append(buffer,length);
            beginData = request.find("\r\n\r\n")+1;

            if(beginData != 0){
                int counter = request.size() - beginData;
                int beginContentLen = request.find("Content-Length: ") + 16;
                int contentLen = atoi(request.substr(beginContentLen , request.find('\r' , beginContentLen)).c_str());
                while(counter < contentLen){
                    length = recv(socket, &buffer , sizeof(buffer), 0);
                    request.append(buffer,length);
                    counter += length;
                }
                break;
            }
        }
    }

    return request;
}



void storeFile(string filePath , string request){
    ofstream ofs ("../serverFiles/" + filePath, ofstream::out | ofstream::binary);

    int beginContentLen = request.find("Content-Length: ") + 16;
    int endContentLen = request.find('\r' , beginContentLen);
    int contentLen = atoi(request.substr(beginContentLen , endContentLen).c_str());

    string data = request.substr(request.find("\r\n\r\n")+4 ,contentLen);

    ofs << data;

    ofs.close();

}