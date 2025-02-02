/**
 * Copyright (c) JoeScan Inc. All Rights Reserved.
 *
 * Licensed under the BSD 3 Clause License. See LICENSE.txt in the project
 * root for license information.
 */

/**
 * @file discover.cpp
 * @brief Example showing how to use the discovery functions to probe the
 * network for JS-50 scan heads.
 */

#include <iostream>
#include <stdexcept>
#include <sstream>
#include <string>
#include "joescan_pinchot.h"

class ApiError : public std::runtime_error {
 private:
  jsError m_return_code;

 public:
  ApiError(const char* what, int32_t return_code) : std::runtime_error(what)
  {
    if ((0 < return_code) || (JS_ERROR_UNKNOWN > m_return_code)) {
      m_return_code = JS_ERROR_UNKNOWN;
    } else {
      m_return_code = (jsError) return_code;
    }
  }

  jsError return_code() const
  {
    return m_return_code;
  }
};

/**
 * @brief Display the API version to console output for visual confirmation as
 * to the version being used for this example.
 */
void PrintApiVersion()
{
  uint32_t major, minor, patch;
  jsGetAPISemanticVersion(&major, &minor, &patch);
  std::cout << "Joescan API version " << major << "." << minor << "." << patch
            << std::endl;
}

/**
 * @brief Display the capabilities of a scan head to console output.
 *
 * @param c Struct holding the scan head capabilities.
 */
static std::string IpToString(uint32_t ip)
{
  std::stringstream ipstream;
  ipstream << static_cast<uint32_t>(ip >> 24) << "."
           << static_cast<uint32_t>((ip >> 16) & 0xff) << "."
           << static_cast<uint32_t>((ip >> 8) & 0xff) << "."
           << static_cast<uint32_t>(ip & 0xff);

  return ipstream.str();
}

/**
 * @brief Prints the contents of a `jsScanHeadStatus` data type to standard out.
 *
 * @param stat Reference to scan head status to print.
 */
void PrintScanHeadDiscovered(jsDiscovered &d)
{
  std::cout << d.serial_number << "\n";
  std::cout << "  " << d.type_str << "\n";
  std::cout << "  Firmware v" << d.firmware_version_major << "."
                              << d.firmware_version_minor << "."
                              << d.firmware_version_patch << "\n";
  std::cout << "  IP Address " << IpToString(d.ip_addr) << std::endl;
}

int main(int argc, char *argv[])
{
  jsScanSystem scan_system = 0;
  jsDiscovered *discovered = nullptr;
  int32_t r = 0;

  try {
    PrintApiVersion();

    // One of the first calls to the API should be to create a scan manager
    // software object. When created, it will automatically perform discovery
    // to see what scan heads are available.
    scan_system = jsScanSystemCreate(JS_UNITS_INCHES);
    if (0 > scan_system) {
      throw ApiError("failed to create scan system", r);
    }

    // We can also manually trigger a discover to look for new scan heads or
    // scan heads that came online late due to delays in powering up hardware.
    r = jsScanSystemDiscover(scan_system);
    if (0 > r) {
      throw ApiError("failed to discover scan heads", r);
    }

    // The returned value, if non-negative, indicates how many scan heads were
    // found on the network.
    std::cout << "Discovered " << r << " JS-50 scan heads" << std::endl;
    discovered = new jsDiscovered[r];

    r = jsScanSystemGetDiscovered(scan_system, discovered, r);
    if (0 > r) {
      throw ApiError("failed to get discover scan heads", r);
    }

    for (int n = 0; n < r; n++) {
      PrintScanHeadDiscovered(discovered[n]);
    }
  } catch (ApiError &e) {
    std::cout << "ERROR: " << e.what() << std::endl;
    r = 1;

    const char *err_str = nullptr;
    jsError err = e.return_code();
    if (JS_ERROR_NONE != err) {
      jsGetError(err, &err_str);
      std::cout << "jsError (" << err << "): " << err_str << std::endl;
    }
  }

  // Clean up data allocated by the scan manager.
  jsScanSystemFree(scan_system);

  return r;
}
