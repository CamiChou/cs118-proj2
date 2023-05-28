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

    std::stringstream ss;
    for (int i = 0; i < BUFFER_SIZE; ++i)
    {
      ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(buffer[i]);
    }
    std::string hexString = ss.str();
    Datagram datagram = parseIPDatagram(hexString);

    struct iphdr ipheader = DatagramToIphdr(datagram);
    struct iphdr *iph = &ipheader;

    iph->ttl -= 1;
    if (iph->ttl <= 0)
    {
      cout << "TTL expired. Dropping packet" << endl;
      return;
    }

    iph->check = 0;
    memcpy(buffer, iph, sizeof(struct iphdr));
    unsigned short new_checksum = compute_checksum((unsigned short *)buffer, iph->ihl * 4);
    iph->check = new_checksum;
    memcpy(buffer, iph, sizeof(struct iphdr));

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
      std::cout << "SAME SUBNET, GONNA SEND" << std::endl;
      send(address_to_socket[datagram.ipHeader.destinationIP], buffer, num_bytes, 0);
    }

    else
    {
      std::cout << "NOT SAME SUBNET, GOING NAT IT UP" << std::endl;

      int destport;
      int sourceport;
      // Check the active type of the variant
      if (std::holds_alternative<UDPHeader>(datagram.transportHeader.header))
      {
        // Variant is UDPHeader
        UDPHeader &udpHeader = std::get<UDPHeader>(datagram.transportHeader.header);
        destport = udpHeader.destinationPort;
        sourceport = udpHeader.sourcePort;
        std::cout << "WOW its a udp header" << std::endl;
      }
      else if (std::holds_alternative<TCPHeader>(datagram.transportHeader.header))
      {
        // Variant is TCPHeader
        TCPHeader &tcpHeader = std::get<TCPHeader>(datagram.transportHeader.header);
        destport = tcpHeader.destinationPort;
        sourceport = tcpHeader.sourcePort;
        std::cout << "WOW its a tcp header" << std::endl;
      }
      std::cout << "THIS IS MY DESTPORT: " << destport << std::endl;
      std::cout << "THIS IS MY SOURCEPORT: " << sourceport << std::endl;
      std::cout << "THIS IS MY DEST IP: " << datagram.ipHeader.destinationIP << std::endl;
      std::cout << "THIS IS MY SOURCE IP: " << datagram.ipHeader.sourceIP << std::endl;

      pair<string, int> destKey = make_pair(datagram.ipHeader.destinationIP, destport);
      pair<string, int> sourceKey = make_pair(datagram.ipHeader.sourceIP, sourceport);

      for (const auto &[lanIp, wanPort] : naptTable)
      {
        std::cout << "LAN IP: " << lanIp.first << "LAN Port:" << lanIp.second << std::endl;
        std::cout << "WAN Port: " << wanPort << std::endl;
      }

      // Check if the entry exists in the NAPT table
      if (naptTable.count(sourceKey) > 0)
      {
        // Perform translation using the NAPT table
        int translatedPort = naptTable[sourceKey];
        if (std::holds_alternative<UDPHeader>(datagram.transportHeader.header))
        {
          // Variant is UDPHeader
          UDPHeader &udpHeader = std::get<UDPHeader>(datagram.transportHeader.header);
          // udpHeader.destinationPort = translatedPort; // make equal WAN port instead
          udpHeader.sourcePort = translatedPort;
          std::cout << "YAYAY THIS IS MY SOURCE PORT: " << udpHeader.sourcePort << std::endl;
          datagram.ipHeader.sourceIP = wanIP;
          std::cout << "YAYAY THIS IS MY DEST PORT: " << udpHeader.destinationPort << std::endl;

          // datagram.ipHeader.destinationIP = wanIP;
        }
        else if (std::holds_alternative<TCPHeader>(datagram.transportHeader.header))
        {
          // Variant is TCPHeader
          TCPHeader &tcpHeader = std::get<TCPHeader>(datagram.transportHeader.header);
          // tcpHeader.destinationPort = translatedPort;
          tcpHeader.sourcePort = translatedPort;
          std::cout << "YAYAY THIS IS MY SOURCE PORT: " << tcpHeader.sourcePort << std::endl;
          datagram.ipHeader.sourceIP = wanIP;
          std::cout << "YAYAY THIS IS MY DEST PORT: " << tcpHeader.destinationPort << std::endl;
          // datagram.ipHeader.destinationIP = wanIP;
        }

        fflush(stdout);
        std::cout << "THIS IS MY Dest IP: " << datagram.ipHeader.destinationIP << std::endl;
        std::cout << "THIS IS MY Source IP: " << datagram.ipHeader.sourceIP << std::endl;

        send(address_to_socket[datagram.ipHeader.destinationIP], buffer, num_bytes, 0);
        std::cout << "SENTTTT" << std::endl;
      }

      std::cout << "Ya so... Not there" << std::endl;
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
  // parser.print();

  clientIPs.push_back("0.0.0.0");
  for (auto ip : parser.getIpConfig().clientIps)
  {
    clientIPs.push_back(ip);
  }
  naptTable = parser.getNaptConfig().convertToMap();
  // NEED TO MAKE SURE THAT THE NAPT TABLE IS VALID

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
    // cout << "Server waiting for connection...\n" << endl;

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