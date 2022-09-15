#include <cmath>

#include "PhaseTable.hpp"
#include "ScanHead.hpp"

using namespace joescan;

PhaseTable::PhaseTable()
{
  Reset();
}

PhaseTableCalculated PhaseTable::CalculatePhaseTable()
{
  PhaseTableCalculated table_calculated;

  // build up the initial calculated phase table using the user data; set the
  // duration for each phase to be the longest laser on time per phase
  for (auto &phased_elements_vector : m_table) {
    PhaseTableEntry entry;

    for (auto &element : phased_elements_vector) {
      if (!element.is_cfg_unique) {
        // load the configuration dynamically, don't know when user changed it
        element.cfg = element.scan_head->GetConfiguration();
      }

      if (element.cfg.laser_on_time_max_us > entry.duration_us) {
        entry.duration_us = element.cfg.laser_on_time_max_us;
      }
      entry.elements.push_back(element);
    }

    table_calculated.phases.push_back(entry);
  }

  // NOTE: we're working with the calculated phase table object in code below

  // calculate the real time per phase and for the entire phase table by
  // looking at the scanning limitations dictated by the scan window.

  // this calculation loop works by tracking the amount of time has elapsed
  // since a given camera has been "seen"; there must be more time since the
  // last time it was seen and can be used again than is required for the
  // camera to fully readout the data from a previous scan.

  // we'll need to perform  this calculation loop twice; once for the the first
  // application on window constraints and a second time to handle window
  // constraints that wrap back around to the beginning of the phase table.
  const uint32_t num_calculation_iterations = 2;
  // variable to track the last time a given camera was seen in calculation
  std::map<std::pair<ScanHead *, jsCamera>, uint32_t> accum;

  // cameras require some time before they can be used for scanning again
  const double kRowTimeNs = 3210.0;
  const double kOverheadRows = 42;
  const double kSafetyMarginRows = 3;
  const uint32_t frame_overhead_time_us = static_cast<uint32_t>(
    std::ceil(kRowTimeNs * (4 + kOverheadRows + kSafetyMarginRows) / 1000.0));

  for (uint32_t n = 0; n < num_calculation_iterations; n++) {
    for (auto &phase : table_calculated.phases) {
      // extend accumulator for cameras previously seen
      for (auto &map : accum) {
        map.second += phase.duration_us;
      }

      for (auto &element : phase.elements) {
        ScanHead *scan_head = element.scan_head;
        jsCamera camera = element.camera;
        std::pair<ScanHead *, jsCamera> key(scan_head, camera);
        // the minimum scan period is driven by the readout time that a given
        // camera takes to process all of the columns inside the scan window

        if (accum.find(key) != accum.end()) {
          uint32_t min_scan_period_us = scan_head->GetMinScanPeriod();
          uint32_t laser_on_max_us = element.cfg.laser_on_time_max_us;
          int32_t last_seen_us = accum[key];
          int32_t adj_min_period_us = 0;
          int32_t adj_fot_us = 0;
          int32_t adj = 0;

          // time required to read out from camera; affected by window
          adj_min_period_us = min_scan_period_us - last_seen_us;
          // overhead time required between scans on same camera
          adj_fot_us =
            frame_overhead_time_us - (last_seen_us - laser_on_max_us);

          // the parentheses prevent the `max` macro invocation which gets
          // defined when building on windows
          adj = (std::max)(adj_min_period_us, adj_fot_us);

          if (0 < adj) {
            phase.duration_us += adj;
            // add time to all accumulators since the phase has increased
            for (auto &map : accum) {
              map.second += adj;
            }
          }
        }
        // reset accumulator for this camera since it's been seen
        accum[key] = 0;
      }
    }
  }

  // calculate the total duration of the entire phase table
  for (auto &phase : table_calculated.phases) {
    table_calculated.total_duration_us += phase.duration_us;
  }

  return table_calculated;
}

uint32_t PhaseTable::GetNumberOfPhases()
{
  return static_cast<uint32_t>(m_table.size());
}

void PhaseTable::Reset()
{
  m_table.clear();
  m_scan_head_count.clear();
}

void PhaseTable::CreatePhase()
{
  std::vector<PhasedElement> phase;
  m_table.push_back(phase);
}

int PhaseTable::AddToLastPhaseEntry(ScanHead *scan_head, jsCamera camera,
                                    jsScanHeadConfiguration *cfg)
{
  if (0 == m_table.size()) {
    return JS_ERROR_INVALID_ARGUMENT;
  }

  uint32_t phase = static_cast<uint32_t>(m_table.size()) - 1;
  jsLaser laser = scan_head->GetPairedLaser(camera);
  if (JS_LASER_INVALID == laser) {
    return JS_ERROR_INVALID_ARGUMENT;
  }

  return AddToPhaseEntryCommon(phase, scan_head, camera, laser, cfg);
}

int PhaseTable::AddToLastPhaseEntry(ScanHead *scan_head, jsLaser laser,
                                    jsScanHeadConfiguration *cfg)
{
  if (0 == m_table.size()) {
    return JS_ERROR_INVALID_ARGUMENT;
  }

  uint32_t phase = static_cast<uint32_t>(m_table.size()) - 1;
  jsCamera camera = scan_head->GetPairedCamera(laser);
  if (JS_CAMERA_INVALID == camera) {
    return JS_ERROR_INVALID_ARGUMENT;
  }

  return AddToPhaseEntryCommon(phase, scan_head, camera, laser, cfg);
}

int PhaseTable::AddToPhaseEntryCommon(uint32_t phase, ScanHead *scan_head,
                                      jsCamera camera, jsLaser laser,
                                      jsScanHeadConfiguration *cfg)
{
  if (m_table.size() <= phase) {
    return JS_ERROR_INVALID_ARGUMENT;
  }

  if (m_scan_head_count.find(scan_head) != m_scan_head_count.end()) {
    if (m_scan_head_count[scan_head] >= scan_head->GetMaxScanPairs()) {
      return JS_ERROR_NO_MORE_ROOM;
    }
  } else {
    // first time scan head has been entered into phase table
    m_scan_head_count[scan_head] = 0;
  }

  for (auto &el : m_table[phase]) {
    if ((el.scan_head == scan_head) && (el.camera == camera)) {
      return JS_ERROR_INVALID_ARGUMENT;
    }
  }

  m_scan_head_count[scan_head] += 1;

  PhasedElement el;
  el.scan_head = scan_head;
  el.camera = camera;
  el.laser = laser;

  if (nullptr == cfg) {
    el.is_cfg_unique = false;
  } else {
    if (!(scan_head->IsConfigurationValid(*cfg))) {
      return JS_ERROR_INVALID_ARGUMENT;
    }

    el.is_cfg_unique = true;
    el.cfg = *cfg;
  }

  m_table[phase].push_back(el);

  return 0;
}
