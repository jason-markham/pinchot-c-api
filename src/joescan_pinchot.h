/**
 * Copyright (c) JoeScan Inc. All Rights Reserved.
 *
 * Licensed under the BSD 3 Clause License. See LICENSE.txt in the project
 * root for license information.
 */

/**
 * @file joescan_pinchot.h
 * @author JoeScan
 * @brief This file contains the interface for the client software used to
 * control scanning for JoeScan products.
 */

#ifndef _JOESCAN_PINCHOT_H
#define _JOESCAN_PINCHOT_H

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#pragma once

/**
 * @brief Opaque reference to an object in software used to manage a complete
 * system of scan heads.
 */
typedef int64_t jsScanSystem;

/**
 * @brief Opaque reference to an object in software that represents as single
 * physical scan head.
 */
typedef int64_t jsScanHead;

/**
 * @brief Constant values used with this API.
 */
enum jsConstants {
  JS_SCAN_HEAD_TYPE_STR_MAX_LEN = 32,
  /** @brief Array length of data reserved for a profile. */
  JS_PROFILE_DATA_LEN = 1456,
  /** @brief Array length of data reserved for a raw profile. */
  JS_RAW_PROFILE_DATA_LEN = 1456,
  /** @brief Maximum number of columns in an image taken from the scan head. */
  JS_CAMERA_IMAGE_DATA_MAX_WIDTH = 1456,
  /** @brief Maximum number of rows in an image taken from the scan head. */
  JS_CAMERA_IMAGE_DATA_MAX_HEIGHT = 1088,
  /** @brief Array length of data reserved for an image. */
  JS_CAMERA_IMAGE_DATA_LEN =
    JS_CAMERA_IMAGE_DATA_MAX_HEIGHT * JS_CAMERA_IMAGE_DATA_MAX_WIDTH,
  /**
   * @brief Value that `x` and `y` will be assigned in `jsProfileData` if the
   * point is invalid.
   */
  JS_PROFILE_DATA_INVALID_XY = INT_MIN,
  /**
   * @brief Value that `brightness` will be assigned in `jsProfileData` if
   * the measurement is invalid.
   */
  JS_PROFILE_DATA_INVALID_BRIGHTNESS = 0,
  /**
   * @brief The maximum number of profiles that can be read from a given
   * scan head with one API call.
   */
  JS_SCAN_HEAD_PROFILES_MAX = 1000,
};

/**
 * @brief Enumerated value for possible errors returned from API functions.
 *
 * @note These values can be converted to a string value by using the
 * `jsGetError` function.
 */
enum jsError {
  /** @brief No error. */
  JS_ERROR_NONE = 0,
  /** @brief Error resulted from internal code. */
  JS_ERROR_INTERNAL = -1,
  /** @brief Error resulted from `NULL` value passed in as argument. */
  JS_ERROR_NULL_ARGUMENT = -2,
  /** @brief Error resulted from incorrect or out of range argument value. */
  JS_ERROR_INVALID_ARGUMENT = -3,
  /** @brief Error resulted from the system not being in a connected state. */
  JS_ERROR_NOT_CONNECTED = -4,
  /** @brief Error resulted from the system being in a connected state. */
  JS_ERROR_CONNECTED = -5,
  /** @brief Error occurred from the system not being in a scanning state. */
  JS_ERROR_NOT_SCANNING = -6,
  /** @brief Error occurred from the system being in a scanning state. */
  JS_ERROR_SCANNING = -7,
  /** @brief Error occurred from a version compatibility issue between the scan
     head(s) and the API. */
  JS_ERROR_VERSION_COMPATIBILITY = -8,
  /** @brief Error occured trying to add or create something that already has
      been done and would result in a duplicate. */
  JS_ERROR_ALREADY_EXISTS = -9,
  /** @brief Error occured trying to add or create something, but due to
      constraints this can not be done. */
  JS_ERROR_NO_MORE_ROOM = -10,
  /** @brief Error occured with networking interface or communication. */
  JS_ERROR_NETWORK = -11,
  /** @brief Error occured with scan head not found on the network. */
  JS_ERROR_NOT_DISCOVERED = -12,
  /** @brief Error occured for an unknown reason; this should never happen. */
  JS_ERROR_UNKNOWN = -13,
};

/**
 * @brief The units that a given scan system and all associated scan heads will
 * use for configuration and returned data.
 */
typedef enum {
  JS_UNITS_INVALID = 0,
  JS_UNITS_INCHES = 1,
  JS_UNITS_MILLIMETER = 2,
} jsUnits;

/**
 * @brief The camera orientation for a given scan head; the orientation
 * selected influences the coordinate system of returned data.
 */
typedef enum {
  JS_CABLE_ORIENTATION_INVALID = 0,
  JS_CABLE_ORIENTATION_DOWNSTREAM = 1,
  JS_CABLE_ORIENTATION_UPSTREAM = 2,
} jsCableOrientation;

/**
 * @brief Enumerated value identifying the scan head type.
 */
typedef enum {
  JS_SCAN_HEAD_INVALID_TYPE = 0,
  JS_SCAN_HEAD_JS50WX = 1,
  JS_SCAN_HEAD_JS50WSC = 2,
  JS_SCAN_HEAD_JS50X6B20 = 3,
  JS_SCAN_HEAD_JS50X6B30 = 4,
} jsScanHeadType;

/**
 * @brief Data type for identifying a camera on the scan head.
 */
typedef enum {
  JS_CAMERA_INVALID = 0,
  JS_CAMERA_A = 1,
  JS_CAMERA_B,
  JS_CAMERA_MAX,
} jsCamera;

/**
 * @brief Data type for identifying a laser on the scan head.
 */
typedef enum {
  JS_LASER_INVALID = 0,
  JS_LASER_1 = 1,
  JS_LASER_2,
  JS_LASER_3,
  JS_LASER_4,
  JS_LASER_5,
  JS_LASER_6,
  JS_LASER_MAX,
} jsLaser;

/**
 * @brief Data type for identifying an encoder on the scan head.
 */
typedef enum {
  JS_ENCODER_MAIN = 0,
  JS_ENCODER_AUX_1,
  JS_ENCODER_AUX_2,
  JS_ENCODER_MAX,
} jsEncoder;

typedef enum {
  /** @brief ScanSync encoder A+/A- input connection is faulty. */
  JS_PROFILE_FLAG_ENCODER_MAIN_FAULT_A = 1 << 0,
  /** @brief ScanSync encoder B+/B- input connection is faulty. */
  JS_PROFILE_FLAG_ENCODER_MAIN_FAULT_B = 1 << 1,
  /** @brief ScanSync aux Y+/Y- input connection is faulty. */
  JS_PROFILE_FLAG_ENCODER_MAIN_FAULT_Y = 1 << 2,
  /** @brief ScanSync index Z+/Z- input connection is faulty. */
  JS_PROFILE_FLAG_ENCODER_MAIN_FAULT_Z = 1 << 3,
  /** @brief ScanSync encoder data rate exceeds hardware capabilities. */
  JS_PROFILE_FLAG_ENCODER_MAIN_OVERRUN = 1 << 4,
  /** @brief ScanSync termination resistor pairs installed. */
  JS_PROFILE_FLAG_ENCODER_MAIN_TERMINATION_ENABLE = 1 << 5,
  /** @brief ScanSync index Z input is logic high. */
  JS_PROFILE_FLAG_ENCODER_MAIN_INDEX_Z = 1 << 6,
  /** @brief ScanSync sync input is logic high. */
  JS_PROFILE_FLAG_ENCODER_MAIN_SYNC = 1 << 7,
} jsProfileFlags;

