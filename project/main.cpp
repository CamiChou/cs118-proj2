// #include <iostream>
// #include <sstream>
// #include <cstring>
// #include <unistd.h>
// #include <netinet/in.h>
// #include <netinet/ip.h>
// #include <string>
// #include <fstream>
// #include <iomanip>
// #include <thread>
// #include <arpa/inet.h>
// #include "config_parser.h"
// #include "datagram_parser.h"

// using namespace std;

// std::string calculateNetworkAddress(const std::string& ipAddress, const std::string& subnetMask) {
//   std::vector<int> ipParts;
//   std::vector<int> maskParts;
//   std::vector<int> networkParts;

//   // Parse IP address
//   std::string part;
//   std::istringstream ipStream(ipAddress);
//   while (getline(ipStream, part, '.')) {
//     ipParts.push_back(std::stoi(part));
//   }

//   // Parse subnet mask
//   std::istringstream maskStream(subnetMask);
//   while (getline(maskStream, part, '.')) {
//     maskParts.push_back(std::stoi(part));
//   }

//   // Calculate network address by performing bitwise AND
//   for (size_t i = 0; i < ipParts.size(); ++i) {
//     networkParts.push_back(ipParts[i] & maskParts[i]);
//   }

//   // Construct network address string
//   std::string networkAddress;
//   for (size_t i = 0; i < networkParts.size(); ++i) {
//     networkAddress += std::to_string(networkParts[i]);
//     if (i < networkParts.size() - 1) {
//       networkAddress += ".";
//     }
//   }
//   return networkAddress;
// }

// bool isSameSubnet(const std::string first,const std::string second, const std::string subnetMask)
// {
//   return calculateNetworkAddress(first, subnetMask) == calculateNetworkAddress(second, subnetMask);
// }


// void forward_UDP_packet(const char* packet, size_t packet_length, std::string destination_ip, uint16_t destination_port) {
//   printf("Forwarding UDP packet to %s:%d\n", destination_ip.c_str(), destination_port);
//   // Create a socket.
//   int socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
//   if (socket_fd < 0) {
//       perror("Failed to create socket");
//       return;
//   }

//   // Set up the destination address.
//   struct sockaddr_in dest_address;
//   dest_address.sin_family = AF_INET;
//   dest_address.sin_port = htons(destination_port);
//   int result = inet_pton(AF_INET, destination_ip.c_str(), &dest_address.sin_addr);
//   if (result == 0) {
//       fprintf(stderr, "Invalid IP address: %s\n", destination_ip.c_str());
//       close(socket_fd);
//       return;
//   } else if (result < 0) {
//       perror("Failed to set destination IP address");
//       close(socket_fd);
//       return;
//   }
//   // packet contents
//   printf("Packet contents: ");
//   for (size_t i = 0; i < packet_length; ++i) {
//       printf("%02x ", static_cast<unsigned char>(packet[i]));
//   }
//   // Send the packet.
//   if (sendto(socket_fd, packet, packet_length, 0, (struct sockaddr*)&dest_address, sizeof(dest_address)) < 0) {
//       perror("Failed to send packet");
//   }

//   // Close the socket.
//   close(socket_fd);
// }
// void forward_TCP_packet(const char* packet, size_t packet_length, std::string destination_ip, uint16_t destination_port) {
//   printf("Forwarding TCP packet to %s:%d\n", destination_ip.c_str(), destination_port);

//   // Create a socket.
//   int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
//   if (socket_fd < 0) {
//       perror("Failed to create socket");
//       return;
//   }

//   // Set up the destination address.
//   struct sockaddr_in dest_address;
//   dest_address.sin_family = AF_INET;
//   dest_address.sin_port = htons(destination_port);
//   int result = inet_pton(AF_INET, destination_ip.c_str(), &dest_address.sin_addr);
//   if (result == 0) {
//       fprintf(stderr, "Invalid IP address: %s\n", destination_ip.c_str());
//       close(socket_fd);
//       return;
//   } else if (result < 0) {
//       perror("Failed to set destination IP address");
//       close(socket_fd);
//       return;
//   }

