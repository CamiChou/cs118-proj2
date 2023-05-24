#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <sstream>
using namespace std;

struct IpConfig {
    string lanIP; // router's LAN IP
    string wanIP; // router's WAN IP
    vector<string> clientIps; // client IPs
};
struct NaptConfig {
    map<pair<string, int>, int> lanToWanMap; // maps from (LAN IP, LAN port) to WAN port
};

class ConfigParser {
private:
    string in;
    IpConfig ipConfig;
    NaptConfig naptConfig;

public:
    ConfigParser(string in);
    IpConfig getIpConfig() const;
    NaptConfig getNaptConfig() const;
    void parse();

private:
    void parseIpConfig();
    void parseNaptConfig();
};