/**
 * @brief Enumerated value representing the types of data and the formats it
 * can take. For full resolution data formats, every data entry will be filled
 * within the returned profile's `data` array. Selecting half or quarter
 * resolution will result in every other or every fourth entry in the `data`
 * array to be filled respectively.
 */
typedef enum {
  JS_DATA_FORMAT_INVALID = 0,
  // Geometry and laser line brightness at combinations of full, 1/2, and 1/4
  // resolution.
  JS_DATA_FORMAT_XY_BRIGHTNESS_FULL,
  JS_DATA_FORMAT_XY_BRIGHTNESS_HALF,
  JS_DATA_FORMAT_XY_BRIGHTNESS_QUARTER,

  // Geometry at full, 1/2 and 1/4 resolution, no laser line brightness.
  JS_DATA_FORMAT_XY_FULL,
  JS_DATA_FORMAT_XY_HALF,
  JS_DATA_FORMAT_XY_QUARTER,
} jsDataFormat;

/**
 * @brief Data type for setting fixed camera & laser exposure or to use the
 * auto exposure algorithm when obtaining diagnostic profiles and images.
 */
typedef enum {
  JS_DIAGNOSTIC_MODE_INVALID = 0,
  JS_DIAGNOSTIC_FIXED_EXPOSURE,
  JS_DIAGNOSTIC_AUTO_EXPOSURE,
} jsDiagnosticMode;

#pragma pack(push, 1)

/**
 * @brief Structure used to provide information as to scan heads discovered on
 * the network.
 */
typedef struct {
  uint32_t serial_number;
  uint32_t ip_addr;
  jsScanHeadType type;
  char type_str[JS_SCAN_HEAD_TYPE_STR_MAX_LEN];
  /** @brief Firmware major version number of the scan head. */
  uint32_t firmware_version_major;
  /** @brief Firmware minor version number of the scan head. */
  uint32_t firmware_version_minor;
  /** @brief Firmware patch version number of the scan head. */
  uint32_t firmware_version_patch;
} jsDiscovered;

/**
 * @brief Structure used to communicate the various capabilities and limits of
 * a given scan head type.
 */
typedef struct {
  /** @brief Number of bits used for a `brightness` value in `jsProfileData`. */
  uint32_t camera_brightness_bit_depth;
  /** @brief Maximum image height camera supports. */
  uint32_t max_camera_image_height;
  /** @brief Maximum image width camera supports. */
  uint32_t max_camera_image_width;
  /** @brief The smallest scan period in microseconds supported by product. */
  uint32_t min_scan_period_us;
  /** @brief The largest scan period in microseconds supported by product. */
  uint32_t max_scan_period_us;
  /** @brief The number of cameras supported by product. */
  uint32_t num_cameras;
  /** @brief The number of encoders supported by product. */
  uint32_t num_encoders;
  /** @brief The number of lasers supported by product. */
  uint32_t num_lasers;
} jsScanHeadCapabilities;

/**
 * @brief Structure used to configure a scan head's operating parameters.
 */
typedef struct {
  /**
   * @brief Sets the minimum microseconds time value for the camera
   * autoexposure algorithm used when the scan head is in image mode. This
   * value should be within the range of 15 to 2000000 microseconds.
   *
   * @note To disable autoexposure algorithm, set `camera_exposure_time_min_us`,
   * `camera_exposure_time_max_us`, and `camera_exposure_time_def_us` to the
   * same value.
   */
  uint32_t camera_exposure_time_min_us;
  /**
   * @brief Sets the maximum microseconds time value for the camera
   * autoexposure algorithm used when the scan head is in image mode. This
   * value should be within the range of 15 to 2000000 microseconds.
   *
   * @note To disable autoexposure algorithm, set `camera_exposure_time_min_us`,
   * `camera_exposure_time_max_us`, and `camera_exposure_time_def_us` to the
   * same value.
   */
  uint32_t camera_exposure_time_max_us;
  /**
   * @brief Sets the default microseconds time value for the camera
   * autoexposure algorithm used when the scan head is in image mode. This
   * value should be within the range of 15 to 2000000 microseconds.
   *
   * @note To disable autoexposure algorithm, set `camera_exposure_time_min_us`,
   * `camera_exposure_time_max_us`, and `camera_exposure_time_def_us` to the
   * same value.
   */
  uint32_t camera_exposure_time_def_us;
  /**
   * @brief Sets the minimum microseconds time value for the laser on
   * algorithm. This value should be within the range of 15 to 650000
   * microseconds.
   *
   * @note To disable the laser on algorithm, set `laser_on_time_min_us`,
   * `laser_on_time_max_us`, and `laser_on_time_def_us` to the same value.
   */
  uint32_t laser_on_time_min_us;
  /**
   * @brief Sets the maximum microseconds time value for the laser on
   * algorithm. This value should be within the range of 15 to 650000
   * microseconds.
   *
   * @note To disable the laser on algorithm, set `laser_on_time_min_us`,
   * `laser_on_time_max_us`, and `laser_on_time_def_us` to the same value.
   */
  uint32_t laser_on_time_max_us;
  /**
   * @brief Sets the default microseconds time value for the laser on
   * algorithm. This value should be within the range of 15 to 650000
   * microseconds.
   *
   * @note To disable the laser on algorithm, set `laser_on_time_min_us`,
   * `laser_on_time_max_us`, and `laser_on_time_def_us` to the same value.
   */
  uint32_t laser_on_time_def_us;
  /**
   * @brief The minimum brightness a data point must have to be considered a
   * valid data point. Value must be between `0` and `1023`.
   */
  uint32_t laser_detection_threshold;
  /**
   * @brief Set how bright a data point must be to be considered saturated.
   * Value must be between `0` and `1023`.
   */
  uint32_t saturation_threshold;
  /**
   * @brief Set the maximum percentage of the pixels in a scan that are allowed
   * to be brighter than the saturation threshold. Value must be between `0`
   * and `100`.
   */
  uint32_t saturation_percentage;
} jsScanHeadConfiguration;

/**
 * @brief Structure used to hold information pertaining to the scan head.
 */
