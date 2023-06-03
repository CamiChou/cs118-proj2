#include "config_parser.h"

ConfigParser::ConfigParser(string in) : in(in) {}

IpConfig ConfigParser::getIpConfig() const
{
    return ipConfig;
}
NaptConfig ConfigParser::getNaptConfig() const
{
    return naptConfig;
}

std::vector<ACL> ConfigParser::getACL() const
{
    return aclConfig;
}

map<pair<string, int>, int> NaptConfig::getLtoW()
{
    return lanToWanMap;
}
map<int, pair<string, int>> NaptConfig::getWtoL()
{
    return wanToLanMap;
}
void ConfigParser::parse()
{
    parseIpConfig();
    parseNaptConfig();
    parseACLConfig();
}

void ConfigParser::parseIpConfig()
{
    /*
    Parse the IP configuration file.
    The first line is the router's LAN IP and the WAN IP.
    The line containing "0.0.0.0" is the WAN port.
    The following lines are the client's IPs.
    */
    istringstream iss(in); // create istringstream from in

    string line;
    if (getline(iss, line))
    {
        stringstream ss(line); // create stringstream from line
        string lanIp, wanIp;
        ss >> lanIp >> wanIp; // extract values from stringstream
        ipConfig.lanIP = lanIp;
        ipConfig.wanIP = wanIp;
    }

    // print the result of the getline fcn
    while (getline(iss, line))
    {
        // skip line if it contains "0.0.0.0"
        if (line == "0.0.0.0")
            continue;
        // break out on new lines
        if (line == "")
            break;
        ipConfig.clientIps.push_back(line);
    }

    // move the ConfigParser.in pointer to the next line
    in = iss.str().substr(iss.tellg());
}
void ConfigParser::parseNaptConfig()
{
    /*
    Parse the NAPT configuration file.
    Each line is a mapping from a LAN IP and port to a WAN port.
    */
    istringstream iss(in); // create istringstream from in

    string line;
    while (getline(iss, line))
    {
        // break out on new lines
        if (line.empty())
            break;

        stringstream ss(line);
        string lanIp;
        int lanPort, wanPort;
        ss >> lanIp >> lanPort >> wanPort;

        naptConfig.lanToWanMap[make_pair(lanIp, lanPort)] = wanPort;
        naptConfig.wanToLanMap[wanPort] = make_pair(lanIp, lanPort);
    }
    in = iss.str().substr(iss.tellg());
}

void ConfigParser::parseACLConfig()
{
    istringstream iss(in); // create istringstream from in
    string line;

    while (getline(iss, line))
    {
        if (line.empty())
            break;
        // break out on new lines

        ACL acl;
        stringstream ss(line);
        string sourceIp, sourcePort, destIp, destPort;
        ss >> sourceIp >> sourcePort >> destIp >> destPort;

        // Parse source IP and subnet
        size_t slashIndex = sourceIp.find('/');

        acl.sourceIP = sourceIp.substr(0, slashIndex);
        acl.subnet = stoi(sourceIp.substr(slashIndex + 1));

        // Parse source port range
        size_t dashIndex = sourcePort.find('-');
        if (dashIndex != string::npos)
        {
            acl.sourcePortStart = stoi(sourcePort.substr(0, dashIndex));
            acl.sourcePortEnd = stoi(sourcePort.substr(dashIndex + 1));
        }

        // Parse destination IP and subnet
        slashIndex = destIp.find('/');
        if (slashIndex != string::npos)
        {
            acl.destIP = destIp.substr(0, slashIndex);
            // Subnet is not specified in the given format, so you may need to decide how to handle it
        }

        // Parse destination port range
        dashIndex = destPort.find('-');
        if (dashIndex != string::npos)
        {
            acl.destPortStart = stoi(destPort.substr(0, dashIndex));
            acl.destPortEnd = stoi(destPort.substr(dashIndex + 1));
        }

        aclConfig.push_back(acl);
    }
}

void ConfigParser::print() const
{
    cout << "PRINTING CONFIGURATION" << endl;
    cout << "Router IP: " << ipConfig.lanIP << endl;
    cout << "Router WAN IP: " << ipConfig.wanIP << endl;
    for (const auto &clientIp : ipConfig.clientIps)
    {
        cout << "Client IP: " << clientIp << endl;
    }

    for (const auto &[lanIp, wanPort] : naptConfig.lanToWanMap)
    {
        cout << "LAN IP: " << lanIp.first << "LAN Port:" << lanIp.second << endl;
        cout << "WAN Port: " << wanPort << endl;
    }

    cout << "PRINTING ACL" << endl;
    for (const ACL &acl : aclConfig)
    {
        cout << "Source IP: " << acl.sourceIP << endl;
        cout << "SUBNET: " << acl.subnet << endl;
        cout << "Source Port Start: " << acl.sourcePortStart << endl;
        cout << "Source Port End: " << acl.sourcePortEnd << endl;
        cout << "Destination IP: " << acl.destIP << endl;
        cout << "Destination Port Start: " << acl.destPortStart << endl;
        cout << "Destination Port End: " << acl.destPortEnd << endl;
        cout << endl;
    }

    cout << "FINI";
    cout << endl;
}