#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <sstream>
using namespace std;

struct ACL
{
    string sourceIP;
    int subnet;
    int sourcePortStart;
    int sourcePortEnd;
    string destIP;
    int destPortStart;
    int destPortEnd;
};

struct IpConfig
{
    string lanIP;             // router's LAN IP
    string wanIP;             // router's WAN IP
    vector<string> clientIps; // client IPs
};

struct NaptConfig
{
    map<pair<string, int>, int> lanToWanMap; // maps from (LAN IP, LAN port) to WAN port
    map<pair<string, int>, int> getLtoW();   // maps from (WAN IP, WAN port) to LAN port
    map<int, pair<string, int>> wanToLanMap;
    map<int, pair<string, int>> getWtoL();
};

class ConfigParser
{
private:
    string in;
    IpConfig ipConfig;
    NaptConfig naptConfig;
    std::vector<ACL> aclConfig;

public:
    ConfigParser(string in);
    IpConfig getIpConfig() const;
    NaptConfig getNaptConfig() const;
    std::vector<ACL> getACL() const;
    void parse();
    void print() const;

private:
    void parseIpConfig();
    void parseNaptConfig();
    void parseACLConfig();
};