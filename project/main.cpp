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
map<string, int> address_to_socket;
vector<string> clientIPs;
int dynamicPort = 49152;

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
    // Convert the IP addresses from network byte order to host byte order
    uint32_t ip1_host_order = ntohl(ip1);
    uint32_t ip2_host_order = ntohl(ip2);

    // Compare the top 24 bits (the /24 subnet)
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

unsigned short tcp_checksum(struct iphdr *iph, unsigned char *payload, int payload_len)
{
    struct PseudoHeader psh;
    psh.sourceAddress = iph->saddr;
    psh.destinationAddress = iph->daddr;
    psh.reserved = 0;
    psh.protocol = IPPROTO_TCP;
    psh.length = htons(htons(iph->tot_len) - iph->ihl*4);

    int psize = sizeof(struct PseudoHeader) + htons(iph->tot_len) - iph->ihl*4;
    auto *pseudogram = new uint8_t[psize];

    memcpy(pseudogram, (char *)&psh, sizeof(struct PseudoHeader));
    memcpy(pseudogram + sizeof(struct PseudoHeader), payload, htons(iph->tot_len) - iph->ihl*4);
    unsigned short sum = compute_checksum((unsigned short *)pseudogram, psize);

    delete[] pseudogram;
    return sum;
}
unsigned short udp_checksum(struct iphdr *iph, struct udphdr *udph, unsigned char *payload, int payload_len) {
    struct PseudoHeader psh;
    psh.sourceAddress = iph->saddr;
    psh.destinationAddress = iph->daddr;
    psh.reserved = 0;
    psh.protocol = IPPROTO_UDP;
    psh.length = htons(sizeof(struct udphdr) + payload_len);

    int psize = sizeof(struct PseudoHeader) + sizeof(struct udphdr) + payload_len;
    uint8_t *pseudogram = new uint8_t[psize];

    memcpy(pseudogram, &psh, sizeof(struct PseudoHeader));
    memcpy(pseudogram + sizeof(struct PseudoHeader), udph, sizeof(struct udphdr));
    memcpy(pseudogram + sizeof(struct PseudoHeader) + sizeof(struct udphdr), payload, payload_len);

    unsigned short checksum = compute_checksum((unsigned short *)pseudogram, psize);

    delete[] pseudogram;
    return checksum;
}

