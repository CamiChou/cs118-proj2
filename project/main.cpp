#include <string>
#include <vector>
#include <map>
#include <utility>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <thread>
#include <cstring>
#include <arpa/inet.h>
#include <iomanip>
#include "config_parser.h"
using namespace std;

const int BUFFER_SIZE = 2048;
map<pair<string, int>, int> lanToWan;
map<int, pair<string, int>> wanToLan;
map<string, int> AddressToSocket;
vector<string> clientIPs;
int dynamicPort = 49152;
int counter = 0;

string wan;
string lan;

void printBufferAsHex(const unsigned char *buffer, int size)
{
  for (int i = 0; i < size; ++i)
  {
    int byte = static_cast<unsigned char>(buffer[i]);
    if (byte < 16)
      std::cout << '0';
    std::cout << std::hex << byte << ' ';
  }
  std::cout << std::dec << std::endl;
}

struct PseudoHeader
{
  uint32_t sourceAddress;
  uint32_t destinationAddress;
  uint8_t reserved;
  uint8_t protocol;
  uint16_t length;
};

bool fromSameSubnet(uint32_t ip1, uint32_t ip2) {
    uint32_t ip1_host_order = ntohl(ip1);
    uint32_t ip2_host_order = ntohl(ip2);

    return (ip1_host_order >> 8) == (ip2_host_order >> 8);
}
string ipToString(uint32_t address)
{
    struct in_addr ipAddr;
    ipAddr.s_addr = address;
    char str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &ipAddr, str, INET_ADDRSTRLEN);
    return std::string(str);
}

unsigned short compute_checksum(unsigned short *addr, int len)
{
  int count = len;
  unsigned long sum = 0;

  while (count > 1)
  {
    sum += *addr++;
    count -= 2;
  }

  if (count > 0)
  {
    sum += *(unsigned char *)addr;
  }

  while (sum >> 16)
  {
    sum = (sum & 0xffff) + (sum >> 16);
  }

  unsigned short result = ~((unsigned short)sum);
  return result;
}

unsigned short transport_checksum(struct iphdr *iph, unsigned char *payload) 
{
  unsigned short transportLength = ntohs(iph->tot_len) - iph->ihl * 4;

  struct PseudoHeader psh;
  psh.sourceAddress = iph->saddr;
  psh.destinationAddress = iph->daddr;
  psh.reserved = 0;
  psh.protocol = iph->protocol;
  psh.length = htons(transportLength);

  int pseudogramSize = sizeof(struct PseudoHeader) + transportLength;
  uint8_t *pseudogram = new uint8_t[pseudogramSize];  

  memcpy(pseudogram, (char *)&psh, sizeof(struct PseudoHeader));
  memcpy(pseudogram + sizeof(struct PseudoHeader), payload, transportLength);
  unsigned short sum = compute_checksum((unsigned short *)pseudogram, pseudogramSize);

  delete[] pseudogram;
  return sum;
}

