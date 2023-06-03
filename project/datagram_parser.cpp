#include "datagram_parser.h"
#include <sstream>
#include <stdexcept>
#include <iostream>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

int hexToDecimal(std::string hex)
{
    int decimal = 0;
    int base = 1;
    int len = hex.length();
    for (int i = len - 1; i >= 0; i--)
    {
        if (hex[i] >= '0' && hex[i] <= '9')
        {
            decimal += (hex[i] - 48) * base;
            base *= 16;
        }
        else if (hex[i] >= 'A' && hex[i] <= 'F')
        {
            decimal += (hex[i] - 55) * base;
            base *= 16;
        }
        else if (hex[i] >= 'a' && hex[i] <= 'f')
        {
            decimal += (hex[i] - 87) * base;
            base *= 16;
        }
    }
    return decimal;
}

std::string hexToIP(const std::string &hex)
{
    std::stringstream ss;
    ss << std::dec;
    for (size_t i = 0; i < hex.length(); i += 2)
    {
        std::string byte = hex.substr(i, 2);
        int val = std::stoi(byte, nullptr, 16);
        if (i > 0)
            ss << ".";
        ss << val;
    }
    return ss.str();
}

// Parse IP header
IPHeader parseIPHeader(const std::string &hexString)
{

    // printf("Parsing IP header...\n");
    // fflush(stdout);
    IPHeader ipHeader;
    ipHeader.version = hexToDecimal(hexString.substr(0, 1));
    ipHeader.ihl = hexToDecimal(hexString.substr(1, 1));
    ipHeader.typeOfService = hexToDecimal(hexString.substr(2, 2));
    ipHeader.totalLength = hexToDecimal(hexString.substr(4, 4));
    ipHeader.identification = hexToDecimal(hexString.substr(8, 4));
    ipHeader.flagsAndFragmentOffset = hexToDecimal(hexString.substr(12, 4));
    ipHeader.ttl = hexToDecimal(hexString.substr(16, 2));
    ipHeader.protocol = hexToDecimal(hexString.substr(18, 2));
    ipHeader.headerChecksum = hexToDecimal(hexString.substr(20, 4));
    ipHeader.sourceIP = hexToIP(hexString.substr(24, 8));
    ipHeader.destinationIP = hexToIP(hexString.substr(32, 8));
    ipHeader.optionsAndPadding = hexToDecimal(hexString.substr(40, 8));

    // std::cout << "IP Version: " << ipHeader.version << std::endl;
    // std::cout << "IP IHL: " << ipHeader.ihl << std::endl;
    // std::cout << "IP Type of Service: " << ipHeader.typeOfService << std::endl;
    // std::cout << "IP Total Length: " << ipHeader.totalLength << std::endl;
    // std::cout << "IP Identification: " << ipHeader.identification << std::endl;
    // std::cout << "IP Flags and Fragment Offset: " << ipHeader.flagsAndFragmentOffset << std::endl;
    // std::cout << "IP TTL: " << ipHeader.ttl << std::endl;
    // std::cout << "IP Protocol: " << ipHeader.protocol << std::endl;
    // std::cout << "IP Header Checksum: " << ipHeader.headerChecksum << std::endl;
    // std::cout << "IP Source Address: " << ipHeader.sourceIP << std::endl;
    // std::cout << "IP Destination Address: " << ipHeader.destinationIP << std::endl;
    // std::cout << std::endl;
    fflush(stdout);

    return ipHeader;
}

// Parse UDP header
UDPHeader parseUDPHeader(const std::string &hexString)
{
    // printf("Parsing UDP header...\n");
    // fflush(stdout);

    UDPHeader udpHeader;
    udpHeader.sourcePort = hexToDecimal(hexString.substr(0, 4));
    udpHeader.destinationPort = hexToDecimal(hexString.substr(4, 4));
    udpHeader.length = hexToDecimal(hexString.substr(8, 4));
    udpHeader.checksum = hexToDecimal(hexString.substr(12, 4));

    // std::cout << "UDP Header Hexstring: " << hexString << std::endl;
    // std::cout << "Transport Protocol: UDP" << std::endl;
    // std::cout << "UDP Source Port: " << udpHeader.sourcePort << std::endl;
    // std::cout << "UDP Destination Port: " << udpHeader.destinationPort << std::endl;
    // std::cout << "UDP Length: " << udpHeader.length << std::endl;
    // std::cout << "UDP Checksum: " << udpHeader.checksum << std::endl;
    // std::cout << std::endl;
    // fflush(stdout);

    return udpHeader;
}

