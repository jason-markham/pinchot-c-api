#ifdef __linux__
#else
#define _USE_MATH_DEFINES
#endif

#include <chrono>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

#include "joescan_pinchot.h"
#include "cxxopts.hpp"
#include "jsScanApplication.hpp"

/* NOTE: For Linux, set rmem manually for best results.
   # echo 0x10000000 > /proc/sys/net/core/rmem_max
*/

static std::mutex _mtx;
static std::vector<uint64_t> _recv_profiles;
static std::vector<uint64_t> _recv_packets;
static std::vector<uint64_t> _exp_packets;
static bool _is_load = false;
// these are included to help avoid the transform from getting optimized out
static volatile double _x = 0.0;
static volatile double _y = 0.0;
static volatile double _z = 0.0;

void PrintStatus(jsScanHeadStatus &stat)
{
  std::cout << "jsScanHeadStatus" << std::endl;
  std::cout << "\tglobal_time_ns=" << stat.global_time_ns << std::endl;
  std::cout << "\tnum_encoder_values=" << stat.num_encoder_values << std::endl;

  std::cout << "\tencoder_values=";
  for (uint32_t n = 0; n < stat.num_encoder_values; n++) {
    std::cout << stat.encoder_values[n];
    if (n != (stat.num_encoder_values - 1)) {
      std::cout << ",";
    } else {
      std::cout << std::endl;
    }
  }

  std::cout << "\tcamera_a_pixels_in_window="
            << stat.camera_a_pixels_in_window << std::endl;
  std::cout << "\tcamera_a_temp=" << stat.camera_a_temp << std::endl;


  std::cout << "\tcamera_b_pixels_in_window="
            << stat.camera_b_pixels_in_window << std::endl;
  std::cout << "\tcamera_b_temp=" << stat.camera_b_temp << std::endl;

  std::cout << "\tnum_profiles_sent=" << stat.num_profiles_sent << std::endl;
}

static void transform(jsProfile *profiles, uint32_t num_profiles)
{
  static const double roll = 1.0;
  static const double pitch = 2.0;
  static const double yaw = 3.0;
  static const double rho = M_PI / 180.0;
  static const double shift_x = 10.0;
  static const double shift_y = 20.0;
  static const double shift_z = 30.0;
  static const double cos_roll = std::cos(roll * rho);
  static const double cos_pitch = std::cos(pitch * rho);
  static const double cos_yaw = std::cos(yaw * rho);
  static const double sin_roll = std::sin(roll * rho);
  static const double sin_pitch = std::sin(pitch * rho);
  static const double sin_yaw = std::sin(yaw * rho);

  for (uint32_t m = 0; m < num_profiles; m++) {
    for (uint32_t n = 0; n < profiles[m].data_len; n++) {
      if (0 == profiles[m].data[n].brightness) {
        continue;
      }

      double x = static_cast<double>(profiles[m].data[n].x) / 1000.0;
      double y = static_cast<double>(profiles[m].data[n].y) / 1000.0;

      double xt = x * cos_yaw * cos_roll - y * sin_roll + shift_x;

      double yt = x * (cos_yaw * sin_roll * cos_pitch + sin_yaw * sin_pitch) +
                  y * cos_roll * cos_pitch + shift_y;

      double zt = x * (cos_yaw * sin_pitch * sin_roll - cos_pitch * sin_yaw) +
                  y * cos_roll * sin_pitch + shift_z;

      _x = xt;
      _y = yt;
      _z = zt;
    }
  }
}

/**
 * @brief This function receives profile data from a given scan head. We start
 * a thread for each scan head to pull out the data as fast as possible.
 *
 * @param scan_head Reference to the scan head to recieve profile data from.
 */