typedef struct {
  /** @brief System global time in nanoseconds. */
  uint64_t global_time_ns;
  /** @brief The current encoder positions. */
  int64_t encoder_values[JS_ENCODER_MAX];
  /** @brief The number of encoder values available. */
  uint32_t num_encoder_values;
  /** @brief Total number of pixels seen by the camera A's scan window. */
  int32_t camera_a_pixels_in_window;
  /** @brief Total number of pixels seen by the camera B's scan window. */
  int32_t camera_b_pixels_in_window;
  /** @brief Current temperature in Celsius reported by camera A. */
  int32_t camera_a_temp;
  /** @brief Current temperature in Celsius reported by camera B. */
  int32_t camera_b_temp;
  /** @brief Total number of profiles sent during the last scan period. */
  uint32_t num_profiles_sent;
} jsScanHeadStatus;

/**
 * @brief A data point within a returned profile's data.
 */
typedef struct {
  /**
   * @brief The X coordinate in 1/1000 scan system units.
   * @note If invalid, will be set to `JS_PROFILE_DATA_INVALID_XY`.
   */
  int32_t x;
  /**
   * @brief The Y coordinate in 1/1000 scan system units.
   * @note If invalid, will be set to `JS_PROFILE_DATA_INVALID_XY`.
   */
  int32_t y;
  /**
   * @brief Measured brightness at given point.
   * @note If invalid, will be set to `JS_PROFILE_DATA_INVALID_BRIGHTNESS`.
   */
  int32_t brightness;
} jsProfileData;

/**
 * @brief Scan data is returned from the scan head through profiles; each
 * profile returning a single scan line at a given moment in time.
 */
typedef struct {
  /** @brief The Id of the scan head that the profile originates from. */
  uint32_t scan_head_id;
  /** @brief The camera used for the profile. */
  jsCamera camera;
  /** @brief The laser used for the profile. */
  jsLaser laser;
  /** @brief Time of the scan head in nanoseconds when profile was taken. */
  uint64_t timestamp_ns;
  /** @brief Bitmask of flags listed in `enum jsProfileFlags`. */
  uint32_t flags;
  /** @brief Monotonically increasing count of profiles generated by camera. */
  uint32_t sequence_number;
  /** @brief Array holding current encoder values. */
  int64_t encoder_values[JS_ENCODER_MAX];
  /** @brief Number of encoder values in this profile. */
  uint32_t num_encoder_values;
  /** @brief Time in microseconds for the laser emitting. */
  uint32_t laser_on_time_us;
  /**
   * @brief The format of the data for the given `jsProfile`.
   */
  jsDataFormat format;
  /**
   * @brief Number of packets received for the profile. If less than
   * `packets_expected`, then the profile data is incomplete. Generally,
   * this implies some type of network issue.
   */
  uint32_t packets_received;
  /** @brief Total number of packets expected to comprise the profile. */
  uint32_t packets_expected;
  /**
   * @brief The total number of valid scan line measurement points for this
   * profile held in the `data` array.
   */
  uint32_t data_len;
  /** @brief Reserved for future use. */
  uint64_t reserved_0;
  /** @brief Reserved for future use. */
  uint64_t reserved_1;
  /** @brief Reserved for future use. */
  uint64_t reserved_2;
  /** @brief Reserved for future use. */
  uint64_t reserved_3;
  /** @brief Reserved for future use. */
  uint64_t reserved_4;
  /** @brief Reserved for future use. */
  uint64_t reserved_5;
  /** @brief An array of scan line data associated with this profile. */
  jsProfileData data[JS_PROFILE_DATA_LEN];
} jsProfile;

/**
 * @brief A Raw Profile is the most basic type of profile returned back from
 * a scan head. The data is left unprocessed with the contents being dependent
 * on the `jsDataFormat` that the scan head is configured for.
 *
 * @note It is highly recommended to use `jsProfile` rather than `jsRawProfile`
 * as the former is far more simplified and user friendly than the later which
 * presents low level API implementation details.
 */
typedef struct {
  /** @brief The Id of the scan head that the profile originates from. */
  uint32_t scan_head_id;
  /** @brief The camera used for the profile. */
  jsCamera camera;
  /** @brief The laser used for the profile. */
  jsLaser laser;
  /** @brief Time of the scan head in nanoseconds when profile was taken. */
  uint64_t timestamp_ns;
  /** @brief Bitmask of flags listed in `enum jsProfileFlags`. */
  uint32_t flags;
  /** @brief Monotonically increasing count of profiles generated by camera. */
  uint32_t sequence_number;
  /** @brief Array holding current encoder values. */
  int64_t encoder_values[JS_ENCODER_MAX];
  /** @brief Number of encoder values in this profile. */
  uint32_t num_encoder_values;
  /** @brief Time in microseconds for the laser emitting. */
  uint32_t laser_on_time_us;
  /**
   * @brief The arrangement profile data contained in the `data` array. For raw
   * profile data, the data will be filled in at most according to the
   * resolution specified in the `jsDataFormat` type. For full resolution, the
   * entire scan line will be sampled. For half resolution, half of the scan
   * line will be sampled and `data` array will be populated at every other
   * entry. Similarly for quarter resolution, every fourth entry will be used
   * for scan data. Comparison with `JS_PROFILE_DATA_INVALID_XY` for X/Y values
   * and `JS_PROFILE_DATA_INVALID_BRIGHTNESS` can be used to determine if
   * data is set or not.
   */
  jsDataFormat format;
  /**
   * @brief Number of packets received for the profile. If less than
   * `packets_expected`, then the profile data is incomplete. Generally,
   * this implies some type of network issue.
   */
  uint32_t packets_received;
  /** @brief Total number of packets expected to comprise the profile. */
  uint32_t packets_expected;
  /**
   * @brief The total length of profile data held in the `data` array. This
   * value will be less than or equal to `JS_RAW_PROFILE_DATA_LEN` and should
   * be used for iterating over the array.
   */
  uint32_t data_len;
  /**
   * @brief Number of `brightness` values in the `data` array that are valid.
   * Invalid `brightness` will be set to `JS_PROFILE_DATA_INVALID_BRIGHTNESS`.
   */
  uint32_t data_valid_brightness;
  /**
   * @brief Number of `x` and `y` values in the `data` array that are valid.
   * Invalid `x` and `y` will have both set to `JS_PROFILE_DATA_INVALID_XY`.
   */
  uint32_t data_valid_xy;
  /** @brief Reserved for future use. */
  uint64_t reserved_0;
  /** @brief Reserved for future use. */
  uint64_t reserved_1;
  /** @brief Reserved for future use. */
  uint64_t reserved_2;
  /** @brief Reserved for future use. */
  uint64_t reserved_3;
  /** @brief Reserved for future use. */
  uint64_t reserved_4;
  /** @brief Reserved for future use. */
  uint64_t reserved_5;
  /** @brief An array of scan line data associated with this profile. */
  jsProfileData data[JS_RAW_PROFILE_DATA_LEN];
} jsRawProfile;

/**
 * @brief This structure is used to return a greyscale image capture from the
 * scan head.
 */