//   // Connect to the destination.
//   if (connect(socket_fd, (struct sockaddr*)&dest_address, sizeof(dest_address)) < 0) {
//       perror("Failed to connect to destination");
//       close(socket_fd);
//       return;
//   }

//   // Send the packet.
//   if (send(socket_fd, packet, packet_length, 0) < 0) {
//       perror("Failed to send packet");
//   }

//   // Close the socket.
//   close(socket_fd);
// }



// const int BUFFER_SIZE = 2048;
// map<string, int> addressToSocket;
// vector<string> clientIps;
// int counter = 0;

// void handle_client(int client_socket) {
//   printf("Client connected at socket: %d\n", client_socket);

//   addressToSocket[clientIps[counter]] = client_socket;
//   printf("The socket is associated with %s\n", clientIps[counter].c_str());
//   counter++;

//   uint8_t buffer[BUFFER_SIZE];

//   // Datagram datagram;
//   while (true) {
//     printf("Waiting for request...\n");
//     int bytesRead = recv(client_socket, buffer, sizeof(buffer), 0);
//     printf("Bytes read: %d\n", bytesRead);

//     if (bytesRead < 0) {
//       perror("Failed to read from client socket");
//       return;
//     } else if (bytesRead == 0) {
//       printf("Client disconnected\n");
//       return;
//     }
//     printf("Received request\n");
//     struct iphdr* ipHeader = (struct iphdr*) buffer;

//     // print ipheader
//     printf("IP Header: \n");
//     printf("Version: %d\n", ipHeader->version);
//     printf("Header Length: %d\n", ipHeader->ihl);
//     printf("Type of Service: %d\n", ipHeader->tos);
//     printf("Total Length: %d\n", ipHeader->tot_len);
//     printf("Identification: %d\n", ipHeader->id);
//     printf("Flags: %d\n", ipHeader->frag_off);
//     printf("Time to Live: %d\n", ipHeader->ttl);
//     printf("Protocol: %d\n", ipHeader->protocol);
//     printf("Header Checksum: %d\n", ipHeader->check);
//     printf("Source IP: %d\n", ipHeader->saddr);
//     printf("Destination IP: %d\n", ipHeader->daddr);
//     fflush(stdout);

//     // // decreast ttl
//     // if (ipHeader->ttl > 0) {
//     //   ipHeader->ttl--;
//     // } else {
//     //   printf("TTL is 0, dropping packet\n");
//     //   return;
//     // }

//     // // deal with checksum


//     // uint32_t sourceIP = ipHeader->saddr;
//     // struct in_addr addr;
//     // addr.s_addr = sourceIP;
//     // inet_ntop(AF_INET, &(addr.s_addr), sourceIP, INET_ADDRSTRLEN);

//     // uint32_t destinationIP;
//     // char destinationIP[INET_ADDRSTRLEN];
//     // inet_ntop(AF_INET, &(destinationIP.s_addr), destinationIP, INET_ADDRSTRLEN);
//     // send(addressToSocket[destinationIP], buffer, bytesRead, 0);

// //============================
//     // if (bytesRead > 0) {
//     //   printf("Received request\n");
//     //   std::string request(buffer, bytesRead);
//     //   std::stringstream ss;
//     //   ss << std::hex << std::setfill('0');
//     //   for (char c : request) {
//     //       ss << std::setw(2) << static_cast<int>(static_cast<unsigned char>(c));
//     //   }
//     //   std::string hexRequest = ss.str();
//     //   std::cout << "Received request in hex: \n" << hexRequest << std::endl;
//     //   fflush(stdout);
//     //   datagram = parseIPDatagram(hexRequest);
//     // }
//     // // forward packet
//     // if (std::holds_alternative<TCPHeader>(datagram.transportHeader.header)) {
//     //   forward_TCP_packet(buffer, bytesRead, datagram.ipHeader.destinationIP, std::get<TCPHeader>(datagram.transportHeader.header).destinationPort);
//     // } else {
//     //   send(addressToSocket[datagram.ipHeader.destinationIP], buffer, bytesRead, 0);
//     // }

