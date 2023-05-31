#include "config_parser.h"

ConfigParser::ConfigParser(string in) : in(in) {}

IpConfig ConfigParser::getIpConfig() const{
    return ipConfig;
}
NaptConfig ConfigParser::getNaptConfig() const{
    return naptConfig;
}
map<pair<string, int>, int> NaptConfig::convertToMap() {
    return lanToWanMap;
}
void ConfigParser::parse(){
    parseIpConfig();
    parseNaptConfig();
}

void ConfigParser::parseIpConfig(){
    /*
    Parse the IP configuration file.
    The first line is the router's LAN IP and the WAN IP.
    The line containing "0.0.0.0" is the WAN port.
    The following lines are the client's IPs.
    */
    istringstream iss(in);  // create istringstream from in

    string line;
    if (getline(iss, line)) {
        stringstream ss(line);  // create stringstream from line
        string lanIp, wanIp;
        ss >> lanIp >> wanIp;  // extract values from stringstream
        ipConfig.lanIP = lanIp;
        ipConfig.wanIP = wanIp;
    }

    // print the result of the getline fcn
    while(getline(iss, line)) {
        // skip line if it contains "0.0.0.0"
        if(line == "0.0.0.0") continue;
        // break out on new lines
        if(line == "") break;
        ipConfig.clientIps.push_back(line);
    }

    // move the ConfigParser.in pointer to the next line
    in = iss.str().substr(iss.tellg());
}
void ConfigParser::parseNaptConfig(){
    /*
    Parse the NAPT configuration file.
    Each line is a mapping from a LAN IP and port to a WAN port.
    */
    istringstream iss(in);  // create istringstream from in

    string line;
    while(getline(iss, line)) {
        // break out on new lines
        if(line.empty()) break;

        stringstream ss(line);
        string lanIp;
        int lanPort, wanPort;
        ss >> lanIp >> lanPort >> wanPort;

        naptConfig.lanToWanMap[make_pair(lanIp, lanPort)] = wanPort;
    }
}

void ConfigParser::print() const {
    cout << "PRINTING CONFIGURATION" << endl;
    cout << "Router IP: " << ipConfig.lanIP << endl;
    cout << "Router WAN IP: " << ipConfig.wanIP << endl;
    for (const auto& clientIp : ipConfig.clientIps) {
        cout << "Client IP: " << clientIp << endl;
    }

    for (const auto& [lanIp, wanPort] : naptConfig.lanToWanMap) {
        cout << "LAN IP: " << lanIp.first << "LAN Port:" << lanIp.second << endl;
        cout << "WAN Port: " << wanPort << endl;
    }
    cout << endl;
}