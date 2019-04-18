#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include <fstream>
#include <vector>
#include <sstream>
#include <regex>


#define BUFF_SIZE 3000

//server port number according to http rfc
#define SERV_PORT 8080

#define HTTP_VERSION "HTTP/1.0"

using namespace std;

vector<string> split(string str, char delimiter);

tuple<string, string, string> receiveRes(int socket, string reqType);

void storeFile(string filePath, string data);

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
        vector<string> splitReq = split(req, ' ');
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

        // modify and send request to server
        if (reqType == "GET") {

            //Send the request
            string request = reqType + " " + filePath + " " + HTTP_VERSION + "\r\n";//request line

            cout << "client request " << endl;
            cout << request << endl;

            send(connSocket, request.c_str(), request.size(), 0);

            //Receive response from server
            string statusLine, headers, data;
            tie(statusLine, headers, data) = receiveRes(connSocket, reqType);

            //display response
            cout << "server response " << endl;
            cout << statusLine << endl;

            int statusCode = atoi(split(statusLine, ' ')[1].c_str());
            if (statusCode != 404) {
                //store file
                storeFile(filePath, data);
            }


        } else {

            //open an input stream
            ifstream fis("../clientFiles/" + filePath, ifstream::in | ifstream::binary);

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

                send(connSocket, request.c_str(), request.size(), 0);
                send(connSocket, buffer, fis.gcount(), 0); //data

                cout << "My request " << endl;
                cout << request + "data" << endl;

                //Receive response from server
                string statusLine, headers, data;
                tie(statusLine, headers, data) = receiveRes(connSocket, reqType);

                //display response
                cout << "server response " << endl;
                cout << statusLine << endl;

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


tuple<string, string, string> receiveRes(int socket, string reqType) {


    regex getResReg(R"(^(\S+\s\S+\s[\w ]+\r\n)([\S\s]+\r\n\r\n){0,1}([\S\s]+){0,1}$)");
    regex postResReg(R"(^(\S+\s\S+\s\S+\r\n)([\S\s]+\r\n\r\n){0,1}$)");
    smatch match;

    char buffer[BUFF_SIZE];
    string response = "";
    string statusLine, headers, data;
    int length = 0;

    if (reqType == "GET") {
        while (true) {
            length = recv(socket, &buffer, sizeof(buffer), 0);
            response.append(buffer, length);

            if (regex_match(response, match, getResReg)) {
                statusLine = match[1];
                headers = match[2];
                data = match[3];

                //if status code is 404 then no data
                if (statusLine.find("404") != -1)
                    return make_tuple(statusLine, headers, "");

                //get content length
                regex fieldReg(R"(Content-Length: ([0-9]+)\r\n)");
                smatch fieldMatch;
                regex_search(headers, fieldMatch, fieldReg);
                int contentLen = atoi(string(fieldMatch[1]).c_str());

                while(data.size() != contentLen){
                    length = recv(socket, &buffer, sizeof(buffer), 0);
                    data.append(buffer,length);
                }

                return make_tuple(statusLine, headers, data);
            }
        }

    } else {

        while (true) {
            length = recv(socket, &buffer, sizeof(buffer), 0);
            response.append(buffer, length);

            if (regex_match(response, match, postResReg)) {
                statusLine = match[1];
                headers = match[2];
                data = "";

                return make_tuple(statusLine, headers, data);
            }
        }

    }


}


void storeFile(string filePath, string data) {

    ofstream ofs("../clientFiles/" + filePath, ofstream::binary | ofstream::out);

    ofs << data;

    ofs.close();

}