int counter = 0;
void handle_client(int client_socket, string wanIP)
{
  address_to_socket[clientIPs[counter]] = client_socket;
  counter++;

  uint8_t buffer[BUFFER_SIZE];
  fflush(stdout);

  while (true)
  {
    int num_bytes = recv(client_socket, buffer, BUFFER_SIZE, 0);
    if (num_bytes == -1)
    {
      perror("Empty or Error with receiving packet data");
      continue;
    }
    else if (num_bytes == 0)
    {
      printf("Client disconnected\n");
      break;
    }

    std::stringstream ss;
    for (int i = 0; i < BUFFER_SIZE; ++i)
    {
      ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(buffer[i]);
    }

    std::string hexString = ss.str();

    struct iphdr *iph = (struct iphdr *)buffer;

    // check the ip checksum
    unsigned short checksum = compute_checksum((unsigned short *)iph, iph->ihl * 4);
    if (checksum != 0)
    {
      cout << "IP Checksum failed. Dropping packet" << endl;
      return;
    }

    if (iph->ttl <= 1)
    {
      cout << "TTL expired. Dropping packet" << endl;
      return;
    }
    iph->ttl -= 1;

    struct tcphdr *tcph; 
    struct udphdr *udph;
    if (iph->protocol == IPPROTO_TCP)
    {
      unsigned short myChecksum = tcp_checksum(iph, buffer + iph->ihl * 4, htons(iph->tot_len) - iph->ihl * 4);
      if (myChecksum != 0)
      {
        cout << "TCP Checksum failed. Dropping packet" << endl;
        continue;
      }
    }
    else if (iph->protocol == IPPROTO_UDP)
    {
      unsigned short myChecksum = udp_checksum(iph, (struct udphdr *)(buffer + iph->ihl * 4), buffer + iph->ihl * 4 + sizeof(struct udphdr), htons(iph->tot_len) - iph->ihl * 4 - sizeof(struct udphdr));
      if (myChecksum != 0)
      {
        cout << "UDP Checksum failed. Dropping packet" << endl;
        continue;
      }
    }
    
    bool sameSubnet = fromSameSubnet(iph->saddr, iph->daddr);
    printf("Source IP: %s\n", ipToString(iph->saddr).c_str());
    printf("Destination IP: %s\n", ipToString(iph->daddr).c_str());
    std::cout << "Same subnet: = " << sameSubnet << std::endl;
    fflush(stdout);

    if (sameSubnet)
    {
      iph->check = 0;
      memcpy(buffer, iph, sizeof(struct iphdr));

      unsigned short new_checksum = compute_checksum((unsigned short *)buffer, iph->ihl * 4);
      iph->check = new_checksum;

      send(address_to_socket[ipToString(iph->daddr)], buffer, num_bytes, 0);
    }
    else
    {
      std::cout << "NOT SAME SUBNET, GOING NAT IT UP" << std::endl;
      // check if the source ip is in the vector of client ips
      bool isClient = std::find(clientIPs.begin(), clientIPs.end(), ipToString(iph->saddr)) != clientIPs.end();
      struct udphdr *udph;
      struct tcphdr *tcph;
      PseudoHeader myPsuedo;
      // Check the active type of the variant
      if (iph->protocol == IPPROTO_UDP)
      {
        // Variant is UDPHeader
        udph = (struct udphdr *)(buffer + iph->ihl*4);
        udph->uh_sum = 0;
        std::cout << "WOW its a udp header" << std::endl;
      }
      else if (iph->protocol == IPPROTO_TCP)
      {
        tcph = (struct tcphdr *)(buffer + sizeof(struct iphdr));
        tcph->th_sum = 0;
        std::cout << "WOW its a tcp header" << std::endl;
      }

      if (isClient) // LAN to WAN
      {
        printf("LAN to WAN\n");
        pair<string, int> sourceKey;
        if (iph->protocol == IPPROTO_UDP)
        {
          sourceKey = make_pair(ipToString(iph->saddr), htons(udph->uh_sport));
        }
        else if (iph->protocol == IPPROTO_TCP) 
        {
          sourceKey = make_pair(ipToString(iph->saddr), htons(tcph->th_sport));
        }
        
        if (lanToWan.count(sourceKey) == 0)
        {
          // Add to NAPT table
          printf("Adding to NAPT table\n");
          lanToWan[sourceKey] = dynamicPort;
          wanToLan[dynamicPort] = sourceKey;
          dynamicPort++;
        }
        
        // Perform translation using the NAPT table
        uint16_t translatedPort = lanToWan[sourceKey];
        printf("TRANSLATED PORT===: %d\n", translatedPort);
        if (iph->protocol == IPPROTO_UDP)
        {
          udph->uh_sport = htons(translatedPort);
          iph->saddr = inet_addr(wanIP.c_str());
        }
        else if (iph->protocol == IPPROTO_TCP)
        {
          tcph->th_sport = htons(translatedPort);
          iph->saddr = inet_addr(wanIP.c_str());
        }
        
      }
      else // is not in Client list: translate from WAN to LAN
      {
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
            printf("DROPPING PACKET\n");
            continue;
          }

          udph->uh_dport = htons(translatedIpAndPort.second);
          iph->daddr = inet_addr(translatedIpAndPort.first.c_str());

          
          myPsuedo.length = udph->uh_ulen;
          myPsuedo.destinationAddress = iph->daddr;
          myPsuedo.sourceAddress = iph->saddr;
          myPsuedo.reserved = 0;
          myPsuedo.protocol = iph->protocol;
        }
        else if (iph->protocol == IPPROTO_TCP)
        {
          destPort = tcph->th_dport;
          int destPortInt = ntohs(destPort);
          pair<string, int> translatedIpAndPort = wanToLan[destPortInt];

          tcph->th_dport = htons(translatedIpAndPort.second);
          iph->daddr = inet_addr(translatedIpAndPort.first.c_str());
          printf("translated destination port: **** %d\n", translatedIpAndPort.second);

          unsigned int tcpHeaderLength = tcph->th_off * 4;
          unsigned int dataLength = iph->tot_len - (iph->ihl * 4) - tcpHeaderLength;

          myPsuedo.length = tcpHeaderLength + dataLength;

          myPsuedo.sourceAddress = iph->saddr;
          myPsuedo.destinationAddress = iph->daddr;

          char sourceIP[INET_ADDRSTRLEN];
          inet_ntop(AF_INET, &(myPsuedo.sourceAddress), sourceIP, INET_ADDRSTRLEN);

          myPsuedo.reserved = 0;
          myPsuedo.protocol = iph->protocol;
        }
      }
      iph->check = 0;
      unsigned short new_checksum = compute_checksum((unsigned short *)buffer, iph->ihl * 4);
      iph->check = new_checksum;

      if (iph->protocol == IPPROTO_UDP) 
      {
        int payload_len = ntohs(iph->tot_len) - (iph->ihl * 4) - sizeof(struct udphdr);
        unsigned char *payload = buffer + iph->ihl * 4 + sizeof(struct udphdr);
        unsigned short checksum = udp_checksum(iph, udph, payload, payload_len);
        udph->uh_sum = checksum;
        printf("udp checksum: %d\n", checksum);
      }
      else if (iph->protocol == IPPROTO_TCP)
      {
        // unsigned int payloadLength = htons(iph->tot_len)- iph->ihl*4 - tcph->th_off*4;
        unsigned short myChecksum = tcp_checksum(iph, buffer + iph->ihl*4, htons(iph->tot_len) - iph->ihl*4);
        tcph->th_sum = myChecksum; 
      }

      if (address_to_socket.count(ipToString(iph->daddr)) > 0)
      {
        send(address_to_socket[ipToString(iph->daddr)], buffer, num_bytes, 0);
        std::cout << "Heree " << std::endl;
      }
      else
      {
        send(address_to_socket["0.0.0.0"], buffer, num_bytes, 0);
        std::cout << "Here INSTEAD " << std::endl;
      }
      std::cout << "SENTTTT" << std::endl;
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
  {
    perror("Failed to set SO_REUSEADDR option");
    // handle the error
  }

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
    // cout << "Connection accepted, spawning handler thread...\n" << endl;

    thread client_thread(handle_client, client_socket, wan);
    client_thread.detach();
  }
  return 0;
}