static void receiver(jsScanHead scan_head)
{
  jsProfile *profiles = nullptr;
  int max_profiles = 10;
  int sleep_ms = 1;

  // Allocate memory to recieve profile data for this scan head.
  profiles = new jsProfile[max_profiles];
  uint32_t serial = jsScanHeadGetSerial(scan_head);
  uint32_t idx = jsScanHeadGetId(scan_head);

  _mtx.lock();
  std::cout << "begin receiving on scan head " << serial << std::endl;
  _mtx.unlock();

  while (1) {
    int32_t r = 0;

    r = jsScanHeadWaitUntilProfilesAvailable(scan_head, 10, 50000);
    if (0 > r) {
      std::cout << "ERROR jsScanHeadWaitUntilProfilesAvailable returned" << r
                << std::endl;
      continue;
    } else if (0 == r) {
      std::cout << "no profiles left to read" << std::endl;
      break;
    }

    r = jsScanHeadGetProfiles(scan_head, profiles, max_profiles);
    if (0 > r) {
      std::cout << "ERROR jsScanHeadGetProfiles returned" << r << std::endl;
      continue;
    } else if (0 == r) {
      std::cout << "ERROR jsScanHeadGetProfiles no profiles" << std::endl;
      continue;
    }

    if (_is_load) {
      transform(profiles, r);
    }
    _mtx.lock();
    _recv_profiles[idx] += r;
    for (int n = 0; n < r; n++) {
      _recv_packets[idx] += profiles[n].udp_packets_received;
      _exp_packets[idx] += profiles[n].udp_packets_expected;
    }
    _mtx.unlock();
  }

  _mtx.lock();
  std::cout << "end receiving on scan head " << serial << std::endl;
  _mtx.unlock();

  delete[] profiles;
}

