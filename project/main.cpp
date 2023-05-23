#include <iostream>
#include <sstream>
#include <string>
#include <fstream>
using namespace std;
#include "config_parser.h"

int main() {
  std::istringstream configInput(
        "192.168.1.1 98.149.235.132\n"
        "0.0.0.0\n"
        "192.168.1.100\n"
        "192.168.1.200\n"
        "\n"
        "192.168.1.100 8080 8080\n"
        "192.168.1.200 9000 443\n"
        "\n"
    );

  ConfigParser parser(configInput);
  parser.parse();

  auto ipConfig = parser.getIpConfig();
  auto naptConfig = parser.getNaptConfig();

  // print ipConfig
  std::cout << "Router IP: " << ipConfig.lanIP << std::endl;
  std::cout << "Router WAN IP: " << ipConfig.wanIP << std::endl;
  for (const auto& clientIp : ipConfig.clientIps) {
      std::cout << "Client IP: " << clientIp << std::endl;
  }

  // print naptConfig
  for (const auto& [lanIp, entry] : naptConfig.lanToWanMap) {
      std::cout << "NAPT Entry: " << lanIp.first << ", " << lanIp.second << ", " << entry << std::endl;
  }
}


// int main() {
//   std::string szLine;

//   // First line is the router's LAN IP and the WAN IP
//   std::getline(std::cin, szLine);
//   size_t dwPos = szLine.find(' ');
//   auto szLanIp = szLine.substr(0, dwPos);
//   auto szWanIp = szLine.substr(dwPos + 1);

//   std::cout << "Server's LAN IP: " << szLanIp << std::endl
//             << "Server's WAN IP: " << szWanIp << std::endl;

//   // TODO: Modify/Add/Delete files under the project folder.

//   return 0;
// }