typedef struct {
  /** @brief The Id of the scan head that the image originates from. */
  uint32_t scan_head_id;
  /** @brief The camera used to capture the image. */
  jsCamera camera;
  /** @brief The laser used during the image capture. */
  jsLaser laser;
  /** @brief Time of the scan head in nanoseconds when image was taken. */
  uint64_t timestamp_ns;
  /** @brief Array holding current encoder values. */
  uint64_t encoder_values[JS_ENCODER_MAX];
  /** @brief Number of encoder values in this profile. */
  uint32_t num_encoder_values;
  /** @brief Time in microseconds for the camera's exposure. */
  uint32_t camera_exposure_time_us;
  /** @brief Time in microseconds the laser is emitting. */
  uint32_t laser_on_time_us;

  /** @brief The overall height of the image in pixels. */
  uint32_t image_height;
  /** @brief The overall width of the image in pixels. */
  uint32_t image_width;

  /** @brief An array of pixel data representing the image. */
  uint8_t data[JS_CAMERA_IMAGE_DATA_LEN];
} jsCameraImage;

#pragma pack(pop)

#ifndef NO_PINCHOT_INTERFACE

// Macros for setting the visiblity of functions within the library.
#if defined _WIN32 || defined __CYGWIN__
  #ifdef WIN_EXPORT
    #ifdef __GNUC__
      #define EXPORTED __attribute__((dllexport))
    #else
      #define EXPORTED __declspec(dllexport)
    #endif
  #else
    #ifdef __GNUC__
      #define EXPORTED __attribute__((dllexport))
    #else
      #define EXPORTED __declspec(dllexport)
    #endif
  #endif
  #define NOT_EXPORTED
  #define PRE __cdecl
  #define POST
#else
  #if __GNUC__ >= 4
    #define EXPORTED __attribute__((visibility("default")))
    #define NOT_EXPORTED __attribute__((visibility("hidden")))
  #else
    #define EXPORTED
    #define NOT_EXPORTED
  #endif
  #define PRE
  #define POST __attribute__((sysv_abi))
#endif

/**
 * @brief Obtains the semantic version of the client API presented in this
 * header. The version string will be of the form `vX.Y.Z`, where `X` is the
 * major version number, `Y` is the minor version number, and `Z` is the patch
 * number. In some cases, additional information may be appended to the version
 * string, separated by hyphens.
 *
 * @param version_str Address to be updated with API version string.
 */
EXPORTED void PRE jsGetAPIVersion(
  const char **version_str) POST;

/**
 * @brief Obtains the semantic version of the client API presented as unsigned
 * integer values.
 *
 * @param major The major version number.
 * @param minor The minor version number.
 * @param patch The patch version number.
 */
EXPORTED void PRE jsGetAPISemanticVersion(
  uint32_t *major,
  uint32_t *minor,
  uint32_t *patch) POST;

/**
 * @brief Converts a `jsError` error value returned from an API function call
 * to a string value.
 *
 * @param return_code The `jsError` value returned from API call.
 * @param error_str Address to be updated with error string.
 */
EXPORTED void PRE jsGetError(
  int32_t return_code,
  const char **error_str) POST;

/**
 * @brief Creates a `jsScanSystem` used to manage and coordinate `jsScanHead`
 * objects.
 *
 * @param units The units the scan system and all scan heads will use.
 * @return Positive valued token on success, negative value mapping to `jsError`
 * on error.
 */
EXPORTED jsScanSystem PRE jsScanSystemCreate(
  jsUnits units) POST;

/**
 * @brief Frees a `jsScanSystem` and all resources associated with it. In
 * particular, this will free all `jsScanHead` objects created by this
 * object.
 *
 * @param scan_system Reference to system that will be freed.
 */
EXPORTED void PRE jsScanSystemFree(
  jsScanSystem scan_system) POST;

/**
 * @brief Performs a network discovery to determine what scan heads are on the
 * network.
 *
 * @param scan_system The scan system to perform discovery.
 * @return The total number of discovered scan heads on success, negative value
 * mapping to `jsError` on error.
 */
EXPORTED int PRE jsScanSystemDiscover(
  jsScanSystem scan_system) POST;

/**
 * @brief Obtains a list of all of the scan heads discovered on the network.
 *
 * @param scan_system The scan system that previously performed discovery.
 * @return The total number of discovered scan heads on success, negative value
 * mapping to `jsError` on error.
 */
EXPORTED int PRE jsScanSystemGetDiscovered(
  jsScanSystem scan_system,
  jsDiscovered *results,
  uint32_t max_results) POST;

/**
 * @brief Creates a `jsScanHead` object representing a physical scan head
 * within the system.
 *
 * @note This function can only be called when the scan system is disconnected.
 * Once `jsScanSystemConnect()` is called, `jsScanSystemDisconnect()` must be
 * called if new scan heads are desired to be created.
 *
 * @param scan_system Reference to system that will own the scan head.
 * @param serial The serial number of the physical scan head.
 * @param id A user defined numerically unique id to assign to this scan head.
 * @return Positive valued token on success, negative value mapping to `jsError`
 * on error.
 */
EXPORTED jsScanHead PRE jsScanSystemCreateScanHead(
  jsScanSystem scan_system,
  uint32_t serial,
  uint32_t id) POST;

/**
 * @brief Obtains a reference to an existing `jsScanHead` object.
 *
 * @param scan_system Reference to system that owns the scan head.
 * @param id The numeric ID of the `jsScanHead` object.
 * @return Positive valued token on success, negative value mapping to `jsError`
 * on error.
 */
EXPORTED jsScanHead PRE jsScanSystemGetScanHeadById(
  jsScanSystem scan_system,
  uint32_t id) POST;

/**
 * @brief Obtains a reference to an existing `jsScanHead` object.
 *
 * @param scan_system Reference to system that owns the scan head.
 * @param serial The serial number of the physical scan head.
 * @return Positive valued token on success, negative value mapping to `jsError`
 * on error.
 */
EXPORTED jsScanHead PRE
jsScanSystemGetScanHeadBySerial(
  jsScanSystem scan_system,
  uint32_t serial) POST;

/**
 * @brief Returns the total number of scan heads within a given system. This
 * should equal the number of times `jsScanSystemCreateScanHead()` was
 * successfully called with a new serial number.
 *
 * @param scan_system Reference to system that owns the scan heads.
 * @return The number of scan heads on success, negative value mapping to
 * `jsError` on error.
 */
EXPORTED int32_t PRE
jsScanSystemGetNumberScanHeads(
  jsScanSystem scan_system) POST;

/**
 * @brief Attempts to connect to all scan heads within the system.
 *
 * @param scan_system Reference to system owning scan heads to connect to.
 * @param timeout_s TCP timeout for all managed scan heads in seconds.
 * @return The total number of connected scan heads on success, negative value
 * mapping to `jsError` on error.
 */
EXPORTED int32_t PRE jsScanSystemConnect(
  jsScanSystem scan_system,
  int32_t timeout_s) POST;

/**
 * @brief Disconnects all scan heads from a given system.
 *
 * @param scan_system Reference to system of scan heads.
 * @return `0` on success, negative value `jsError` on error.
 */