//   }
//     close(client_socket);
// }


// int main() {
//   // Parse the configuration files
//   string configInput;
//   string temp;
//   while (getline(cin, temp))
//   {
//     configInput += temp + "\n";
//   }
  
//   ConfigParser parser(configInput);
//   parser.parse();
//   clientIps = parser.getIpConfig().clientIps;
//   parser.print();
//  //====================================

//   // Start the server
//   int server_socket;
//   struct sockaddr_in server_address;
//   int port = 5152;

//   if ((server_socket = socket(PF_INET, SOCK_STREAM, 0)) == 0) {
//       perror("Failed to create socket.");
//       return -1;
//   }

//   server_address.sin_family = AF_INET;
//   server_address.sin_addr.s_addr = INADDR_ANY;
//   server_address.sin_port = htons(port);

//   if (::bind(server_socket, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) {
//       perror("Failed to bind socket to port.");
//       return -1;
//   }

//   if (listen(server_socket, 10) < 0) {
//       perror("Failed to listen for connections.");
//       return -1;
//   }

//   std::cout << "Server is listening on port " << port << std::endl;

//   while (true) {
//       struct sockaddr_in client_address;
//       socklen_t client_address_length = sizeof(client_address);

//       int client_socket = accept(server_socket, (struct sockaddr*)&client_address, &client_address_length);
//       if (client_socket < 0) {
//           perror("Error accepting client connection");
//           // continue;
//       }
//       thread clientThread(handle_client, client_socket);
//       clientThread.detach();
//   }

//   return 0;
// }

//=============================

#include <string>
#include <vector>
#include <map>
#include <utility>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <unistd.h>
#include <thread>
#include <cstring>
#include <arpa/inet.h>
#include <iomanip> 
#include "config_parser.h"
#include "datagram_parser.h"

using namespace std;

const int BUFFER_SIZE = 2048;
map<pair<string, int>, int> naptTable;
map<string, int> address_to_socket;
vector<string> clientIPs;

unsigned short compute_checksum(unsigned short *addr, int len) {
  int count = len;
  unsigned long sum = 0;

  while (count > 1) {
      sum += *addr++;
      count -= 2;
  }

  if (count > 0) {
      sum += *(unsigned char *)addr;
  }

  // fold the sum to 16 bits: add carrier to result
  while (sum >> 16) {
      sum = (sum & 0xffff) + (sum >> 16);
  }

  // one's complement
  unsigned short result = ~((unsigned short)sum);
  return result;
}

