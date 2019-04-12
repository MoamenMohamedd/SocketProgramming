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
string receiveFile(int socket);
void storeFile(string fileName , string response);

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
        string requestedFile = splitReq[1];
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
            send(connSocket, req, strlen(req), 0);

            //Receive response from server
            string responseMsg = receiveFile(connSocket);

            //display message
            cout << responseMsg;

            //store file
            storeFile(requestedFile , responseMsg);

        }else{

        }




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


string receiveFile(int socket){

    char buffer[BUFF_SIZE];
    string response = "";
    int length = 0;

    do{
        length = recv(socket, &buffer , sizeof(buffer), 0);
        response.append(buffer,length);

    }while (length > 0);

    if (length == -1){
        perror("recv error");
    }else if (length == 0){
        close(socket);
    }

    return response;
}


void storeFile(string fileName , string response){
    ofstream ofs ("../clientFiles/" + fileName, ofstream::out);


    int fieldStartInd = response.find("Content-Length");
    int fieldEndInd = response.find_first_of("\r\n",fieldStartInd);
    int length = atoi(response.substr(fieldStartInd+16 , fieldEndInd).c_str());
    
    int dataBegin = response.find_last_of("\r\n");
    string data = response.substr(dataBegin,length);

    ofs << data;

    ofs.close();


}