#ifndef DATAGRAM_PARSER_H
#define DATAGRAM_PARSER_H

#include <string>
#include <variant>

struct UDPHeader
{
    UDPHeader() : sourcePort(0), destinationPort(0), length(0), checksum(0) {}

    unsigned int sourcePort;
    unsigned int destinationPort;
    unsigned int length;
    uint16_t checksum;
    void recomputeChecksum();
};

struct TCPHeader
{
    TCPHeader() : sourcePort(0), destinationPort(0), sequenceNumber(0), acknowledgmentNumber(0), flags(0), windowSize(0), checksum(0), urgentPointer(0) {}

    unsigned int sourcePort;
    unsigned int destinationPort;
    unsigned int sequenceNumber;
    unsigned int acknowledgmentNumber;
    unsigned short dataOffset;
    unsigned short reserved;
    unsigned short flags;
    unsigned int windowSize;
    uint16_t checksum;
    unsigned int urgentPointer;
    void recomputeChecksum();
};

struct IPHeader
{
    IPHeader() : version(0), ihl(0), typeOfService(0), totalLength(0), identification(0),
                 flagsAndFragmentOffset(0), ttl(0), protocol(0), headerChecksum(0), sourceIP(""), destinationIP("") {}

    unsigned int version;
    unsigned int ihl;
    unsigned int typeOfService;
    unsigned int totalLength;
    unsigned int identification;
    unsigned int flagsAndFragmentOffset;
    unsigned int ttl;
    unsigned int protocol;
    uint16_t headerChecksum;
    std::string sourceIP;
    std::string destinationIP;
    unsigned int optionsAndPadding;
};

struct TransportHeader
{
    std::variant<UDPHeader, TCPHeader> header;
};

struct Datagram
{
    IPHeader ipHeader;
    TransportHeader transportHeader;
};

Datagram parseIPDatagram(const std::string &hexString);
struct iphdr DatagramToIphdr(const Datagram &datagram);
struct udphdr UDPHeaderToUdphdr(const UDPHeader &udpHeader);
struct tcphdr TCPHeaderToTcphdr(const TCPHeader &tcpHeader);

#endif // DATAGRAM_PARSER_H