void handle_client(int client_socket, string wanIP)
{
  AddressToSocket[clientIPs[counter]] = client_socket;
  counter++;

  uint8_t buffer[BUFFER_SIZE];
  int num_bytes = recv(client_socket, buffer, BUFFER_SIZE, 0);

  while (num_bytes > 0)
  {
    // printf("Received packet from client\n\t");
    // printBufferAsHex(buffer, num_bytes);

    struct iphdr *iph = (struct iphdr *)buffer;

    // Check the IP checksum
    unsigned short checksum = compute_checksum((unsigned short *)iph, iph->ihl * 4);
    if (checksum != 0)
    {
      // cout << "IP Checksum failed. Dropping packet\n\t";
      // printBufferAsHex(buffer, num_bytes);
      num_bytes = recv(client_socket, buffer, BUFFER_SIZE, 0);
      continue;
    }
    // check the ttl
    if (iph->ttl <= 1)
    {
      // cout << "TTL expired. Dropping packet\n\t";
      // printBufferAsHex(buffer, num_bytes);
      num_bytes = recv(client_socket, buffer, BUFFER_SIZE, 0);
      continue;
    }
    iph->ttl -= 1;

    // Check transport layer checksum
    if (iph->protocol == IPPROTO_TCP)
    {
      unsigned short myChecksum = transport_checksum(iph, buffer + iph->ihl * 4);
      if (myChecksum != 0)
      {
        // cout << "TCP Checksum failed. Dropping packet\n\t";
        // printBufferAsHex(buffer, num_bytes);
        num_bytes = recv(client_socket, buffer, BUFFER_SIZE, 0);
        continue;
      }
    }
    else if (iph->protocol == IPPROTO_UDP)
    {
      unsigned short myChecksum = transport_checksum(iph, buffer + iph->ihl * 4);
      if (myChecksum != 0)
      {
        // cout << "UDP Checksum failed. Dropping packet\n\t";
        // printBufferAsHex(buffer, num_bytes);
        num_bytes = recv(client_socket, buffer, BUFFER_SIZE, 0);
        continue;
      }
    }
    
    bool sameSubnet = fromSameSubnet(iph->saddr, iph->daddr);
    bool isWan = fromSameSubnet(iph->saddr, inet_addr(wanIP.c_str()));

    if (sameSubnet && !isWan) // LAN to LAN =================================================
    {
      iph->check = 0;
      unsigned short new_checksum = compute_checksum((unsigned short *)buffer, iph->ihl * 4);
      iph->check = new_checksum;

      send(AddressToSocket[ipToString(iph->daddr)], buffer, num_bytes, 0);
      num_bytes = recv(client_socket, buffer, BUFFER_SIZE, 0);
    }
    else
    {
      bool isClient = std::find(clientIPs.begin(), clientIPs.end(), ipToString(iph->saddr)) != clientIPs.end();
      struct udphdr *udph;
      struct tcphdr *tcph;

      // get transport header
      if (iph->protocol == IPPROTO_UDP)
      {
        udph = (struct udphdr *)(buffer + iph->ihl*4);
        udph->uh_sum = 0;
      }
      else if (iph->protocol == IPPROTO_TCP)
      {
        tcph = (struct tcphdr *)(buffer + iph->ihl*4);
        tcph->th_sum = 0;
      }

      if (isClient) // LAN to WAN =================================================
      {
        printf("LAN to WAN\n");
        pair<string, int> sourceKey;
        if (iph->protocol == IPPROTO_UDP)
          sourceKey = make_pair(ipToString(iph->saddr), ntohs(udph->uh_sport));
        else if (iph->protocol == IPPROTO_TCP) 
          sourceKey = make_pair(ipToString(iph->saddr), ntohs(tcph->th_sport));
        
        // Dynamic NAPT
        if (lanToWan.count(sourceKey) == 0)
        {
          printf("Source key not found in NAPT table: %s:%d\n", sourceKey.first.c_str(), sourceKey.second);
          if (dynamicPort > 65535)
          {
            printf("Dynamic port range exceeded\n");
            dynamicPort = 49152;
          }

          printf("Adding to NAPT table\n");
          lanToWan[sourceKey] = dynamicPort;
          wanToLan[dynamicPort] = sourceKey;
          dynamicPort++;
        }
        
        // Perform translation using the NAPT table
        // print lan to wan table
        printf("LAN to WAN table\n");
        for (auto it = lanToWan.begin(); it != lanToWan.end(); it++)
        {
          printf("%s:%d -> %d\n", it->first.first.c_str(), it->first.second, it->second);
        }

        uint16_t translatedPort = lanToWan[sourceKey];
        printf("TRANSLATED PORT: %d\n", translatedPort);
        if (iph->protocol == IPPROTO_UDP)
        {
          printf("Replacing source port: %d -> %d\n", ntohs(udph->uh_sport), translatedPort);
          udph->uh_sport = htons(translatedPort);
          iph->saddr = inet_addr(wanIP.c_str());
        }
        else if (iph->protocol == IPPROTO_TCP)
        {
          printf("Replacing source port: %d -> %d\n", ntohs(tcph->th_sport), translatedPort);
          tcph->th_sport = htons(translatedPort);
          iph->saddr = inet_addr(wanIP.c_str());
        } 
        fflush(stdout);
      }
      else // is not in Client list: translate from WAN to LAN ===================
      {
        printf("WAN to LAN\n");
        u_int16_t destPort;
        if (iph->protocol == IPPROTO_UDP)
        {
          destPort = udph->uh_dport;
          int destPortInt = ntohs(destPort);

          pair<string, int> translatedIpAndPort;
          if (wanToLan.count(destPortInt) > 0)
          {
            translatedIpAndPort = wanToLan[destPortInt];
          } 
          else
          {
            // printf("not recognized...DROPPING PACKET\n\t");
            // printBufferAsHex(buffer, num_bytes);
            num_bytes = recv(client_socket, buffer, BUFFER_SIZE, 0);
            continue;
          }

          udph->uh_dport = htons(translatedIpAndPort.second);
          iph->daddr = inet_addr(translatedIpAndPort.first.c_str());
        }
        else if (iph->protocol == IPPROTO_TCP)
        {
          destPort = tcph->th_dport;
          int destPortInt = ntohs(destPort);
          pair<string, int> translatedIpAndPort;
          if (wanToLan.count(destPortInt) > 0)
          {
            translatedIpAndPort = wanToLan[destPortInt];
          }
          else
          {
            // printf("not recognized...DROPPING PACKET\n\t");
            // printBufferAsHex(buffer, num_bytes);
            num_bytes = recv(client_socket, buffer, BUFFER_SIZE, 0);
            continue;
          }

          tcph->th_dport = htons(translatedIpAndPort.second);
          iph->daddr = inet_addr(translatedIpAndPort.first.c_str());
        }
      }

      // RECOMPUTE CHECKSUMS !!!!!!!!!!!!!!!!!!!!!!!!
      if (iph->protocol == IPPROTO_UDP) 
      {
        unsigned short checksum = transport_checksum(iph, buffer + iph->ihl*4);
        udph->uh_sum = checksum;
      }
      else if (iph->protocol == IPPROTO_TCP)
      {
        unsigned short myChecksum = transport_checksum(iph, buffer + iph->ihl*4);
        tcph->th_sum = myChecksum; 
      }

      iph->check = 0;
      unsigned short new_checksum = compute_checksum((unsigned short *)buffer, iph->ihl * 4);
      iph->check = new_checksum;

      if (AddressToSocket.count(ipToString(iph->daddr)) > 0)
        send(AddressToSocket[ipToString(iph->daddr)], buffer, num_bytes, 0);
      else
        send(AddressToSocket["0.0.0.0"], buffer, num_bytes, 0);
      
      num_bytes = recv(client_socket, buffer, BUFFER_SIZE, 0);
    }
  }

  close(client_socket);
}

