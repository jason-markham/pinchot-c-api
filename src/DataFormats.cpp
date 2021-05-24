/**
 * Copyright (c) JoeScan Inc. All Rights Reserved.
 *
 * Licensed under the BSD 3 Clause License. See LICENSE.txt in the project
 * root for license information.
 */

#include "DataFormats.hpp"

using namespace joescan;

std::map<jsDataFormat, std::pair<DataType, std::vector<uint16_t>>>
  DataFormats::formats = {
    {JS_DATA_FORMAT_XY_FULL_LM_FULL,
     {DataType::Brightness | DataType::XYData, {1, 1}}},

    {JS_DATA_FORMAT_XY_HALF_LM_HALF,
     {DataType::Brightness | DataType::XYData, {2, 2}}},

    {JS_DATA_FORMAT_XY_QUARTER_LM_QUARTER,
     {DataType::Brightness | DataType::XYData, {4, 4}}},

    {JS_DATA_FORMAT_XY_FULL, {DataType::XYData, {1}}},

    {JS_DATA_FORMAT_XY_HALF, {DataType::XYData, {2}}},

    {JS_DATA_FORMAT_XY_QUARTER, {DataType::XYData, {4}}},

    // TODO: We don't know how these modes will be handled in C++ yet, so ignore
    // them for now
    //{ JS_DATA_FORMAT_SUBPIXEL, { DataType::Subpixel, {1} } }
};

DataType DataFormats::GetDataType(jsDataFormat format)
{
  return formats[format].first;
}

std::vector<uint16_t> DataFormats::GetStep(jsDataFormat format)
{
  return formats[format].second;
}
