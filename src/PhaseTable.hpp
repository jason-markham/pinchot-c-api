#ifndef _PHASE_TABLE_HPP
#define _PHASE_TABLE_HPP

/**
 * Copyright (c) JoeScan Inc. All Rights Reserved.
 *
 * Licensed under the BSD 3 Clause License. See LICENSE.txt in the project
 * root for license information.
 */
#include <map>
#include <memory>
#include <vector>
#include "joescan_pinchot.h"

namespace joescan {
class ScanHead;

struct PhasedElement {
  ScanHead *scan_head = nullptr;
  jsCamera camera = JS_CAMERA_INVALID;
  jsLaser laser = JS_LASER_INVALID;
  jsScanHeadConfiguration cfg;
  bool is_cfg_unique;
};

struct PhaseTableEntry {
  uint32_t duration_us = 0;
  std::vector<PhasedElement> elements;
};

struct PhaseTableCalculated {
  uint32_t total_duration_us = 0;
  std::vector<PhaseTableEntry> phases;
};

class PhaseTable {
 public:
  PhaseTable();

  PhaseTableCalculated CalculatePhaseTable();
  uint32_t GetNumberOfPhases();
  void Reset();
  void CreatePhase();
  int AddToLastPhaseEntry(ScanHead *scan_head, jsCamera camera,
                          jsScanHeadConfiguration *cfg = nullptr);
  int AddToLastPhaseEntry(ScanHead *scan_head, jsLaser laser,
                          jsScanHeadConfiguration *cfg = nullptr);

 private:
  int AddToPhaseEntryCommon(uint32_t phase, ScanHead *scan_head,
                            jsCamera camera, jsLaser laser,
                            jsScanHeadConfiguration *cfg);

  std::vector<std::vector<PhasedElement>> m_table;
  std::map<ScanHead*, uint32_t> m_scan_head_count;
};
}

#endif
