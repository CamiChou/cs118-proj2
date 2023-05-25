#include "datagram_parser.h"
#include <sstream>
#include <stdexcept>
#include <iostream>

int hexToDecimal(std::string hex) {
    int decimal = 0;
    int base = 1;
    int len = hex.length();
    for (int i = len - 1; i >= 0; i--) {
        if (hex[i] >= '0' && hex[i] <= '9') {
            decimal += (hex[i] - 48) * base;
            base *= 16;
        }
        else if (hex[i] >= 'A' && hex[i] <= 'F') {
            decimal += (hex[i] - 55) * base;
            base *= 16;
        }
        else if (hex[i] >= 'a' && hex[i] <= 'f') {
            decimal += (hex[i] - 87) * base;
            base *= 16;
        }
    }
    return decimal;
}

std::string hexToIP(const std::string &hex) {
    std::stringstream ss;
    ss << std::dec;
    for (int i = 0; i < hex.length(); i += 2) {
        std::string byte = hex.substr(i, 2);
        int val = std::stoi(byte, nullptr, 16);
        if (i > 0) ss << ".";
        ss << val;
    }
    return ss.str();
}


// Parse IP header
IPHeader parseIPHeader(const std::string& hexString) {
    
    // printf("Parsing IP header...\n");
    IPHeader ipHeader;
    ipHeader.version = hexToDecimal(hexString.substr(0, 1));
    ipHeader.ihl = hexToDecimal(hexString.substr(1, 1));
    ipHeader.typeOfService = hexToDecimal(hexString.substr(2, 2));
    ipHeader.totalLength = hexToDecimal(hexString.substr(4, 4));
    ipHeader.identification = hexToDecimal(hexString.substr(8, 4));
    ipHeader.flagsAndFragmentOffset = hexString.substr(12, 4);
    ipHeader.ttl = hexToDecimal(hexString.substr(16, 2));
    ipHeader.protocol = hexToDecimal(hexString.substr(18, 2));
    ipHeader.headerChecksum = hexString.substr(20, 4);
    ipHeader.sourceIP = hexToIP(hexString.substr(24, 8));
    ipHeader.destinationIP = hexToIP(hexString.substr(32, 8));

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
    // fflush(stdout);

    return ipHeader;
}

// Parse UDP header
UDPHeader parseUDPHeader(const std::string& hexString) {
    printf("Parsing UDP header...\n");

    std::cout << "UDP Header Hexstring: " << hexString << std::endl;
    UDPHeader udpHeader;
    udpHeader.sourcePort = hexToDecimal(hexString.substr(0, 4));
    udpHeader.destinationPort = hexToDecimal(hexString.substr(4, 4));
    udpHeader.length = hexToDecimal(hexString.substr(8, 4));
    udpHeader.checksum = hexString.substr(12, 4);

    // std::cout << "Transport Protocol: UDP" << std::endl;
    std::cout << "UDP Source Port: " << udpHeader.sourcePort << std::endl;
    std::cout << "UDP Destination Port: " << udpHeader.destinationPort << std::endl;
    std::cout << "UDP Length: " << udpHeader.length << std::endl;
    std::cout << "UDP Checksum: " << udpHeader.checksum << std::endl;
    // std::cout << std::endl;
    // fflush(stdout);

    return udpHeader;
}

// Parse TCP header
TCPHeader parseTCPHeader(const std::string& hexString) {
    printf("Parsing TCP header...\n");

    TCPHeader tcpHeader;
    tcpHeader.sourcePort = hexToDecimal(hexString.substr(0, 4));
    tcpHeader.destinationPort = hexToDecimal(hexString.substr(4, 4));
    tcpHeader.sequenceNumber = hexToDecimal(hexString.substr(8, 8));
    tcpHeader.acknowledgmentNumber = hexToDecimal(hexString.substr(16, 8));
    tcpHeader.flags = hexString.substr(24, 4);
    tcpHeader.windowSize = hexToDecimal(hexString.substr(28, 4));
    tcpHeader.checksum = hexString.substr(32, 4);
    tcpHeader.urgentPointer = hexToDecimal(hexString.substr(36, 4));

    // std::cout << "TCP Source Port: " << tcpHeader.sourcePort << std::endl;
    // std::cout << "TCP Destination Port: " << tcpHeader.destinationPort << std::endl;
    // std::cout << "TCP Sequence Number: " << tcpHeader.sequenceNumber << std::endl;
    // std::cout << "TCP Acknowledgment Number: " << tcpHeader.acknowledgmentNumber << std::endl;
    // std::cout << "TCP Flags: " << tcpHeader.flags << std::endl;
    // std::cout << "TCP Window Size: " << tcpHeader.windowSize << std::endl;
    // std::cout << "TCP Checksum: " << tcpHeader.checksum << std::endl;
    // std::cout << "TCP Urgent Pointer: " << tcpHeader.urgentPointer << std::endl;  
    // std::cout << std::endl;
    // fflush(stdout);
    
    return tcpHeader;
}

Datagram parseIPDatagram(const std::string& hexString) {
    printf("Parsing datagram...\n");
    fflush(stdout);
    Datagram datagram;
    datagram.ipHeader = parseIPHeader(hexString);

    int headerLength = datagram.ipHeader.ihl * 8;
    std::string protocolData = hexString.substr(headerLength, datagram.ipHeader.totalLength - headerLength);

    if (datagram.ipHeader.protocol == 17) {
        datagram.transportHeader.header = parseUDPHeader(protocolData);
    } else if (datagram.ipHeader.protocol == 6) { // TCP
        datagram.transportHeader.header = parseTCPHeader(protocolData);
    } else {
        throw std::runtime_error("Unsupported protocol: " + std::to_string(datagram.ipHeader.protocol));
    }

    return datagram;
}
