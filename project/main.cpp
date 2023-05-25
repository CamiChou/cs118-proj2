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

bool isSameSubnet(const std::string first,const std::string second, const std::string subnetMask)
{
  return calculateNetworkAddress(first, subnetMask) == calculateNetworkAddress(second, subnetMask);
}

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
      fflush(stdout);
      datagram = parseIPDatagram(hexRequest);
  }

  // forward packet locally
  std::string subnetMask24 = "255.255.255.0";

  // IpConfig ipConfig = parser.getIpConfig();
  // NaptConfig naptConfig = parser.getNaptConfig();

  // print source and destination ips
  std::cout << "Source IP: " << datagram.ipHeader.sourceIP << std::endl;
  std::cout << "Destination IP: " << datagram.ipHeader.destinationIP << std::endl;
  // check if the source and destination are on the same subnet
  std::cout << "Same subnet: = " << isSameSubnet(datagram.ipHeader.sourceIP, datagram.ipHeader.destinationIP, subnetMask24) << std::endl;
  fflush(stdout);

  // Create server socket
  int destSocket;
  struct sockaddr_in destAddress;
  int destPort;
  if (datagram.ipHeader.protocol == 6) {
    // TCP
    if (std::holds_alternative<TCPHeader>(datagram.transportHeader.header)) {
        destPort = std::get<TCPHeader>(datagram.transportHeader.header).destinationPort;
    } else {
        std::cout << "TransportHeader variant doesn't hold TCPHeader." << std::endl;
        return;
    }
} else if (datagram.ipHeader.protocol == 17) {
    // UDP
    if (std::holds_alternative<UDPHeader>(datagram.transportHeader.header)) {
        destPort = std::get<UDPHeader>(datagram.transportHeader.header).destinationPort;
    } else {
        std::cout << "TransportHeader variant doesn't hold UDPHeader." << std::endl;
        return;
    }
} else {
    std::cout << "Unsupported protocol: " << datagram.ipHeader.protocol << std::endl;
    return;
}

  // Create the socket
  if ((destSocket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    perror("Failed to create socket.");
    return;
  }

  // Set up the server address
  destAddress.sin_family = AF_INET;
  destAddress.sin_addr.s_addr = INADDR_ANY;
  destAddress.sin_port = htons(destPort);

  // Bind the socket to the specified port
  if (::bind(destSocket, (struct sockaddr*)&destAddress, sizeof(destAddress)) < 0) {
    perror("Failed to bind socket to port.");
    return;
  }

  // Listen for incoming connections
  if (listen(destSocket, 3) < 0) {
    perror("Failed to listen for connections.");
    return;
  }
  
  std::cout << "Server is listening on port " << destPort << std::endl;

  // froward packet to destPort
  send(destSocket, buffer, bytesRead, 0);

  // while (true) 
  // {
  //   struct sockaddr_in clientAddress{};
  //   socklen_t clientAddressLength = sizeof(clientAddress);

  //   int clientSocket = accept(serverSocket, reinterpret_cast<struct sockaddr*>(&clientAddress), &clientAddressLength);
  //   if (clientSocket < 0) {
  //     perror("Error accepting client connection");
  //     continue;
  //   }
  //   std::cout << "New client connected" << clientSocket << std::endl;
  //   handleClient(clientSocket);
  // }

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
  // parser.print();
 

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
    std::cout << "New client connected: " << clientSocket << std::endl;
    handleClient(clientSocket);
  }
}

