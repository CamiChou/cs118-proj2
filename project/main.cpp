#include <iostream>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <netinet/in.h>
#include <string>
#include <fstream>
#include <iomanip>
#include "config_parser.h"
#include "datagram_parser.h"
using namespace std;

const int BUFFER_SIZE = 8192;

void handleClient(int clientSocket) {
  // char buffer[BUFFER_SIZE];

  // ssize_t bytesRead = recv(clientSocket, buffer, BUFFER_SIZE, 0);
  // if (bytesRead > 0) 
  // {
  //   std::string request(buffer, bytesRead);
    
  //   std::stringstream ss;
  //   ss << std::hex << std::setfill('0');
  //   for (char c : request) {
  //       ss << std::setw(2) << static_cast<int>(static_cast<unsigned char>(c)) << " ";
  //   }
  //   std::string hexRequest = ss.str();
  //   std::cout << "Received request in hex: \n" << hexRequest << std::endl;

    
    // std::string responseBody = "<html><body><h1>Hello from server!</h1></body></html>";

    // std::string response = "HTTP/1.1 200 OK\r\n";
    // response += "Content-Type: text/html\r\n";
    // response += "Content-Length: " + std::to_string(responseBody.length()) + "\r\n";
    // response += "\r\n";
    // response += responseBody;

    // ssize_t bytesSent = send(clientSocket, response.c_str(), response.length(), 0);
    // if (bytesSent < 0) {
    //     std::cerr << "Error sending response to client" << std::endl;
    // }
  // }

  char buffer[BUFFER_SIZE];

  ssize_t bytesRead = recv(clientSocket, buffer, BUFFER_SIZE, 0);
  if (bytesRead > 0) {
      std::string request(buffer, bytesRead);
      std::stringstream ss;
      ss << std::hex << std::setfill('0');
      for (char c : request) {
          ss << std::setw(2) << static_cast<int>(static_cast<unsigned char>(c));
      }
      std::string hexRequest = ss.str();
      std::cout << "Received request in hex: \n" << hexRequest << std::endl;

      Datagram datagram = parseIPDatagram(hexRequest);
  }

  close(clientSocket);
}


int main() {
  // Parse the configuration files
  string configInput;
  string temp;
  while (getline(cin, temp))
  {
      configInput += temp + "\n";
  }
  
  ConfigParser parser(configInput);
  parser.parse();
  parser.print();
 

  // Create server socket
  int serverSocket;
  struct sockaddr_in serverAddress;
  int port = 5152; 

  // Create the socket
  if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    perror("Failed to create socket.");
    return 1;
  }

  // Set up the server address
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_addr.s_addr = INADDR_ANY;
  serverAddress.sin_port = htons(port);

  // Bind the socket to the specified port
    if (::bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
      perror("Failed to bind socket to port.");
      return 1;
    }

  // Listen for incoming connections
  if (listen(serverSocket, 3) < 0) {
    perror("Failed to listen for connections.");
    return 1;
  }
  
  std::cout << "Server is listening on port " << port << std::endl;

  while (true) 
  {
    struct sockaddr_in clientAddress{};
    socklen_t clientAddressLength = sizeof(clientAddress);

    int clientSocket = accept(serverSocket, reinterpret_cast<struct sockaddr*>(&clientAddress), &clientAddressLength);
    if (clientSocket < 0) {
      perror("Error accepting client connection");
      continue;
    }
    std::cout << "New client connected" << clientSocket << std::endl;
    handleClient(clientSocket);

  }
}