EXPORTED int32_t PRE jsScanSystemDisconnect(
  jsScanSystem scan_system) POST;

/**
 * @brief Gets connected state for a scan system.
 *
 * @note A scan system is said to be connected if all of the scan heads
 * associated with it are connected.
 *
 * @param scan_system Reference to system of scan heads.
 * @return Boolean `true` if connected, `false` if disconnected.
 */
EXPORTED bool PRE jsScanSystemIsConnected(
  jsScanSystem scan_system) POST;

/**
 * @brief Clears all phases created for a given scan system previously made
 * through `jsScanSystemPhaseCreate`.
 *
 * @param scan_system Reference to the scan system.
 * @return `0` on success, negative value mapping to `jsError` on error.
 */
EXPORTED int32_t PRE jsScanSystemPhaseClearAll(
  jsScanSystem scan_system) POST;

/**
 * @brief Creates a new phase entry for the scan system. If no phases have been
 * created for the scan system it will create the first phase entry. If entries
 * have been created already through previous calls to `jsScanSystemPhaseCreate`
 * then the new phase will be placed after the last created phase.
 *
 * @param scan_system Reference to the scan system.
 * @return `0` on success, negative value mapping to `jsError` on error.
 */
EXPORTED int32_t PRE jsScanSystemPhaseCreate(
  jsScanSystem scan_system) POST;

/**
 * @brief Inserts a scan head and it's camera into a given phase entry.
 * Multiple scan heads may be inserted into a given phase entry. However, the
 * the total phase time for this paticular entry will be constrained by the
 * scan head that requires the most time to complete it's scan.
 *
 * @note This function should be used with scan heads that are camera driven.
 *
 * @param scan_system Reference to the scan system.
 * @param scan_head Reference to the scan head.
 * @param camera The camera of the scan head to scan with during phase entry.
 * @return `0` on success, negative value mapping to `jsError` on error.
 */
EXPORTED int32_t PRE jsScanSystemPhaseInsertCamera(
  jsScanSystem scan_system,
  jsScanHead scan_head,
  jsCamera camera) POST;

/**
 * @brief Inserts a scan head and it's camera into a given phase entry.
 * Multiple scan heads may be inserted into a given phase entry. However, the
 * the total phase time for this paticular entry will be constrained by the
 * scan head that requires the most time to complete it's scan.
 *
 * @note This function should be used with scan heads that are laser driven.
 *
 * @param scan_system Reference to the scan system.
 * @param scan_head Reference to the scan head.
 * @param laser The camera of the scan head to scan with during phase entry.
 * @return `0` on success, negative value mapping to `jsError` on error.
 */
EXPORTED int32_t PRE jsScanSystemPhaseInsertLaser(
  jsScanSystem scan_system,
  jsScanHead scan_head,
  jsLaser laser) POST;

/**
 * @brief Inserts a scan head and it's camera into a given phase entry.
 * Multiple scan heads may be inserted into a given phase entry. However, the
 * the total phase time for this paticular entry will be constrained by the
 * scan head that requires the most time to complete it's scan.
 *
 * @note This function will use the `jsScanHeadConfiguration` provided as a
 * function argument when configuring this particular camera for scanning
 * rather than that specified in `jsScanHeadSetConfiguration`.
 *
 * @note Only the `laser_on_time_max_us`, `laser_on_time_def_us`, and
 * `laser_on_time_min_us` fields from `jsScanHeadSetConfiguration`
 * are currently used for per phase configuration.
 *
 * @note This function should be used with scan heads that are camera driven.
 *
 * @param scan_system Reference to the scan system.
 * @param scan_head Reference to the scan head.
 * @param camera The camera of the scan head to scan with during phase entry.
 * @return `0` on success, negative value mapping to `jsError` on error.
 */
EXPORTED int32_t PRE jsScanSystemPhaseInsertCameraConfiguration(
  jsScanSystem scan_system,
  jsScanHead scan_head,
  jsCamera camera,
  jsScanHeadConfiguration cfg) POST;

/**
 * @brief Inserts a scan head and it's laser into a given phase entry.
 * Multiple scan heads may be inserted into a given phase entry. However, the
 * the total phase time for this paticular entry will be constrained by the
 * scan head that requires the most time to complete it's scan.
 *
 * @note This function will use the `jsScanHeadConfiguration` provided as a
 * function argument when configuring this particular camera for scanning
 * rather than that specified in `jsScanHeadSetConfiguration`.
 *
 * @note Only the `laser_on_time_max_us`, `laser_on_time_def_us`, and
 * `laser_on_time_min_us` fields from `jsScanHeadSetConfiguration` are
 * currently used for per phase configuration.
 *
 * @note This function should be used with scan heads that are laser driven.
 *
 * @param scan_system Reference to the scan system.
 * @param scan_head Reference to the scan head.
 * @param laser The camera of the scan head to scan with during phase entry.
 * @param cfg Configuration to be applied to the laser.
 * @return `0` on success, negative value mapping to `jsError` on error.
 */
EXPORTED int32_t PRE jsScanSystemPhaseInsertLaserConfiguration(
  jsScanSystem scan_system,
  jsScanHead scan_head,
  jsLaser laser,
  jsScanHeadConfiguration cfg) POST;

/**
 * @brief Obtains the minimum period that a given scan system can achieve
 * scanning when `jsScanSystemStartScanning` is called.
 *
 * @param scan_system Reference to system of scan heads.
 * @return The minimum scan period in microseconds on success, negative value
 * mapping to `jsError` on error.
 */
EXPORTED int32_t PRE
jsScanSystemGetMinScanPeriod(
  jsScanSystem scan_system) POST;

/**
 * @brief Commands scan heads in system to begin scanning, returning geometry
 * and/or brightness values to the client.
 *
 * @note The internal memory buffers of the scan heads will be cleared of all
 * old profile data upon start of scan. Ensure that all data from the previous
 * scan that is desired is read out before calling this function.
 *
 * @param scan_system Reference to system of scan heads.
 * @param period_us The scan period in microseconds.
 * @param fmt The data format of the returned scan profile data.
 * @return `0` on success, negative value mapping to `jsError` on error.
 */
EXPORTED int32_t PRE jsScanSystemStartScanning(
  jsScanSystem scan_system,
  uint32_t period_us,
  jsDataFormat fmt) POST;

/**
 * @brief Commands scan heads in system to stop scanning.
 *
 * @param scan_system Reference to system of scan heads.
 * @return `0` on success, negative value `jsError` on error.
 */
EXPORTED int32_t PRE jsScanSystemStopScanning(
  jsScanSystem scan_system) POST;

/**
 * @brief Gets scanning state for a scan system.
 *
 * @param scan_system Reference to system of scan heads.
 * @return Boolean `true` if scanning, `false` if not scanning.
 */
EXPORTED bool PRE jsScanSystemIsScanning(
  jsScanSystem scan_system) POST;

