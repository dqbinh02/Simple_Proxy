#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <fstream>

using namespace std;

// A function to parse the URL and extract the server and path
void parse_url(string url, string& server, string& path)
{
    // Check if the URL contains "http://" and remove it if it does
    if (url.find("http://") != string::npos) {
        url = url.substr(7);
    }

    // Find the position of the first slash after the host name
    size_t slash_pos = url.find('/');

    // If no slash is found, the entire URL corresponds to the server name
    if (slash_pos == string::npos) {
        server = url;
        path = "/";
    }
    // Otherwise, the server name is the substring before the slash, and the path is the substring after the slash
    else {
        server = url.substr(0, slash_pos);
        path = url.substr(slash_pos);
    }
}

int main(int argc, char *argv[])
{

    // Parse the URL and extract the server and path
    string url = argv[1];
    string server, path;
    parse_url(url, server, path);
    
    // Create a TCP socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        cerr << "Failed to create socket." << endl;
        return 1;
    }

    // Get server address info
    struct addrinfo hints, *servinfo;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    getaddrinfo(server.c_str(), "http", &hints, &servinfo);

    // Connect to server
    int res = connect(sockfd, servinfo->ai_addr, servinfo->ai_addrlen);
    if (res == -1) {
        cerr << "Failed to connect to server." << endl;
        return 1;
    }
    // Send HTTP GET request
    string request = "GET " + path +" HTTP/1.1\r\nHost: " + server + "\r\n\r\n";
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

    // write data to file
    ofstream output_file;
    if (path == "/"){
        output_file.open(server+"_index.html");
    } else {
        output_file.open(argv[2]);
    }

    if (output_file.fail()) {
        cerr << "Failed to open output file." << endl;
        return 1;
    }   
    output_file << data;
    output_file.close();
    
    // close socket
    close(sockfd);

    return 0;
}

