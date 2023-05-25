#ifndef DATAGRAM_PARSER_H
#define DATAGRAM_PARSER_H

#include <string>
#include <variant>

struct UDPHeader {
    UDPHeader() : sourcePort(0), destinationPort(0), length(0), checksum("") {}

    unsigned int sourcePort;
    unsigned int destinationPort;
    unsigned int length;
    std::string checksum;
};

struct TCPHeader {
    TCPHeader() : sourcePort(0), destinationPort(0), sequenceNumber(0), acknowledgmentNumber(0), flags(""), windowSize(0), checksum(""), urgentPointer(0) {}

    unsigned int sourcePort;
    unsigned int destinationPort;
    unsigned int sequenceNumber;
    unsigned int acknowledgmentNumber;
    std::string flags;
    unsigned int windowSize;
    std::string checksum;
    unsigned int urgentPointer;
};

struct IPHeader {
    IPHeader() : version(0), ihl(0), typeOfService(0), totalLength(0), identification(0), 
    flagsAndFragmentOffset(""), ttl(0), protocol(0), headerChecksum(""), sourceIP(""), destinationIP("") {} 
    
    unsigned int version;
    unsigned int ihl;
    unsigned int typeOfService;
    unsigned int totalLength;
    unsigned int identification;
    std::string flagsAndFragmentOffset;
    unsigned int ttl;
    unsigned int protocol;
    std::string headerChecksum;
    std::string sourceIP;
    std::string destinationIP;
};

struct TransportHeader {
    std::variant<UDPHeader, TCPHeader> header;
};

struct Datagram {
    IPHeader ipHeader;
    TransportHeader transportHeader;
};

Datagram parseIPDatagram(const std::string& hexString);

#endif //DATAGRAM_PARSER_H