/**
 * @brief Obtains the product type of a given scan head.
 *
 * @note This function can only be called when a scan head is successfully
 * connected after calling `jsScanSystemConnect()`.
 *
 * @param scan_head Reference to scan head.
 * @return The enumerated scan head type.
 */
EXPORTED jsScanHeadType PRE jsScanHeadGetType(
  jsScanHead scan_head) POST;

/**
 * @brief Obtains the ID of the scan head.
 *
 * @param scan_head Reference to scan head.
 * @return The numeric ID of the scan head.
 */
EXPORTED uint32_t PRE jsScanHeadGetId(
  jsScanHead scan_head) POST;

/**
 * @brief Obtains the serial number of the scan head.
 *
 * @param scan_head Reference to scan head.
 * @return The serial number of the scan head.
 */
EXPORTED uint32_t PRE jsScanHeadGetSerial(
  jsScanHead scan_head) POST;

/**
 * @brief Obtains the capabilities for a given scan head.
 *
 * @param type The scan head product type to obtain capabilities for.
 * @param capabilities Pointer to struct to be filled with capabilities.
 * @return `0` on success, negative value mapping to `jsError` on error.
 */
EXPORTED int32_t PRE jsScanHeadGetCapabilities(
  jsScanHead scan_head,
  jsScanHeadCapabilities *capabilities) POST;

/**
 * @brief Obtains the firmware version of the scan head presented as unsigned
 * integer values.
 *
 * @param major The major version number.
 * @param minor The minor version number.
 * @param patch The patch version number.
 * @return `0` on success, negative value mapping to `jsError` on error.
 */
EXPORTED int32_t PRE jsScanHeadGetFirmwareVersion(
  jsScanHead scan_head,
  uint32_t *major,
  uint32_t *minor,
  uint32_t *patch) POST;

/**
 * @brief Obtains the connection state of a given scan head.
 *
 * @param scan_head Reference to scan head.
 * @return Boolean `true` on connected, `false` otherwise.
 */
EXPORTED bool PRE jsScanHeadIsConnected(
  jsScanHead scan_head) POST;

/**
 * @brief Configures the scan head according to the parameters specified.
 *
 * @note The configuration settings are sent to the scan head during the call
 * to `jsScanSystemStartScanning()`.
 *
 * @param scan_head Reference to scan head to be configured.
 * @param cfg The `jsScanHeadConfiguration` to be applied.
 * @return `0` on success, negative value mapping to `jsError` on error.
 */
EXPORTED int32_t PRE jsScanHeadSetConfiguration(
  jsScanHead scan_head,
  jsScanHeadConfiguration *cfg) POST;

/**
 * @brief Gets the scan head's configuration settings configured by the
 * `jsScanHeadConfiguration` function.
 *
 * @param scan_head Reference to scan head to be configured.
 * @param cfg The `jsScanHeadConfiguration` to be updated with current settings
 * @return `0` on success, negative value mapping to `jsError` on error.
 */
EXPORTED int32_t PRE jsScanHeadGetConfiguration(
  jsScanHead scan_head,
  jsScanHeadConfiguration *cfg) POST;

/**
 * @brief Gets the safe default configuration for a given scan head.
 *
 * @param scan_head Reference to scan head to be configured.
 * @param cfg The `jsScanHeadConfiguration` to be updated with default.
 * @return `0` on success, negative value mapping to `jsError` on error.
 */
EXPORTED int32_t PRE jsScanHeadGetConfigurationDefault(
  jsScanHead scan_head,
  jsScanHeadConfiguration *cfg) POST;

/**
 * @brief Gets the safe default configuration for a given scan head.
 *
 * @param scan_head Reference to scan head to be configured.
 * @param cfg The `jsScanHeadConfiguration` to be updated with default.
 * @return `0` on success, negative value mapping to `jsError` on error.
 */
EXPORTED int32_t PRE jsScanHeadSetCableOrientation(
  jsScanHead scan_head,
  jsCableOrientation cable_orientation);

/**
 * @brief Gets the safe default configuration for a given scan head.
 *
 * @param scan_head Reference to scan head to be configured.
 * @param cfg The `jsScanHeadConfiguration` to be updated with default.
 * @return `0` on success, negative value mapping to `jsError` on error.
 */
EXPORTED int32_t PRE jsScanHeadGetCableOrientation(
  jsScanHead scan_head,
  jsCableOrientation *cable_orientation);

/**
 * @brief Configures spatial parameters of the scan head in order to properly
 * transform the data from a camera based coordinate system to one based on
 * mill placement.
 *
 * @note The alignment settings are sent to the scan head during the call to
 * `jsScanSystemConnect`.
 *
 * @param scan_head Reference to scan head.
 * @param roll_degrees The rotation in degrees to be applied to the Z axis.
 * @param shift_x The shift to be applied specified in scan system units.
 * @param shift_y The shift to be applied specified in scan system units.
 * @return `0` on success, negative value mapping to `jsError` on error.
 */
EXPORTED int32_t PRE jsScanHeadSetAlignment(
  jsScanHead scan_head,
  double roll_degrees,
  double shift_x,
  double shift_y) POST;

/**
 * @brief Configures spatial parameters of the scan head in order to properly
 * transform the data from a camera based coordinate system to one based on
 * mill placement. This function is similar to `jsScanHeadSetAlignment`
 * except that it only applies to one camera rather than both.
 *
 * @note The alignment settings are sent to the scan head during the call to
 * `jsScanSystemConnect`.
 *
 * @param scan_head Reference to scan head.
 * @param camera The camera to apply parameters to.
 * @param roll_degrees The rotation in degrees to be applied.
 * @param shift_x The shift to be applied specified in scan system units.
 * @param shift_y The shift to be applied specified in scan system units.
 * @return `0` on success, negative value mapping to `jsError` on error.
 */
EXPORTED int32_t PRE jsScanHeadSetAlignmentCamera(
  jsScanHead scan_head,
  jsCamera camera,
  double roll_degrees,
  double shift_x,
  double shift_y) POST;

/**
 * @brief Obtains the currently applied alignment settings.
 *
 * @note If configured using `jsScanHeadSetAlignment`, each camera will have
 * the same alignment settings.
 *
 * @param scan_head Reference to scan head.
 * @param camera The camera to get settings from.
 * @param roll_degrees Variable to hold roll in degrees.
 * @param shift_x Variable to hold shift in scan system units.
 * @param shift_y Variable to hold shift in scan system units.
 * @return `0` on success, negative value mapping to `jsError` on error.
 */
EXPORTED int32_t PRE jsScanHeadGetAlignmentCamera(
  jsScanHead scan_head,
  jsCamera camera,
  double *roll_degrees,
  double *shift_x,
  double *shift_y) POST;

