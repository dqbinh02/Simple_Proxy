#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

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
        else if (data_tranfer_method == 1){ 
            if (response.substr(response.length() - 5, 5) == "0\r\n\r\n") break;
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
            }
        }
    } while (1);
    // if Transfer-Encoding: chunked
    if (data_tranfer_method==1){
        data = response.substr(response.find("\r\n\r\n")+4,response.length());
    }

    

    cout << data;

    return 0;

    // //Receive HTTP response
    // char buffer[1024];
    // string response = "";
    // int bytes_received;
	// string data;
    // char data_transfer_method;


    // // for content-lenght 
    // size_t indexData;
    // size_t lentData;

    // do {
    //     bytes_received = recv(sockfd, buffer, 1024, 0);
    //     if (bytes_received == -1) {
    //         cerr << "Error receiving data." << endl;
    //         return 1;
    //     }
    //     response += string(buffer, bytes_received);

		
    //     if (data_transfer_method == 1){
    //         data += string(buffer, bytes_received);    
	// 	    if (data.length() == lentData) break;
    //     }
    //     else if (data_transfer_method == 2){
    //         if (string(buffer,bytes_received) == "0\r\n\r\n") break;
    //         indexData = string(buffer,bytes_received).find("\r\n");
    //         indexData += 2;
    //         data += string(buffer,bytes_received).substr(indexData,bytes_received);
    //     }
    //     else {
    //         size_t indexLentData = response.find("Content-Length: ");
    //         if (indexLentData!=-1){

	// 	        indexLentData = indexLentData + 16;
	// 	        size_t i = response.substr(indexLentData,response.length()).find("\r\n");
    //             if (i != -1){
    //                 data_transfer_method = 1;
    //                 while (response[i] != '\r') i++;	
	// 	            lentData = stoi(response.substr(indexLentData,indexLentData+i));
		        
    //                 indexData = response.find("\n\r") + 3;
	// 	            data = response.substr(indexData, response.length());
	// 	            if (data.length() == lentData) break;
    //             }
    //             continue;
    //         }
    //         if (response.find("Transfer-Encoding: chunked") != -1){
    //             data_transfer_method = 2;
    //             size_t indexChunked = response.find("\n\r") + 3;
    //             if (response[indexChunked] == '0') break;  
    //             indexData = response.substr(indexChunked,response.length()).find("\r\n") + 2;
    //             data = response.substr(indexData,response.length());
    //         }
    //     }
    // } while (bytes_received > 0);



    // // Display HTTP response
    // cout << data;

    // // Close socket
    // close(sockfd);


    // return 0;
}

