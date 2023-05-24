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
  char buffer[BUFFER_SIZE];
  Datagram datagram;

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

      datagram = parseIPDatagram(hexRequest);
  }

  // forward packet locally

  close(clientSocket);
}

std::string calculateNetworkAddress(const std::string& ipAddress, const std::string& subnetMask) {
  std::vector<int> ipParts;
  std::vector<int> maskParts;
  std::vector<int> networkParts;

  // Parse IP address
  std::string part;
  std::istringstream ipStream(ipAddress);
  while (getline(ipStream, part, '.')) {
    ipParts.push_back(std::stoi(part));
  }

  // Parse subnet mask
  std::istringstream maskStream(subnetMask);
  while (getline(maskStream, part, '.')) {
    maskParts.push_back(std::stoi(part));
  }

  // Calculate network address by performing bitwise AND
  for (size_t i = 0; i < ipParts.size(); ++i) {
    networkParts.push_back(ipParts[i] & maskParts[i]);
  }

  // Construct network address string
  std::string networkAddress;
  for (size_t i = 0; i < networkParts.size(); ++i) {
    networkAddress += std::to_string(networkParts[i]);
    if (i < networkParts.size() - 1) {
      networkAddress += ".";
    }
  }
  return networkAddress;
}

bool isSameSubnet(const std::string first,const std::string second)
{
  if (first == second)
    return true;
  return false;
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

