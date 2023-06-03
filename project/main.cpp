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
#include "datagram_parser.h"

using namespace std;

const int BUFFER_SIZE = 2048;
map<pair<string, int>, int> lanToWan;
map<int, pair<string, int>> wanToLan;
map<string, int> address_to_socket;
vector<string> clientIPs;
int dynamicPort = 49152;

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

std::string calculateNetworkAddress(const std::string &ipAddress, const std::string &subnetMask)
{
  std::vector<int> ipParts;
  std::vector<int> maskParts;
  std::vector<int> networkParts;

  // Parse IP address
  std::string part;
  std::istringstream ipStream(ipAddress);
  while (getline(ipStream, part, '.'))
  {
    ipParts.push_back(std::stoi(part));
  }

  // Parse subnet mask
  std::istringstream maskStream(subnetMask);
  while (getline(maskStream, part, '.'))
  {
    maskParts.push_back(std::stoi(part));
  }

  // Calculate network address by performing bitwise AND
  for (size_t i = 0; i < ipParts.size(); ++i)
  {
    networkParts.push_back(ipParts[i] & maskParts[i]);
  }

  // Construct network address string
  std::string networkAddress;
  for (size_t i = 0; i < networkParts.size(); ++i)
  {
    networkAddress += std::to_string(networkParts[i]);
    if (i < networkParts.size() - 1)
    {
      networkAddress += ".";
    }
  }
  return networkAddress;
}

