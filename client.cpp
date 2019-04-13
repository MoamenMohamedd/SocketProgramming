#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include <fstream>
#include <vector>
#include <sstream>


#define BUFF_SIZE 3000
//server port number according to http rfc
#define SERV_PORT 8080

using namespace std;

vector<string> split(string str, char delimiter);
string receiveMsg(int socket);
void storeFile(string filePath , string response);

int main(int argc, char const *argv[]) {

    //define server address
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERV_PORT);

    //loop on all requests in file
    ifstream requests("../clientFiles/requests.txt");

    char req[100];
    while (requests.getline(req, 100)) {

        //split client request
        vector<string> splitReq = split(req,' ');
        string reqType = splitReq[0];
        string filePath = splitReq[1];
        string hostAddr = splitReq[2];

        //Convert IPv4 and IPv6 addresses from text to binary form
        //and save the supplied host address into server address
        if (inet_pton(AF_INET, hostAddr.c_str(), &serverAddr.sin_addr) == -1) {
            perror("Invalid address");
            exit(EXIT_FAILURE);
        }

        //Initialize a socket using IPv4 and TCP
        int connSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (connSocket == -1) {
            perror("Socket creation error");
            exit(EXIT_FAILURE);
        }


        //Connect to server
        if (connect(connSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) == -1) {
            perror("Connection Failed");
            exit(EXIT_FAILURE);
        }



        if(reqType == "GET"){

            //Send the request
            string request = string(req) + "\r\n";//request line
            send(connSocket, request.c_str(), request.size(), 0);

            //Receive response from server
            string responseMsg = receiveMsg(connSocket);

            //display message
            cout << responseMsg << endl;

            //store file
            storeFile(filePath , responseMsg);

        }else {
            //open an input stream
            ifstream fis("../clientFiles/" + filePath);

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

                //send file to server
                string request = string(req) + "\r\n";//request line
                request += buffer;
                request += "\r\n";//data

                send(connSocket, request.c_str(), request.size(), 0);

                //Receive response from server
                string responseMsg = receiveMsg(connSocket);

                //display message
                cout << responseMsg;

            }
        }



        close(connSocket);

    }

    requests.close();

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


string receiveMsg(int socket){

    char buffer[BUFF_SIZE];
    string response = "";
    int length = 0;

    while (true){
        length = recv(socket, &buffer , sizeof(buffer), 0);
        response.append(buffer,length);
        if(response[response.size()-1] == '\n'){
            break;
        }

    }

    if (length == -1){
        perror("recv error");
    }

    return response;
}


void storeFile(string filePath , string response){
    ofstream ofs ("../clientFiles/" + filePath, ofstream::out);


    int dataBegin = response.find_first_of('\n');
    int dataEnd = response.find_last_of('\n');


    int length = dataEnd - dataBegin;

    string data = response.substr(dataBegin+1,length);

    ofs << data;

    ofs.close();

}