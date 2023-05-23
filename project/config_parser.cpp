#include "config_parser.h"

ConfigParser::ConfigParser(istream& in) : in(in) {}

IpConfig ConfigParser::getIpConfig() const{
    return ipConfig;
}
NaptConfig ConfigParser::getNaptConfig() const{
    return naptConfig;
}
void ConfigParser::parse(){
    parseIpConfig();
    parseNaptConfig();
}

void ConfigParser::parseIpConfig(){
    /*
    Parse the IP configuration file.
    The first line is the router's LAN IP and the WAN IP.
    The following lines are the client's IPs.
    */
    string line;
    getline(in, line);
    stringstream ss(line);
    string lanIp, wanIp;
    ss >> lanIp >> wanIp;
    ipConfig.lanIP = lanIp;
    ipConfig.wanIP = wanIp;

    // skip a line
    getline(in, line); 

    while(getline(in, line)) {
        // break out on new lines
        if(line == "") break;
        ipConfig.clientIps.push_back(line);
    }
}

void ConfigParser::parseNaptConfig(){
    /*
    Parse the NAPT configuration file.
    Each line is a mapping from a LAN IP and port to a WAN port.
    */
    string line;
    while(getline(in, line)) {
        // break out on new lines
        if(line.empty()) break;

        stringstream ss(line);
        string lanIp;
        int lanPort, wanPort;
        ss >> lanIp >> lanPort >> wanPort;

        naptConfig.lanToWanMap[make_pair(lanIp, lanPort)] = wanPort;
    }
}