bool isSameSubnet(const std::string first, const std::string second, const std::string subnetMask)
{
  return calculateNetworkAddress(first, subnetMask) == calculateNetworkAddress(second, subnetMask);
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
  psh.length = htons(htons(iph->tot_len) - iph->ihl * 4);

  int psize = sizeof(struct PseudoHeader) + htons(iph->tot_len) - iph->ihl * 4;
  auto *pseudogram = new uint8_t[psize];

  memcpy(pseudogram, (char *)&psh, sizeof(struct PseudoHeader));
  memcpy(pseudogram + sizeof(struct PseudoHeader), payload, htons(iph->tot_len) - iph->ihl * 4);
  unsigned short sum = compute_checksum((unsigned short *)pseudogram, psize);

  delete[] pseudogram;
  return sum;
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
    if (num_bytes <= 0)
    {
      perror("Empty or Error with receiving packet data");
      return;
    }

    // Look at this later....
    std::stringstream ss;
    for (int i = 0; i < BUFFER_SIZE; ++i)
    {
      ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(buffer[i]);
    }

    std::string hexString = ss.str();
    // printf("Received packet: %s\n", hexString.c_str());
    // fflush(stdout);
    Datagram datagram = parseIPDatagram(hexString);

    // struct iphdr ipheader = DatagramToIphdr(datagram);
    // struct iphdr *iph = &ipheader;

    struct iphdr *iph = (struct iphdr *)buffer;

    // check the ip checksum
    unsigned short checksum = compute_checksum((unsigned short *)iph, iph->ihl * 4);
    if (checksum != 0)
    {
      cout << "Checksum failed. Dropping packet" << endl;
      continue;
    }
    if (iph->ttl <= 1)
    {
      cout << "Checksum failed. Dropping packet" << endl;
      continue;
    }
    iph->ttl -= 1;

    if (iph->protocol == IPPROTO_TCP)
    {
      struct tcphdr tcph;
      memcpy(&tcph, buffer + iph->ihl * 4, sizeof(struct tcphdr));
      unsigned short myChecksum = tcp_checksum(iph, buffer + iph->ihl * 4, htons(iph->tot_len) - iph->ihl * 4);
      if (myChecksum != 0)
      {
        cout << "Checksum failed. Dropping packet" << endl;
        continue;
      }
    }
    else if (iph->protocol == IPPROTO_UDP)
    {
      PseudoHeader myPsuedo;
      struct udphdr udph;
      memcpy(&udph, buffer + iph->ihl * 4, sizeof(struct udphdr));
      myPsuedo.length = udph.uh_ulen;
      inet_pton(AF_INET, datagram.ipHeader.sourceIP.c_str(), &(myPsuedo.sourceAddress));
      inet_pton(AF_INET, datagram.ipHeader.destinationIP.c_str(), &(myPsuedo.destinationAddress));
      char sourceIP[INET_ADDRSTRLEN];
      inet_ntop(AF_INET, &(myPsuedo.sourceAddress), sourceIP, INET_ADDRSTRLEN);
      myPsuedo.reserved = 0;
      myPsuedo.protocol = datagram.ipHeader.protocol;

      char *pseudoBuffer = new char[sizeof(PseudoHeader) + sizeof(struct udphdr)];
      memcpy(pseudoBuffer, &myPsuedo, sizeof(PseudoHeader));                     // insert pseudo header into buffer
      memcpy(pseudoBuffer + sizeof(PseudoHeader), &udph, sizeof(struct udphdr)); // insert udp header into buffer

      // Calculate the UDP checksum
      unsigned short myChecksum = compute_checksum(reinterpret_cast<unsigned short *>(pseudoBuffer), sizeof(PseudoHeader) + sizeof(struct udphdr));

      delete[] pseudoBuffer;
      if (myChecksum != 0)
      {
        cout << "Checksum failed. Dropping packet" << endl;
        continue;
      }
    }

    // forward packet locally
    std::string subnetMask24 = "255.255.255.0";

    // print source and destination ips
    std::cout << "Source IP: " << datagram.ipHeader.sourceIP << std::endl;
    std::cout << "Destination IP: " << datagram.ipHeader.destinationIP << std::endl;

    // check if the source and destination are on the same subnet
    bool sameSubnet = isSameSubnet(datagram.ipHeader.sourceIP, datagram.ipHeader.destinationIP, subnetMask24);
    std::cout << "Same subnet: = " << sameSubnet << std::endl;
    fflush(stdout);

    if (sameSubnet)
    {
      iph->check = 0;
      memcpy(buffer, iph, sizeof(struct iphdr));

      std::cout << "Buffer when check is 0: " << std::endl;
      printBufferAsHex(buffer, BUFFER_SIZE);

      unsigned short new_checksum = compute_checksum((unsigned short *)buffer, iph->ihl * 4);
      iph->check = new_checksum;

      std::cout << "CHECKSUM" << iph->check << std::endl;

      memcpy(buffer, iph, sizeof(struct iphdr));

      std::cout << "SAME SUBNET, GONNA SEND" << std::endl;

      std::cout << "Buffer: " << std::endl;
      printBufferAsHex(buffer, BUFFER_SIZE);

      send(address_to_socket[datagram.ipHeader.destinationIP], buffer, num_bytes, 0);
    }
    else
    {
      std::cout << "NOT SAME SUBNET, GOING NAT IT UP" << std::endl;
      // check if the source ip is in the vector of client ips
      bool isClient = std::find(clientIPs.begin(), clientIPs.end(), datagram.ipHeader.sourceIP) != clientIPs.end();
      struct udphdr udph;
      struct tcphdr *tcph;
      PseudoHeader myPsuedo;
      // Check the active type of the variant
      if (std::holds_alternative<UDPHeader>(datagram.transportHeader.header))
      {
        // Variant is UDPHeader
        udph = UDPHeaderToUdphdr(std::get<UDPHeader>(datagram.transportHeader.header));
        udph.uh_sum = 0;
        std::cout << "WOW its a udp header" << std::endl;
      }
      else if (std::holds_alternative<TCPHeader>(datagram.transportHeader.header))
      {
        tcph = (struct tcphdr *)(buffer + sizeof(struct iphdr));
        tcph->th_sum = 0;
        std::cout << "WOW its a tcp header" << std::endl;
      }

      if (isClient) // LAN to WAN
      {
        printf("LAN to WAN\n");
        pair<string, int> sourceKey;
        if (std::holds_alternative<UDPHeader>(datagram.transportHeader.header))
          sourceKey = make_pair(datagram.ipHeader.sourceIP, htons(udph.uh_sport));
        else if (std::holds_alternative<TCPHeader>(datagram.transportHeader.header))
          sourceKey = make_pair(datagram.ipHeader.sourceIP, htons(tcph->th_sport));

        for (const auto &[lanIp, wanPort] : lanToWan)
        {
          std::cout << "LAN IP: " << lanIp.first << "LAN Port:" << lanIp.second << std::endl;
          std::cout << "WAN Port: " << wanPort << std::endl;
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
        int translatedPort = lanToWan[sourceKey];
        printf("TRANSLATED PORT===: %d\n", translatedPort);
        if (std::holds_alternative<UDPHeader>(datagram.transportHeader.header))
        {
          printf("hereeeeeeeee (UDP)");
          datagram.ipHeader.sourceIP = wanIP;
          udph.uh_sport = htons(translatedPort);
          myPsuedo.length = udph.uh_ulen;

          // std::cout << "YAYAY THIS IS MY SOURCE PORT: " << ntohs(udph.uh_sport) << std::endl;
          // std::cout << "YAYAY THIS IS MY DEST PORT: " << ntohs(udph.uh_dport) << std::endl;

          inet_pton(AF_INET, datagram.ipHeader.sourceIP.c_str(), &(myPsuedo.sourceAddress));
          inet_pton(AF_INET, datagram.ipHeader.destinationIP.c_str(), &(myPsuedo.destinationAddress));

          char sourceIP[INET_ADDRSTRLEN];
          inet_ntop(AF_INET, &(myPsuedo.sourceAddress), sourceIP, INET_ADDRSTRLEN);

          // Print the IP address
          std::cout << "THIS IS MY SOURCE IP: " << sourceIP << std::endl;

          myPsuedo.reserved = 0;
          myPsuedo.protocol = datagram.ipHeader.protocol;

          std::cout << "Printing Psuedo: " << std::endl;
          unsigned char *bytes = reinterpret_cast<unsigned char *>(&myPsuedo);
          std::size_t size = sizeof(myPsuedo);

          for (std::size_t i = 0; i < size; ++i)
          {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(bytes[i]) << " ";
          }
          std::cout << std::endl;

          std::cout << "DONE" << std::endl;
        }
        else if (std::holds_alternative<TCPHeader>(datagram.transportHeader.header))
        {
          printf("hereeeeeeeee (TCP)");
          tcph->th_sport = htons(translatedPort);
          datagram.ipHeader.sourceIP = wanIP;
        }
      }
      else // is not in Client list: translate from WAN to LAN
      {
        u_int16_t destPort;

        if (std::holds_alternative<UDPHeader>(datagram.transportHeader.header))
        {
          destPort = udph.uh_dport;
          int destPortInt = ntohs(destPort);

          for (const auto &[wanPort, lanIp] : wanToLan)
          {
            std::cout << "WAN Port: " << wanPort << std::endl;
            std::cout << "LAN IP: " << lanIp.first << "LAN Port:" << lanIp.second << std::endl;
          }
          pair<string, int> translatedIpAndPort;
          // check if destPortInt is in the WAN to LAN map
          if (wanToLan.count(destPortInt) > 0)
          {
            translatedIpAndPort = wanToLan[destPortInt];
          }
          else
          {
            printf("DROPPING PACKET\n");
            continue;
          }

          printf("DEST PORT: %d\nTranslated to LAN IP: %s\nTranslated to LAN Port: %d\n", destPortInt, translatedIpAndPort.first.c_str(), translatedIpAndPort.second);
          fflush(stdout);

          udph.uh_dport = htons(translatedIpAndPort.second);
          datagram.ipHeader.destinationIP = translatedIpAndPort.first;
          myPsuedo.length = udph.uh_ulen;

          inet_pton(AF_INET, datagram.ipHeader.sourceIP.c_str(), &(myPsuedo.sourceAddress));
          inet_pton(AF_INET, datagram.ipHeader.destinationIP.c_str(), &(myPsuedo.destinationAddress));

          char sourceIP[INET_ADDRSTRLEN];
          inet_ntop(AF_INET, &(myPsuedo.sourceAddress), sourceIP, INET_ADDRSTRLEN);

          // Print the IP address
          // std::cout << "THIS IS MY SOURCE IP: " << sourceIP << std::endl;

          myPsuedo.reserved = 0;
          myPsuedo.protocol = datagram.ipHeader.protocol;

          // std::cout << "Printing Psuedo: " << std::endl;
        }
        else if (std::holds_alternative<TCPHeader>(datagram.transportHeader.header))
        {
          destPort = tcph->th_dport;
          int destPortInt = ntohs(destPort);
          pair<string, int> translatedIpAndPort = wanToLan[destPortInt];

          tcph->th_dport = htons(translatedIpAndPort.second);
          datagram.ipHeader.destinationIP = translatedIpAndPort.first;
          printf("translated destination port: **** %d\n", translatedIpAndPort.second);

          unsigned int tcpHeaderLength = tcph->th_off * 4;
          unsigned int dataLength = datagram.ipHeader.totalLength - (datagram.ipHeader.ihl * 4) - tcpHeaderLength;

          myPsuedo.length = tcpHeaderLength + dataLength;

          inet_pton(AF_INET, datagram.ipHeader.sourceIP.c_str(), &(myPsuedo.sourceAddress));
          inet_pton(AF_INET, datagram.ipHeader.destinationIP.c_str(), &(myPsuedo.destinationAddress));

          char sourceIP[INET_ADDRSTRLEN];
          inet_ntop(AF_INET, &(myPsuedo.sourceAddress), sourceIP, INET_ADDRSTRLEN);

          myPsuedo.reserved = 0;
          myPsuedo.protocol = datagram.ipHeader.protocol;
        }
      }

      // fflush(stdout);
      // std::cout << "THIS IS MY Dest IP: " << datagram.ipHeader.destinationIP << std::endl;
      // std::cout << "THIS IS MY Source IP: " << datagram.ipHeader.sourceIP << std::endl;
      iph->saddr = inet_addr(datagram.ipHeader.sourceIP.c_str());
      iph->daddr = inet_addr(datagram.ipHeader.destinationIP.c_str());
      iph->check = 0;
      memcpy(buffer, iph, sizeof(struct iphdr));
      unsigned short new_checksum = compute_checksum((unsigned short *)buffer, iph->ihl * 4);
      iph->check = new_checksum;
      memcpy(buffer, iph, sizeof(struct iphdr));

      if (std::holds_alternative<UDPHeader>(datagram.transportHeader.header))
      {
        // create pseudobuffer in order to checksum!
        char *pseudoBuffer = new char[sizeof(PseudoHeader) + sizeof(struct udphdr)];
        memcpy(pseudoBuffer, &myPsuedo, sizeof(PseudoHeader));                     // insert pseudo header into buffer
        memcpy(pseudoBuffer + sizeof(PseudoHeader), &udph, sizeof(struct udphdr)); // insert udp header into buffer

        // Calculate the UDP checksum
        unsigned short myChecksum = compute_checksum(reinterpret_cast<unsigned short *>(pseudoBuffer), sizeof(PseudoHeader) + sizeof(struct udphdr));
        udph.uh_sum = myChecksum;

        // std::cout << "PSEUDO CHECKSUM" << myChecksum << std::endl;
        memcpy(pseudoBuffer + sizeof(PseudoHeader), &udph, sizeof(struct udphdr)); // insert udp header into buffer

        // reconstruct buffer with ip header and new udp header and original data
        memcpy(buffer, iph, sizeof(struct iphdr));
        memcpy(buffer + sizeof(struct iphdr), &udph, sizeof(struct udphdr));
        memcpy(buffer + sizeof(struct iphdr) + sizeof(struct udphdr), buffer + (iph->ihl * 4), myPsuedo.length);

        // std::cout << "BUFFER NOW WITH THE PSEUDOBUFFER IS:::::" << std::endl;
        // printBufferAsHex(buffer, num_bytes);
        delete[] pseudoBuffer;
      }
      else if (std::holds_alternative<TCPHeader>(datagram.transportHeader.header))
      {
        unsigned int payloadLength = htons(iph->tot_len) - iph->ihl * 4 - tcph->th_off * 4;
        unsigned short myChecksum = tcp_checksum(iph, buffer + iph->ihl * 4, htons(iph->tot_len) - iph->ihl * 4);
        tcph->th_sum = myChecksum;
      }

      if (address_to_socket.count(datagram.ipHeader.destinationIP) > 0)
      {
        send(address_to_socket[datagram.ipHeader.destinationIP], buffer, num_bytes, 0);
        std::cout << "Heree " << std::endl;
        // return;
      }
      else
      {
        send(address_to_socket["0.0.0.0"], buffer, num_bytes, 0);
        std::cout << "Here INSTEAD " << std::endl;
        // return;
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

  string wanIP = parser.getIpConfig().wanIP;

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

    thread client_thread(handle_client, client_socket, wanIP);
    client_thread.detach();
  }
  return 0;
}
