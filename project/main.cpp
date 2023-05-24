#include <iostream>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <netinet/in.h>
#include <string>
#include <fstream>
#include "config_parser.h"
using namespace std;

const int BUFFER_SIZE = 8192;

void handleClient(int clientSocket) {
  char buffer[BUFFER_SIZE];

  ssize_t bytesRead = recv(clientSocket, buffer, BUFFER_SIZE, 0);
  if (bytesRead > 0) 
  {
    std::string request(buffer, bytesRead);
    std::cout << "Received request: " << request << std::endl;

    std::string responseBody = "<html><body><h1>Hello from server!</h1></body></html>";

    std::string response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: text/html\r\n";
    response += "Content-Length: " + std::to_string(responseBody.length()) + "\r\n";
    response += "\r\n";
    response += responseBody;

    ssize_t bytesSent = send(clientSocket, response.c_str(), response.length(), 0);
    if (bytesSent < 0) {
        std::cerr << "Error sending response to client" << std::endl;
    }
  }

  close(clientSocket);
}







// int main() {
//   std::string szLine;

//   // First line is the router's LAN IP and the WAN IP
//   std::getline(std::cin, szLine);
//   size_t dwPos = szLine.find(' ');
//   auto szLanIp = szLine.substr(0, dwPos);
//   auto szWanIp = szLine.substr(dwPos + 1);

//   std::cout << "Server's LAN IP: " << szLanIp << std::endl
//             << "Server's WAN IP: " << szWanIp << std::endl;

//   // TODO: Modify/Add/Delete files under the project folder.

//   return 0;
// }



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

  auto ipConfig = parser.getIpConfig();
  auto naptConfig = parser.getNaptConfig();

  // print ipConfig
  std::cout << "Router IP: " << ipConfig.lanIP << std::endl;
  std::cout << "Router WAN IP: " << ipConfig.wanIP << std::endl;
  for (const auto& clientIp : ipConfig.clientIps) {
      std::cout << "Client IP: " << clientIp << std::endl;
  }

  // print naptConfig
  for (const auto& [lanIp, entry] : naptConfig.lanToWanMap) {
      std::cout << "NAPT Entry: " << lanIp.first << ", " << lanIp.second << ", " << entry << std::endl;
  }


  // ======================
  int serverSocket;
  struct sockaddr_in serverAddress;
  int port = 5152; // Replace with your desired server port

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

  // std::istringstream configInput(
  //       "192.168.1.1 98.149.235.132\n"
  //       "0.0.0.0\n"
  //       "192.168.1.100\n"
  //       "192.168.1.200\n"
  //       "\n"
  //       "192.168.1.100 8080 8080\n"
  //       "192.168.1.200 9000 443\n"
  //       "\n"
  //   );
}

