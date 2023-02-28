#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <fstream>

using namespace std;

int main(int argc, char *argv[])
{
    // Create a TCP socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        cerr << "Failed to create socket." << endl;
        return 1;
    }

    // pairs server address 
    string url_server = argv[1];
    if (url_server.find("http://") != -1){
        url_server = url_server.substr(7, url_server.length()-7); //length of "http://" is 7
    }
    string url_path = "/";
    size_t index_path = url_server.find("/");
    if (index_path != -1){
        url_path += url_server.substr(index_path + 1,url_server.length()-index_path);
        url_server = url_server.substr(0,index_path);
    }

    // Get server address info
    struct addrinfo hints, *servinfo;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    getaddrinfo(url_server.c_str(), "http", &hints, &servinfo);

    // Connect to server
    int res = connect(sockfd, servinfo->ai_addr, servinfo->ai_addrlen);
    if (res == -1) {
        cerr << "Failed to connect to server." << endl;
        return 1;
    }
    // Send HTTP GET request
    string request = "GET " + url_path +" HTTP/1.1\r\nHost: " + url_server + "\r\n\r\n";
    send(sockfd, request.c_str(), request.length(), 0);

    // Receive HTTP response
    char buffer[1024];
    int bytes_received;
    string response = "", data = "";
    char data_tranfer_method = 0; // 0 : Unknown; 1 : Transfer-Encoding: chunked; 2 : Content-length
    size_t length_data = 0;


    string chunked = "";
    int length_chunked = -1;

    do {
        bytes_received = recv(sockfd, buffer, 1024, 0);
        if (bytes_received == -1) {
            cerr << "Error receiving date." << endl;
            return 1;
        }
        response += string(buffer, bytes_received);
        if (data_tranfer_method == 2) {
            data += string(buffer, bytes_received);
            if (data.length() == length_data) break;
        }
        else if (data_tranfer_method == 1)
        {
            chunked += string(buffer,bytes_received);
            while (chunked.length() >= length_chunked + 2){
                if (length_chunked == -1)
                {
                    size_t index_data = chunked.find("\r\n"); 
                    if (index_data ==-1) continue; // continue receive
                    length_chunked = static_cast<int>(stoul(chunked.substr(0,index_data),nullptr, 16));
                    chunked = chunked.substr(index_data+2, chunked.length());
                }
                else if (length_chunked == 0) break;
                else 
                {
                    data += chunked.substr(0,length_chunked + 2);
                    chunked = chunked.substr(length_chunked + 2,chunked.length());
                    length_chunked = -1;    
                }   
            }
            if (length_chunked == 0) break;
        }
        else {
            // check transfer data by content length
            size_t index_lenght_data = response.find("Content-Length: ");
            if (index_lenght_data != -1){
                data_tranfer_method = 2;
                index_lenght_data += 16;
                // check recv receive enough length data or not
                while (response.substr(index_lenght_data, response.length()).find("\r\n") == -1){
                    bytes_received = recv(sockfd, buffer, 1024,0);
                    if (bytes_received == -1) {
                        cerr << "Error receiving date." << endl;
                        return 1;
                        } 
                    response += string(buffer, bytes_received);
                }
                // find lent_data
                size_t index = index_lenght_data;
                while (response[index] != '\r') index++;
                length_data = stoi(response.substr(index_lenght_data,index - index_lenght_data));
                // receive until have data
                size_t index_data = response.find("\r\n\r\n");
                while (index_data == -1){
                    bytes_received = recv(sockfd, buffer, 1024,0);
                    if (bytes_received == -1) {
                        cerr << "Error receiving date." << endl;
                        return 1;
                        } 
                    response += string(buffer, bytes_received);
                    index_data = response.find("\r\n\r\n");
                }
                // Extract data
                data = response.substr(index_data + 4, response.length()); 
                continue;
            } 
            // check transfer data by encoding chunked
            if ( response.find("Transfer-Encoding: chunked") != -1){
                data_tranfer_method = 1;
                // receive until have first chunked
                size_t index_chunked = response.find("\r\n\r\n");
                while (index_chunked == -1){
                    bytes_received = recv(sockfd, buffer, 1024, 0);
                    if (bytes_received == -1) {
                        cerr << "Error receiving date." << endl;
                        return 1;
                    }
                    response += string(buffer, bytes_received);
                    index_chunked = response.find("\r\n\r\n");
                }
                chunked = response.substr(index_chunked + 4, response.length());
            }
        }
    } while (1);

    if (url_path == "/"){
        ofstream file1(url_server+"_index.html");
        if (file1.is_open()){
        file1 << data;
        file1.close();
        }
        else cout << "Don't success";
    } else {
        ofstream file2(argv[2]);
        if (file2.is_open()){
            file2 << data;
            file2.close();
        } else cout << "Don't success";
    }
    
    return 0;
}