int counter = 0;
void handle_client(int client_socket) {
  address_to_socket[clientIPs[counter]]=client_socket;
  cerr << clientIPs[counter]<<" "<<client_socket<<endl;
  counter++;

  uint8_t buffer[BUFFER_SIZE];
  cout << "in handle_client" << endl;
  cout << "socket #:" << client_socket << endl;
  fflush(stdout);

  while(true){
    int num_bytes = recv(client_socket, buffer, BUFFER_SIZE, 0);
    printf("Received %d bytes\n", num_bytes);

    if (num_bytes < 0) {
      perror("Error with receiving packet data");
      return;
    }
    else if(num_bytes > 0){
      cout << "Received packet with content" << endl;
      // for (int i = 0; i < num_bytes; i++) {
      //   if(i % 4==0)
      //     cout<<"\n";
      //   cout << std::hex << std::setw(2) << std::setfill('0') << (int)buffer[i] << " ";
      // }
    }
    else {
      perror("empty packet");
      return;
    }
    fflush(stdout);

    std::stringstream ss;
    for (int i = 0; i < BUFFER_SIZE; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(buffer[i]);
    }
    std::string hexString = ss.str();

    Datagram datagram = parseIPDatagram(hexString);
    cout << "Parsed datagram" << endl;
    cout << "version: " << datagram.ipHeader.version << endl;
    cout << "header length: " << datagram.ipHeader.ihl << endl;
    cout << "type of service: " << datagram.ipHeader.typeOfService << endl;
    cout << "total length: " << datagram.ipHeader.totalLength << endl;
    cout << "identification: " << datagram.ipHeader.identification << endl;
    cout << "flags and fragment offset: " << datagram.ipHeader.flagsAndFragmentOffset << endl;
    cout << "time to live: " << datagram.ipHeader.ttl << endl;
    cout << "protocol: " << datagram.ipHeader.protocol << endl;
    cout << "header checksum: " << datagram.ipHeader.headerChecksum << endl;
    cout << "source address: " << datagram.ipHeader.sourceIP << endl;
    cout << "destination address: " << datagram.ipHeader.destinationIP << endl;
    fflush(stdout);

  // decrement time to live
  datagram.ipHeader.ttl -= 1;
  if (datagram.ipHeader.ttl <= 0) {
    cout << "TTL expired. Dropping packet" << endl;
    return;
  }

  // recalculate checksum


//==========================
    struct iphdr *iph = (struct iphdr *)buffer;

    // Decrease TTL
    if (iph->ttl > 0) {
        iph->ttl--;
    } else {
        cerr << "TTL expired" << "\n";
        continue; 
        // Handle TTL expired, usually by dropping the packet
    }

    // Zero out the checksum in the header
    iph->check = 0;
    // Compute the new checksum
    unsigned short new_checksum = compute_checksum((unsigned short *)iph, iph->ihl*4);
    // Set the checksum in the header
    iph->check = new_checksum; 

    if(num_bytes > 0){
        cerr<<"Post change"<<endl;
        for (int i = 0; i < num_bytes; i++) {
            if(i % 4==0)
                cerr<<"\n";
            cerr << hex <<static_cast<unsigned int>(buffer[i]) << " ";
        }
    }
    cerr << "\n";
    if (iph->protocol == 6) {
        cerr << "tcp" << "\n";
    } else if (iph->protocol == 17) {
        cerr << "udp" << "\n";
    }
    
    uint32_t sourceAddress = iph->saddr;
    // Convert the source address from network byte order to a human-readable format
    struct in_addr addr;
    addr.s_addr = sourceAddress;
    char sourceIP[INET_ADDRSTRLEN]; //source address in char arr
    inet_ntop(AF_INET, &(addr.s_addr), sourceIP, INET_ADDRSTRLEN);
    cerr<< "Source IP: " << sourceIP << endl;
    uint32_t destinationAddress = iph->daddr;
    struct in_addr destAddr;
    destAddr.s_addr = destinationAddress;
    char destIP[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(destAddr.s_addr), destIP, INET_ADDRSTRLEN);
    cerr << "Destination IP: " << destIP << endl;
    //==========================

    send(address_to_socket[destIP], buffer, num_bytes, 0);
  }
  close(client_socket);
}


int main() {
  string configInput;
  string temp;
  while (getline(cin, temp))
  {
    configInput += temp + "\n";
  }
  
  ConfigParser parser(configInput);
  parser.parse();
  // parser.print();

  clientIPs.push_back("0.0.0.0");
  for (auto ip : parser.getIpConfig().clientIps) {
      clientIPs.push_back(ip);
  }
  naptTable = parser.getNaptConfig().convertToMap();
  // NEED TO MAKE SURE THAT THE NAPT TABLE IS VALID

  int serverSocket;
  struct sockaddr_in serverAddr;
  int port = 5152;

  serverSocket = socket(PF_INET, SOCK_STREAM, 0);

  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(port);
  serverAddr.sin_addr.s_addr = INADDR_ANY;

  if(::bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0) {
    perror("bind failed");
    return -1;
  }

  if (listen(serverSocket, 10) < 0) {
    perror("listen failed");
    return -1;
  }

  while(1) {
      cout << "Server waiting for connection...\n" << endl;

      struct sockaddr_in client_address;
      socklen_t client_len = sizeof(client_address);

      int client_socket = accept(serverSocket, (struct sockaddr*)&client_address, &client_len);
      if (client_socket < 0) {
          perror("accept failed");
          return -1;
      }
      cout << "Connection accepted, spawning handler thread...\n" << endl;

      thread client_thread(handle_client, client_socket);
      client_thread.detach();
  }
  return 0;
}