int main()
{

  string configInput;
  string temp;
  while (getline(cin, temp))
  {
    configInput += temp + "\n";
  }

  ConfigParser parser(configInput);
  parser.parse();

  clientIPs.push_back("0.0.0.0");
  for (auto ip : parser.getIpConfig().clientIps)
  {
    clientIPs.push_back(ip);
  }
  lanToWan = parser.getNaptConfig().getLtoW();
  wanToLan = parser.getNaptConfig().getWtoL();

  wan = parser.getIpConfig().wanIP;
  lan = parser.getIpConfig().lanIP;

  int serverSocket;
  struct sockaddr_in serverAddr;
  int port = 5152;

  serverSocket = socket(PF_INET, SOCK_STREAM, 0);

  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(port);
  serverAddr.sin_addr.s_addr = INADDR_ANY;

  int reuse = 1;
  if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
    perror("Failed to set SO_REUSEADDR option");

  if (::bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
  {
    perror("bind failed");
    return -1;
  }

  if (listen(serverSocket, 10) < 0)
  {
    perror("listen failed");
    return -1;
  }

  while (1)
  {
    struct sockaddr_in client_address;
    socklen_t client_len = sizeof(client_address);

    int client_socket = accept(serverSocket, (struct sockaddr *)&client_address, &client_len);
    if (client_socket < 0)
    {
      perror("accept failed");
      return -1;
    }

    thread client_thread(handle_client, client_socket, wan);
    client_thread.detach();
  }
  return 0;
}

