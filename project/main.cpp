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

  while (sum >> 16) {
      sum = (sum & 0xffff) + (sum >> 16);
  }

  unsigned short result = ~((unsigned short)sum);
  return result;
}

int counter = 0;
void handle_client(int client_socket) {
  address_to_socket[clientIPs[counter]]=client_socket;
  cerr << clientIPs[counter]<<" "<<client_socket<<endl;
  counter++;

  uint8_t buffer[BUFFER_SIZE];
  fflush(stdout);


  while(true){
    int num_bytes = recv(client_socket, buffer, BUFFER_SIZE, 0);

    if (num_bytes <= 0) {
      perror("Empty or Error with receiving packet data");
      return;
    }

    std::stringstream ss;
    for (int i = 0; i < BUFFER_SIZE; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(buffer[i]);
    }
    std::string hexString = ss.str();
    Datagram datagram = parseIPDatagram(hexString);

    struct iphdr ipheader = DatagramToIphdr(datagram);
    struct iphdr* iph = &ipheader;
    
    iph->ttl -= 1;
    if (iph->ttl <= 0) {
      cout << "TTL expired. Dropping packet" << endl;
      return;
    }

    iph->check = 0;
    memcpy(buffer, iph, sizeof(struct iphdr));
    unsigned short new_checksum = compute_checksum((unsigned short *)buffer, iph->ihl*4);
    iph->check = new_checksum;
    memcpy(buffer, iph, sizeof(struct iphdr));

    send(address_to_socket[datagram.ipHeader.destinationIP], buffer, num_bytes, 0);
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