// Parse TCP header
TCPHeader parseTCPHeader(const std::string &hexString)
{
    // printf("Parsing TCP header...\n");
    // fflush(stdout);

    // print the hex string
    std::cout << "TCP Header Hexstring: " << hexString << std::endl;

    TCPHeader tcpHeader;
    tcpHeader.sourcePort = hexToDecimal(hexString.substr(0, 4));
    tcpHeader.destinationPort = hexToDecimal(hexString.substr(4, 4));
    tcpHeader.sequenceNumber = hexToDecimal(hexString.substr(8, 8));
    tcpHeader.acknowledgmentNumber = hexToDecimal(hexString.substr(16, 8));
    tcpHeader.dataOffset = std::stoi(hexString.substr(24, 1), nullptr, 16);
    tcpHeader.reserved = hexToDecimal(hexString.substr(25, 1));
    tcpHeader.flags = hexToDecimal(hexString.substr(26, 2));
    tcpHeader.windowSize = hexToDecimal(hexString.substr(28, 4));
    tcpHeader.checksum = hexToDecimal(hexString.substr(32, 4));
    tcpHeader.urgentPointer = hexToDecimal(hexString.substr(36, 4));

    // printf("Source Port: %d\n", tcpHeader.sourcePort);
    // printf("Destination Port: %d\n", tcpHeader.destinationPort);
    // printf("Sequence Number: %d\n", tcpHeader.sequenceNumber);
    // printf("Acknowledgment Number: %d\n", tcpHeader.acknowledgmentNumber);
    // print dataOffset as raw bytes
    // printf("Flags: %d\n", tcpHeader.flags);
    // printf("Window Size: %d\n", tcpHeader.windowSize);
    // printf("Checksum: %d\n", tcpHeader.checksum);
    // printf("Urgent Pointer: %d\n", tcpHeader.urgentPointer);
    fflush(stdout);

    return tcpHeader;
}


Datagram parseIPDatagram(const std::string &hexString)
{
    // printf("Parsing datagram...\n");
    fflush(stdout);
    Datagram datagram;
    datagram.ipHeader = parseIPHeader(hexString);

    if (datagram.ipHeader.protocol == 17)
    {
        int headerLength = datagram.ipHeader.ihl * 8;
        std::string protocolData = hexString.substr(headerLength, datagram.ipHeader.totalLength - headerLength);
        datagram.transportHeader.header = parseUDPHeader(protocolData);
    }
    else if (datagram.ipHeader.protocol == 6)
    { // TCP
        int IPheaderLength = datagram.ipHeader.ihl * 8;
        int TCPandBodyLength = datagram.ipHeader.totalLength * 2 - IPheaderLength;

        std::string protocolData = hexString.substr(IPheaderLength, TCPandBodyLength);
        datagram.transportHeader.header = parseTCPHeader(protocolData);
    }
    else
    {
        throw std::runtime_error("Unsupported protocol: " + std::to_string(datagram.ipHeader.protocol));
    }

    // printf("Parsed datagram.\n");
    fflush(stdout);
    return datagram;
}

struct iphdr DatagramToIphdr(const Datagram &datagram)
{
    struct iphdr ip_header;

    // Convert version and ihl into a single byte
    ip_header.version = datagram.ipHeader.version;
    ip_header.ihl = datagram.ipHeader.ihl;
    ip_header.tos = datagram.ipHeader.typeOfService;
    ip_header.tot_len = ntohs(datagram.ipHeader.totalLength);
    ip_header.id = htons(datagram.ipHeader.identification);
    ip_header.frag_off = htons(datagram.ipHeader.flagsAndFragmentOffset);
    ip_header.ttl = datagram.ipHeader.ttl;
    ip_header.protocol = datagram.ipHeader.protocol;
    ip_header.check = htons(datagram.ipHeader.headerChecksum);

    // Convert source and destination IP addresses from dotted-decimal format to network byte order
    inet_pton(AF_INET, datagram.ipHeader.sourceIP.c_str(), &(ip_header.saddr));
    inet_pton(AF_INET, datagram.ipHeader.destinationIP.c_str(), &(ip_header.daddr));

    return ip_header;
}

struct udphdr UDPHeaderToUdphdr(const UDPHeader &udpHeader)
{
    struct udphdr udph;

    udph.uh_dport = htons(udpHeader.destinationPort);
    udph.uh_sport = htons(udpHeader.sourcePort);
    udph.uh_ulen = htons(udpHeader.length);
    udph.uh_sum = udpHeader.checksum;

    return udph;
}

struct tcphdr TCPHeaderToTcphdr(const TCPHeader& tcpHeader)
{
    struct tcphdr tcph;

    tcph.th_sport = htons(tcpHeader.sourcePort);
    tcph.th_dport = htons(tcpHeader.destinationPort);
    tcph.th_seq = htonl(tcpHeader.sequenceNumber);
    tcph.th_ack = htonl(tcpHeader.acknowledgmentNumber);
    tcph.th_off = tcpHeader.dataOffset;
    // std::cout << "TCP header offset is:" << tcpHeader.dataOffset << std::endl;
    // std::cout << "TCP Header data offset:  ??????" << tcph.th_off << std::endl;
    // printf("TCP Header length (in 32-bit words): %u\n", tcph.th_off);
    // printf("TCP Header length (in bytes): %u\n", tcph.th_off * 4);
    tcph.th_flags = tcpHeader.flags;
    tcph.th_win = htons(tcpHeader.windowSize);
    tcph.th_sum = tcpHeader.checksum;
    tcph.th_urp = htons(tcpHeader.urgentPointer);

    return tcph;
}