int main(int argc, char *argv[])
{
  jsScanSystem scan_system = 0;
  jsDataFormat fmt = JS_DATA_FORMAT_XY_BRIGHTNESS_FULL;

  std::thread *threads = nullptr;
  const int status_message_delay_sec = 1;
  uint64_t scan_time_sec = 10;
  uint32_t laser_min = 25;
  uint32_t laser_def = 25;
  uint32_t laser_max = 25;
  uint32_t period_us = 0;
  double window_top = 20.0;
  double window_bottom = -20.0;
  double window_left = -20.0;
  double window_right = 20.0;
  bool is_status = false;
  std::vector<uint32_t> serial_numbers;
  int32_t r = 0;

  try {
    cxxopts::Options options(argv[0], " - benchmark Joescan C API");
    std::string serials;
    std::string ft;
    std::string lzr;
    std::string prod;
    std::string window;

    options.add_options()(
      "t,time", "Duration in seconds", cxxopts::value<uint64_t>(scan_time_sec))(
      "f,format", "full, half, or quarter", cxxopts::value<std::string>(ft))(
      "l,laser", "usec def or min,def,max", cxxopts::value<std::string>(lzr))(
      "p,period", "scan period in us", cxxopts::value<uint32_t>(period_us))(
      "s,serial", "Serial numbers", cxxopts::value<std::string>(serials))(
      "w,window", "Scan window inches", cxxopts::value<std::string>(window))(
      "load", "Enable synthetic load", cxxopts::value<bool>(_is_load))(
      "status", "Get status while scanning", cxxopts::value<bool>(is_status))(
      "h,help", "Print help");

    auto parsed = options.parse(argc, argv);
    if (parsed.count("help")) {
      std::cout << options.help() << std::endl;
      exit(0);
    }

    if (parsed.count("serial")) {
      std::istringstream ss(serials);
      std::string token;
      while (std::getline(ss, token, ',')) {
        serial_numbers.emplace_back(strtoul(token.c_str(), nullptr, 0));
      }
    } else {
      std::cout << "no serial number(s) provided" << std::endl;
      exit(1);
    }

    if (parsed.count("format")) {
      if ((0 == ft.compare("full")) || (0 == ft.compare("FULL"))) {
        fmt = JS_DATA_FORMAT_XY_BRIGHTNESS_FULL;
      } else if ((0 == ft.compare("half")) || (0 == ft.compare("HALF"))) {
        fmt = JS_DATA_FORMAT_XY_BRIGHTNESS_HALF;
      } else if ((0 == ft.compare("quarter")) || (0 == ft.compare("QUARTER"))) {
        fmt = JS_DATA_FORMAT_XY_BRIGHTNESS_QUARTER;
      } else {
        std::cout << "invalianHead scan_head)d format: " << ft << std::endl;
        exit(1);
      }
    }

    {
      std::istringstream ss(lzr);
      std::string token;
      uint32_t n = 0;

      if (parsed.count("laser")) {
        while (std::getline(ss, token, ',')) {
          if (0 == n) {
            laser_def = strtoul(token.c_str(), nullptr, 0);
            laser_min = laser_def;
            laser_max = laser_def;
          } else if (1 == n) {
            laser_def = strtoul(token.c_str(), nullptr, 0);
            laser_max = laser_def;
          } else if (2 == n) {
            laser_max = strtoul(token.c_str(), nullptr, 0);
          }
          n++;
        }
      }
    }

    {
      std::istringstream ss(window);
      std::string token;
      uint32_t n = 0;

      if (parsed.count("window")) {
        while (std::getline(ss, token, ',')) {
          if (0 == n) {
            window_top = strtod(token.c_str(), nullptr);
          } else if (1 == n) {
            window_bottom = strtod(token.c_str(), nullptr);
          } else if (2 == n) {
            window_left = strtod(token.c_str(), nullptr);
          } else if (3 == n) {
            window_right = strtod(token.c_str(), nullptr);
          }
          n++;
        }
        // use the same parameter for all
        if (4 > n) {
          window_bottom = -1 * window_top;
          window_left = -1 * window_top;
          window_right = window_top;
        }
      }
    }

  } catch (const cxxopts::OptionException &e) {
    std::cout << "error parsing options: " << e.what() << std::endl;
    exit(1);
  }

  try {
    joescan::ScanApplication app;

    for (uint32_t n = 0; n < serial_numbers.size(); n++) {
      _recv_profiles.push_back(0);
      _recv_packets.push_back(0);
      _exp_packets.push_back(0);
    }

    app.SetSerialNumber(serial_numbers);
    app.SetLaserOn(laser_def, laser_min, laser_max);
    app.SetWindow(window_top, window_bottom, window_left, window_right);
    app.Configure();
    app.Connect();
    app.StartScanning(period_us, fmt, &receiver);

    auto scan_heads = app.GetScanHeads();
    // Periodically print out the number of profiles received.
    for (unsigned long i = 0; i < scan_time_sec; i++) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
      std::cout << i << std::endl;
      if (is_status) {
        jsScanHeadStatus status;
        r = jsScanHeadGetStatus(scan_heads[0], &status);
        if (0 == r) PrintStatus(status);
      }
    }

    app.StopScanning();

    // Read the number of profiles sent from each scan head to know how many
    // profiles we should expect to have recieved.
    jsScanHeadStatus status;
    std::vector<uint32_t> exp(scan_heads.size(), 0);
    for (uint32_t i = 0; i < scan_heads.size(); i++) {
      r = jsScanHeadGetStatus(scan_heads[i], &status);
      if (0 > r) {
        throw joescan::ApiError("failed to obtain status message", r);
      }
      exp[i] += status.num_profiles_sent;
    }

    // If everything went well and the system was able to run fast enough,
    // the number of recieved profiles will be equal to the total exp.
    for (uint32_t n = 0; n < scan_heads.size(); n++) {
      if ((_recv_packets[n] != _exp_packets[n]) ||
          (_recv_profiles[n] != exp[n])) {
        std::cout << "ERROR " << serial_numbers[n] << std::endl;
        std::cout << "\texpected profiles: " << exp[n] << std::endl;
        std::cout << "\treceived profiles: " << _recv_profiles[n] << std::endl;
        std::cout << "\texpected packets: " << _exp_packets[n] << std::endl;
        std::cout << "\treceived packets: " << _recv_packets[n] << std::endl;
        r = -1;
      } else {
        std::cout << "success " << serial_numbers[n] << std::endl;
        std::cout << "\tprofiles: " << exp[n] << std::endl;
        std::cout << "\tpackets: " << _exp_packets[n] << std::endl;
      }
    }

    app.Disconnect();

  } catch (joescan::ApiError &e) {
    std::cout << "ERROR: " << e.what() << std::endl;
    r = 1;

    const char *err_str = nullptr;
    jsError err = e.return_code();
    if (JS_ERROR_NONE != err) {
      jsGetError(err, &err_str);
      std::cout << "jsError (" << err << "): " << err_str << std::endl;
    }
  }

  return 0;
}
