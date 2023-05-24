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

// void handleClient(int clientSocket) {
//   char buffer[BUFFER_SIZE];

//   ssize_t bytesRead = recv(clientSocket, buffer, BUFFER_SIZE, 0);
//   if (bytesRead > 0) 
//   {
//     std::string request(buffer, bytesRead);
//     std::cout << "Received request: " << request << std::endl;

//     std::string responseBody = "<html><body><h1>Hello from server!</h1></body></html>";

//     std::string response = "HTTP/1.1 200 OK\r\n";
//     response += "Content-Type: text/html\r\n";
//     response += "Content-Length: " + std::to_string(responseBody.length()) + "\r\n";
//     response += "\r\n";
//     response += responseBody;

//     ssize_t bytesSent = send(clientSocket, response.c_str(), response.length(), 0);
//     if (bytesSent < 0) {
//         std::cerr << "Error sending response to client" << std::endl;
//     }
//   }

//   close(clientSocket);
// }

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


      // if (std::holds_alternative<UDPHeader>(datagram.transportHeader.header)) {
      //     std::cout << "Transport Protocol: UDP" << std::endl;
      //     UDPHeader udpHeader = std::get<UDPHeader>(datagram.transportHeader.header);
      //     // Now you can examine the udpHeader:
      //     std::cout << "UDP Source Port: " << udpHeader.sourcePort << std::endl;
      //     std::cout << "UDP Destination Port: " << udpHeader.destinationPort << std::endl;
      //     std::cout << "UDP Length: " << udpHeader.length << std::endl;
      //     std::cout << "UDP Checksum: " << udpHeader.checksum << std::endl;
      // }
      // else if (std::holds_alternative<TCPHeader>(datagram.transportHeader.header)) {
      //     std::cout << "Transport Protocol: TCP" << std::endl;
      //     TCPHeader tcpHeader = std::get<TCPHeader>(datagram.transportHeader.header);
      //     // Now you can examine the tcpHeader:
      //     std::cout << "TCP Source Port: " << tcpHeader.sourcePort << std::endl;
      //     std::cout << "TCP Destination Port: " << tcpHeader.destinationPort << std::endl;
      //     std::cout << "TCP Sequence Number: " << tcpHeader.sequenceNumber << std::endl;
      //     std::cout << "TCP Acknowledgment Number: " << tcpHeader.acknowledgmentNumber << std::endl;
      //     std::cout << "TCP Flags: " << tcpHeader.flags << std::endl;
      //     std::cout << "TCP Window Size: " << tcpHeader.windowSize << std::endl;
      //     std::cout << "TCP Checksum: " << tcpHeader.checksum << std::endl;
      //     std::cout << "TCP Urgent Pointer: " << tcpHeader.urgentPointer << std::endl;  
      // }

      // Your existing response sending code can follow here...
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

  // auto ipConfig = parser.getIpConfig();
  // auto naptConfig = parser.getNaptConfig();

  // // print ipConfig
  // std::cout << "Router IP: " << ipConfig.lanIP << std::endl;
  // std::cout << "Router WAN IP: " << ipConfig.wanIP << std::endl;
  // for (const auto& clientIp : ipConfig.clientIps) {
  //     std::cout << "Client IP: " << clientIp << std::endl;
  // }

  // // print naptConfig
  // for (const auto& [lanIp, entry] : naptConfig.lanToWanMap) {
  //     std::cout << "NAPT Entry: " << lanIp.first << ", " << lanIp.second << ", " << entry << std::endl;
  // }


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

