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

#define HTTP_VERSION "HTTP/1.0"

using namespace std;

vector<string> split(string str, char delimiter);
string receiveResponse(int socket, string reqType);
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
            string request = reqType + " " + filePath + " " + HTTP_VERSION + "\r\n";//request line

            cout << "client request " << endl;
            cout << request << endl;

            send(connSocket, request.c_str(), request.size(), 0);

            //Receive response from server
            string responseMsg = receiveResponse(connSocket , reqType);

            //display message
            cout << "client response " << endl;
            cout << responseMsg << endl;

            //store file
            storeFile(filePath , responseMsg);

        }else {

            //open an input stream
            ifstream fis("../clientFiles/" + filePath , ifstream::in | ifstream::binary);

            //continue if file is found
            if (fis) {

                //get file length
                fis.seekg(0, fis.end);
                int length = fis.tellg();
                fis.seekg(0, fis.beg);

                //allocate buffer to store file
                char *buffer = new char[length];

                //read file into buffer
                fis.read(buffer, length);

                //close input stream
                fis.close();

                //send file to server
                string request = reqType + " " + filePath + " " + HTTP_VERSION + "\r\n";//request line
                request += "Content-Length: " + to_string(length) + "\r\n";
                request += "\r\n";
                request += buffer;//data

                cout << "client request " << endl;
                cout << request << endl;

                send(connSocket, request.c_str(), request.size(), 0);

                //Receive response from server
                string responseMsg = receiveResponse(connSocket , reqType);

                //display message
                cout << "client response " << endl;
                cout << responseMsg << endl;

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


string receiveResponse(int socket, string reqType){

    char buffer[BUFF_SIZE];
    string response = "";
    int length = 0;

    if(reqType == "GET"){
        int beginData;
        while (true){
            length = recv(socket, &buffer , sizeof(buffer), 0);
            response.append(buffer,length);
            beginData = response.find("\r\n\r\n")+1;

            if(beginData != 0){
                int counter = response.size() - beginData;
                int beginContentLen = response.find("Content-Length: ") + 16;
                int contentLen = atoi(response.substr(beginContentLen , response.find('\r' , beginContentLen)).c_str());
                while(counter < contentLen){
                    length = recv(socket, &buffer , sizeof(buffer), 0);
                    response.append(buffer,length);
                    counter += length;

                }
                break;
            }
        }

    }else{
        //end at \r\n\r\n if headers else \r\n if not
        while (true){
            length = recv(socket, &buffer , sizeof(buffer), 0);
            response.append(buffer,length);
            if(response.find("\r\n")){
                break;
            }

        }
    }

    return response;
}


void storeFile(string filePath , string response){

    ofstream ofs ("../clientFiles/" + filePath, ofstream::out | ofstream::binary);

    int beginContentLen = response.find("Content-Length: ") + 16;
    int contentLen = atoi(response.substr(beginContentLen , response.find('\r' , beginContentLen)).c_str());

    string data = response.substr(beginContentLen ,contentLen);

    ofs << data;

    ofs.close();


}