/**
 * @brief Configures spatial parameters of the scan head in order to properly
 * transform the data from a camera based coordinate system to one based on
 * mill placement. This function is similar to `jsScanHeadSetAlignment`
 * except that it only applies to one laser rather than all.
 *
 * @note The alignment settings are sent to the scan head during the call to
 * `jsScanSystemConnect`.
 *
 * @param scan_head Reference to scan head.
 * @param laser The laser to apply parameters to.
 * @param roll_degrees The rotation in degrees to be applied.
 * @param shift_x The shift to be applied specified in scan system units.
 * @param shift_y The shift to be applied specified in scan system units.
 * @return `0` on success, negative value mapping to `jsError` on error.
 */
EXPORTED int32_t PRE jsScanHeadSetAlignmentLaser(
  jsScanHead scan_head,
  jsLaser laser,
  double roll_degrees,
  double shift_x,
  double shift_y) POST;

/**
 * @brief Obtains the currently applied alignment settings.
 *
 * @note If configured using `jsScanHeadSetAlignment`, each laser will have
 * the same alignment settings.
 *
 * @param scan_head Reference to scan head.
 * @param Laser The laser to get settings from.
 * @param roll_degrees Variable to hold roll in degrees.
 * @param shift_x Variable to hold shift in scan system units.
 * @param shift_y Variable to hold shift in scan system units.
 * @return `0` on success, negative value mapping to `jsError` on error.
 */
EXPORTED int32_t PRE jsScanHeadGetAlignmentLaser(
  jsScanHead scan_head,
  jsLaser laser,
  double *roll_degrees,
  double *shift_x,
  double *shift_y) POST;

/**
 * @brief Sets a rectangular scan window for a scan head to restrict its
 * field of view when scanning.
 *
 * @note The window settings are sent to the scan head during the call to
 * `jsScanSystemConnect`.
 *
 * @param scan_head Reference to scan head.
 * @param window_top The top window dimension in scan system units.
 * @param window_bottom The bottom window dimension in scan system units.
 * @param window_left The left window dimension in scan system units.
 * @param window_right The right window dimension in scan system units.
 * @return `0` on success, negative value mapping to `jsError` on error.
 */
EXPORTED int32_t PRE jsScanHeadSetWindowRectangular(
  jsScanHead scan_head,
  double window_top,
  double window_bottom,
  double window_left,
  double window_right) POST;

/**
 * @brief Reads the last reported status update from a scan head.
 *
 * @param scan_head Reference to scan head.
 * @param status Pointer to be updated with status contents.
 * @return `0` on success, negative value mapping to `jsError` on error.
 */
EXPORTED int32_t PRE jsScanHeadGetStatus(
  jsScanHead scan_head,
  jsScanHeadStatus *status) POST;

/**
 * @brief Obtains the number of profiles currently available to be read out from
 * a given scan head.
 *
 * @param scan_head Reference to scan head.
 * @return The total number of profiles able to be read on success, negative
 * value `jsError` on error.
 */
EXPORTED int32_t PRE jsScanHeadGetProfilesAvailable(
  jsScanHead scan_head) POST;

/**
 * @brief Blocks until the number of requested profiles are avilable to be read
 * out from a given scan head.
 *
 * @param scan_head Reference to scan head.
 * @param count The number of profiles to wait for. Should not exceed
 * `JS_SCAN_HEAD_PROFILES_MAX`.
 * @param timeout_us Maximum amount of time to wait for in microseconds.
 * @return `0` on timeout with no profiles available, positive value indicating
 * the total number of profiles able to be read after `count` or `timeout_us` is
 * reached, or negative value `jsError` on error.
 */
EXPORTED int32_t PRE jsScanHeadWaitUntilProfilesAvailable(
  jsScanHead scan_head,
  uint32_t count,
  uint32_t timeout_us) POST;

/**
 * @brief Empties the internal client side software buffers used to store
 * profiles received from a given scan head.
 *
 * @note Under normal scanning conditions where the application consumes
 * profiles as they become available, this function will not be needed. It's
 * use is to be found in cases where the application fails to consume profiles
 * after some time and the number of buffered profiles, as indicated by the
 * `jsScanHeadGetProfilesAvailable` function becomes more than the application
 * can consume and only the most recent scan data is desired.
 *
 * @param scan_head Reference to scan head.
 * @return `0` on success, negative value mapping to `jsError` on error.
 */
EXPORTED int32_t PRE jsScanHeadClearProfiles(
  jsScanHead scan_head) POST;

/**
 * @brief Reads `jsProfile` formatted profile data from a given scan head.
 * The number of profiles returned is either the max value requested or the
 * total number of profiles ready to be read out, whichever is less.
 *
 * @param scan_head Reference to scan head.
 * @param profiles Pointer to memory to store profile data. Note, the memory
 * pointed to by `profiles` must be at least `sizeof(jsProfile) * max` in
 * total number of bytes available.
 * @param max_profiles The maximum number of profiles to read. Should not
 * exceed `JS_SCAN_HEAD_PROFILES_MAX`.
 * @return The number of profiles read on success, negative value mapping to
 * `jsError` on error.
 */
EXPORTED int32_t PRE jsScanHeadGetProfiles(
  jsScanHead scan_head,
  jsProfile *profiles,
  uint32_t max_profiles) POST;

/**
 * @brief Reads `jsRawProfile` formatted profile data from a given scan head.
 * The number of profiles returned is either the max value requested or the
 * total number of profiles ready to be read out, whichever is less.
 *
 * @param scan_head Reference to scan head.
 * @param profiles Pointer to memory to store profile data. Note, the memory
 * pointed to by `profiles` must be at least `sizeof(jsRawProfile) * max` in
 * total number of bytes available.
 * @param max_profiles The maximum number of profiles to read. Should not
 * exceed `JS_SCAN_HEAD_PROFILES_MAX`.
 * @return The number of profiles read on success, negative value mapping to
 * `jsError` on error.
 */
EXPORTED int32_t PRE jsScanHeadGetRawProfiles(
  jsScanHead scan_head,
  jsRawProfile *profiles,
  uint32_t max_profiles) POST;

/**
 * @brief Obtains a single camera profile from a scan head to be used for
 * diagnostic purposes.
 *
 * @note This function should be called after `jsScanSystemConnect()`, but
 * not while the system is set to scan by calling
 * `jsScanSystemStartScanning()`.
 *
 * @note This function will automatically select the correct camera / laser
 * pair, as used when scanning, when performing the capture.
 *
 * @note Only `JS_DIAGNOSTIC_FIXED_EXPOSURE` is supported; in a future release,
 * `JS_DIAGNOSTIC_AUTO_EXPOSURE` will be supported.
 *
 * @param scan_head Reference to scan head.
 * @param camera Camera to use for profile capture.
 * @param mode `JS_DIAGNOSTIC_FIXED_EXPOSURE` to use the laser on time and
 * camera exposure provided as function arguments, `JS_DIAGNOSTIC_AUTO_EXPOSURE`
 * to dynamically adjust camera & laser according to `jsScanHeadConfiguration`
 * provided to the `jsScanHeadSetConfiguration` function.
 * @param laser_on_time_us Time laser is on in microseconds.
 * @param camera_exposure_time_us Time camera exposes in microseconds.
 * @param profile Pointer to memory to store profile data.
 * @return `0` on success, negative value mapping to `jsError` on error.
 */
