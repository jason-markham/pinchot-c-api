/**
 * Copyright (c) JoeScan Inc. All Rights Reserved.
 *
 * Licensed under the BSD 3 Clause License. See LICENSE.txt in the project
 * root for license information.
 */

#include <cstdlib>
#include <cstring>
#include <sstream>
#include <stdexcept>

#include "NetworkIncludes.hpp"
#include "NetworkInterface.hpp"
#include "NetworkTypes.hpp"

#ifdef _Win32
#include <ws2tcpip.h>
#include <iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")
#else //linux or macos
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <sys/types.h>
#include <unistd.h>
#endif

using namespace joescan;

void NetworkInterface::InitSystem(void)
{
#ifdef _Win32
  WSADATA wsa;
  int result = WSAStartup(MAKEWORD(2, 2), &wsa);
  if (result != 0) {
    std::stringstream error_msg;
    error_msg << "Failed to initialize winsock: " << result;
    throw std::runtime_error(error_msg.str());
  }
#endif
}

void NetworkInterface::FreeSystem(void)
{
#ifdef _Win32
  WSACleanup();
#endif
}

net_iface NetworkInterface::InitBroadcastSocket(uint32_t ip, uint16_t port)
{
  net_iface iface;
  SOCKET sockfd = INVALID_SOCKET;
  int r = 0;

  iface = InitUDPSocket(ip, port);
  sockfd = iface.sockfd;

#if _WIN32
  char bcast_en = 1;
#else
  int bcast_en = 1;
#endif
  r = setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &bcast_en, sizeof(bcast_en));
  if (SOCKET_ERROR == r) {
    CloseSocket(sockfd);
    throw std::runtime_error("faild to enable socket broadcast");
  }

  return iface;
}

net_iface NetworkInterface::InitRecvSocket(uint32_t ip, uint16_t port)
{
  net_iface iface;
  SOCKET sockfd = INVALID_SOCKET;
  int r = 0;

  iface = InitUDPSocket(ip, port);
  sockfd = iface.sockfd;

  {
    int m = 0;
    int n = kRecvSocketBufferSize;
#ifdef _Win32
    int sz = sizeof(n);
    r = setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (char *)&n, sz);
    if (SOCKET_ERROR != r) {
      r = getsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (char *)&m, &sz);
    }

#else
    socklen_t sz = sizeof(n);
    r = setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (void *) &n, sz);
    if (SOCKET_ERROR != r) {
        r = getsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (void *) &m, &sz);
    }
#endif
  }

  return iface;
}

net_iface NetworkInterface::InitSendSocket(uint32_t ip, uint16_t port)
{
  net_iface iface;

  iface = InitUDPSocket(ip, port);

  return iface;
}

void NetworkInterface::CloseSocket(SOCKET sockfd)
{
#ifdef _Win32
  int lastError = WSAGetLastError();
  closesocket(sockfd);
  WSASetLastError(lastError);
#else
  close(sockfd);
#endif
}

std::vector<uint32_t> NetworkInterface::GetActiveIpAddresses()
{
  std::vector<uint32_t> ip_addrs;
#if defined(_Win32)
    {
        PMIB_IPADDRTABLE pIPAddrTable = nullptr;
        DWORD dwSize = 0;
        DWORD dwRetVal = 0;

        // before calling AddIPAddress we use GetIpAddrTable to get an adapter to
        // which we can add the IP
        pIPAddrTable = (MIB_IPADDRTABLE *)malloc(sizeof(MIB_IPADDRTABLE));
        if (pIPAddrTable) {
            // make an initial call to GetIpAddrTable to get the necessary size into
            // the dwSize variable
            if (GetIpAddrTable(pIPAddrTable, &dwSize, 0) ==
                ERROR_INSUFFICIENT_BUFFER) {
                free(pIPAddrTable);
                pIPAddrTable = (MIB_IPADDRTABLE *)malloc(dwSize);
                // make a second call to GetIpAddrTable to get the actual data we want
                GetIpAddrTable(pIPAddrTable, &dwSize, 0);
            }
        }

        if (pIPAddrTable) {
            // iterate over each IP address
            for (int n = 0; n < (int)pIPAddrTable->dwNumEntries; n++) {
                uint32_t ip_addr = ntohl((u_long)pIPAddrTable->table[n].dwAddr);
                if ((0 != ip_addr) && (INADDR_LOOPBACK != ip_addr)) {
                    ip_addrs.push_back(ip_addr);
                }
            }
            free(pIPAddrTable);
        } else {
            throw std::runtime_error("Failed to obtain network interfaces");
        }
    }
#else
  {
    // BSD-style implementation
    struct ifaddrs *root_ifa;
    if (getifaddrs(&root_ifa) == 0) {
      struct ifaddrs *p = root_ifa;
      while (p) {
        struct sockaddr *a = p->ifa_addr;
        uint32_t ip_addr = ((a) && (a->sa_family == AF_INET))
                             ? ntohl(((struct sockaddr_in *)a)->sin_addr.s_addr)
                             : 0;

        if ((0 != ip_addr) && (INADDR_LOOPBACK != ip_addr)) {
          ip_addrs.push_back(ip_addr);
        }
        p = p->ifa_next;
      }
      freeifaddrs(root_ifa);
    } else {
      throw std::runtime_error("Failed to obtain network interfaces");
    }
  }
#endif

  return ip_addrs;
}

net_iface NetworkInterface::InitUDPSocket(uint32_t ip, uint16_t port)
{
  net_iface iface;
  SOCKET sockfd = INVALID_SOCKET;
  int r = 0;

  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (-1 == sockfd) {
    throw std::runtime_error("Failed to create socket");
  }

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = htonl(ip);

  r = bind(sockfd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr));
  if (0 != r) {
    CloseSocket(sockfd);
    throw std::runtime_error("Unable to bind the scan socket");
  }

  socklen_t len = sizeof(addr);
  r = getsockname(sockfd, reinterpret_cast<struct sockaddr *>(&addr), &len);
  if (0 != r) {
    CloseSocket(sockfd);
    throw std::runtime_error("Unable to retrieve the scan socket name");
  }

  memset(&iface, 0, sizeof(net_iface));
  iface.sockfd = sockfd;
  iface.ip_addr = ntohl(addr.sin_addr.s_addr);
  iface.port = ntohs(addr.sin_port);

  return iface;
}
