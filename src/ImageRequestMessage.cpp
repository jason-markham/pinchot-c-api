#include "ImageRequestMessage.hpp"

using namespace joescan;

ImageRequest::ImageRequest(uint32_t client_ip, int client_port,
                           int scan_head_id, uint32_t interval,
                           uint32_t num_cameras,
                           const jsScanHeadConfiguration &config)
{
  m_client_ip = client_ip;
  m_client_port = client_port;
  m_scan_head_id = scan_head_id;
  m_camera_id = 0; // TODO: If these become useful, don't hardcode.
  m_laser_id = 0;  // TODO: If these become useful, don't hardcode.
  m_flags = 0;     // TODO: If these become useful, don't hardcode.
  m_magic = kCommandMagic;
  m_laser_exposure_min_us = config.laser_on_time_min_us;
  m_laser_exposure_def_us = config.laser_on_time_def_us;
  m_laser_exposure_max_us = config.laser_on_time_max_us;
  m_camera_exposure_min_us = config.camera_exposure_time_min_us;
  m_camera_exposure_def_us = config.camera_exposure_time_def_us;
  m_camera_exposure_max_us = config.camera_exposure_time_max_us;
  m_laser_detection_threshold = config.laser_detection_threshold;
  m_saturation_threshold = config.saturation_threshold;
  m_saturation_percentage = config.saturation_percentage;
  // TODO: what to do with this?
  m_average_intensity = 50;
  m_scan_interval_us = interval;
  m_scan_offset_us = config.scan_offset_us;
  // scan long enough to grab an image from each camera
  m_number_of_scans = num_cameras;
  m_start_col = 0;
  m_end_col = 1455;

  m_data_types = DataType::Image;
  m_steps.push_back(1);
}