EXPORTED int32_t PRE jsScanHeadGetDiagnosticProfileCamera(
  jsScanHead scan_head,
  jsCamera camera,
  jsDiagnosticMode mode,
  uint32_t laser_on_time_us,
  uint32_t camera_exposure_time_us,
  jsRawProfile *profile) POST;

/**
 * @brief Obtains a single camera profile from a scan head to be used for
 * diagnostic purposes.
 *
 * @note This function should be called after `jsScanSystemConnect()`, but
 * not while the system is set to scan by calling
 * `jsScanSystemStartScanning()`.
 *
 * @note This function will automatically select the correct camera / laser
 * pair, as used when scanning, when performing the capture.
 *
 * @note Only `JS_DIAGNOSTIC_FIXED_EXPOSURE` is supported; in a future release,
 * `JS_DIAGNOSTIC_AUTO_EXPOSURE` will be supported.
 *
 * @param scan_head Reference to scan head.
 * @param laser Laser to use for profile capture.
 * @param mode `JS_DIAGNOSTIC_FIXED_EXPOSURE` to use the laser on time and
 * camera exposure provided as function arguments, `JS_DIAGNOSTIC_AUTO_EXPOSURE`
 * to dynamically adjust camera & laser according to `jsScanHeadConfiguration`
 * provided to the `jsScanHeadSetConfiguration` function.
 * @param laser_on_time_us Time laser is on in microseconds.
 * @param camera_exposure_time_us Time camera exposes in microseconds.
 * @param profile Pointer to memory to store profile data.
 * @return `0` on success, negative value mapping to `jsError` on error.
 */
EXPORTED int32_t PRE jsScanHeadGetDiagnosticProfileLaser(
  jsScanHead scan_head,
  jsLaser laser,
  jsDiagnosticMode mode,
  uint32_t laser_on_time_us,
  uint32_t camera_exposure_time_us,
  jsRawProfile *profile) POST;

/**
 * @brief Obtains a single camera image from a scan head to be used for
 * diagnostic purposes.
 *
 * @note This function should be called after `jsScanSystemConnect()`, but
 * not after the system has been set to scan by calling
 * `jsScanSystemStartScanning()`.
 *
 * @note This function will automatically select the correct camera / laser
 * pair, as used when scanning, when performing the capture.
 *
 * @note Only `JS_DIAGNOSTIC_FIXED_EXPOSURE` is supported; in a future release,
 * `JS_DIAGNOSTIC_AUTO_EXPOSURE` will be supported.
 *
 * @param scan_head Reference to scan head.
 * @param camera Camera to use for image capture. The laser to be in view of
 * the image will be chosen based on the chosen camera.
 * @param mode `JS_DIAGNOSTIC_FIXED_EXPOSURE` to use the laser on time and
 * camera exposure provided as function arguments, `JS_DIAGNOSTIC_AUTO_EXPOSURE`
 * to dynamically adjust camera & laser according to `jsScanHeadConfiguration`
 * provided to the `jsScanHeadSetConfiguration` function.
 * @param laser_on_time_us Time laser is on in microseconds.
 * @param camera_exposure_time_us Time camera exposes in microseconds.
 * @param image Pointer to memory to store camera image data.
 * @return `0` on success, negative value mapping to `jsError` on error.
 */
EXPORTED int32_t PRE jsScanHeadGetDiagnosticImageCamera(
  jsScanHead scan_head,
  jsCamera camera,
  jsDiagnosticMode mode,
  uint32_t laser_on_time_us,
  uint32_t camera_exposure_time_us,
  jsCameraImage *image) POST;

/**
 * @brief Obtains a single camera image from a scan head to be used for
 * diagnostic purposes.
 *
 * @note This function should be called after `jsScanSystemConnect()`, but
 * not after the system has been set to scan by calling
 * `jsScanSystemStartScanning()`.
 *
 * @note This function will automatically select the correct camera / laser
 * pair, as used when scanning, when performing the capture.
 *
 * @note Only `JS_DIAGNOSTIC_FIXED_EXPOSURE` is supported; in a future release,
 * `JS_DIAGNOSTIC_AUTO_EXPOSURE` will be supported.
 *
 * @param scan_head Reference to scan head.
 * @param laser Laser to be in view of image capture. The camera that takes the
 * image will automatically be chosen based on the chosen laser.
 * @param mode `JS_DIAGNOSTIC_FIXED_EXPOSURE` to use the laser on time and
 * camera exposure provided as function arguments, `JS_DIAGNOSTIC_AUTO_EXPOSURE`
 * to dynamically adjust camera & laser according to `jsScanHeadConfiguration`
 * provided to the `jsScanHeadSetConfiguration` function.
 * @param laser_on_time_us Time laser is on in microseconds.
 * @param camera_exposure_time_us Time camera exposes in microseconds.
 * @param image Pointer to memory to store camera image data.
 * @return `0` on success, negative value mapping to `jsError` on error.
 */
EXPORTED int32_t PRE jsScanHeadGetDiagnosticImageLaser(
  jsScanHead scan_head,
  jsLaser laser,
  jsDiagnosticMode mode,
  uint32_t laser_on_time_us,
  uint32_t camera_exposure_time_us,
  jsCameraImage *image) POST;

/**
 * @brief Obtains a single camera image from a scan head to be used for
 * diagnostic purposes.
 *
 * @note This function should be called after `jsScanSystemConnect()`, but
 * not after the system has been set to scan by calling
 * `jsScanSystemStartScanning()`.
 *
 * @note Only `JS_DIAGNOSTIC_FIXED_EXPOSURE` is supported; in a future release,
 * `JS_DIAGNOSTIC_AUTO_EXPOSURE` will be supported.
 *
 * @param scan_head Reference to scan head.
 * @param camera Camera to use for image capture.
 * @param laser Laser to be in view of image capture.
 * @param mode `JS_DIAGNOSTIC_FIXED_EXPOSURE` to use the laser on time and
 * camera exposure provided as function arguments, `JS_DIAGNOSTIC_AUTO_EXPOSURE`
 * to dynamically adjust camera & laser according to `jsScanHeadConfiguration`
 * provided to the `jsScanHeadSetConfiguration` function.
 * @param laser_on_time_us Time laser is on in microseconds.
 * @param camera_exposure_time_us Time camera exposes in microseconds.
 * @param image Pointer to memory to store camera image data.
 * @return `0` on success, negative value mapping to `jsError` on error.
 */
EXPORTED int32_t PRE jsScanHeadGetDiagnosticImage(
  jsScanHead scan_head,
  jsCamera camera,
  jsLaser laser,
  jsDiagnosticMode mode,
  uint32_t laser_on_time_us,
  uint32_t camera_exposure_time_us,
  jsCameraImage *image) POST;

#ifdef PRE
  #undef PRE
#endif

#ifdef POST
  #undef POST
#endif

#endif

#ifdef __cplusplus
} // extern "C" {
#endif

#endif
