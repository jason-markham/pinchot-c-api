/**
 * Copyright (c) JoeScan Inc. All Rights Reserved.
 *
 * Licensed under the BSD 3 Clause License. See LICENSE.txt in the project
 * root for license information.
 */

#ifndef JOESCAN_IMAGE_REQUEST_MESSAGE_H
#define JOESCAN_IMAGE_REQUEST_MESSAGE_H


#include "ScanRequestMessage.hpp"

namespace joescan {

class ImageRequest : public ScanRequest {
 public:
  ImageRequest(uint32_t client_ip, int client_port, int scan_head_id,
               uint32_t interval, uint32_t num_cameras,
               const jsScanHeadConfiguration &config);
};
}
#endif
