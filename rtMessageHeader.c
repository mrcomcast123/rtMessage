/* Copyright [2017] [Comcast, Corp.]
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "rtMessageHeader.h"
#include "rtEncoder.h"

#include <string.h>

#define RTMSG_HEADER_VERSION 1

rtError
rtMessageHeader_Init(rtMessageHeader* hdr)
{
  hdr->version = RTMSG_HEADER_VERSION;
  hdr->length = 0;
  hdr->sequence_number = 0;
  hdr->flags = 0;
  hdr->payload_length = 0;
  hdr->topic_length = 0;
  memset(hdr->topic, 0, RTMSG_HEADER_MAX_TOPIC_LENGTH);
  return RT_OK;
}

rtError
rtMessageHeader_Encode(rtMessageHeader* hdr, uint8_t* buff)
{
  uint8_t* ptr = buff;
  rtEncoder_EncodeUInt16(&ptr, RTMSG_HEADER_VERSION);
  rtEncoder_EncodeUInt16(&ptr, hdr->topic_length + RTMSG_HEADER_FIZED_SIZE); 
  rtEncoder_EncodeInt32(&ptr, hdr->sequence_number); // con->sequence_number++);
  rtEncoder_EncodeInt32(&ptr, hdr->flags);
  rtEncoder_EncodeInt32(&ptr, hdr->control_data);
  rtEncoder_EncodeInt32(&ptr, hdr->payload_length);
  rtEncoder_EncodeString(&ptr, hdr->topic, NULL);
  return RT_OK;
}

rtError
rtMessageHeader_Decode(rtMessageHeader* hdr, uint8_t const* buff)
{
  uint8_t const* ptr = buff;
  rtEncoder_DecodeUInt16(&ptr, &hdr->version);
  rtEncoder_DecodeUInt16(&ptr, &hdr->length);
  rtEncoder_DecodeUInt32(&ptr, &hdr->sequence_number);
  rtEncoder_DecodeUInt32(&ptr, &hdr->flags);
  rtEncoder_DecodeUInt32(&ptr, &hdr->control_data);
  rtEncoder_DecodeUInt32(&ptr, &hdr->payload_length);
  rtEncoder_DecodeString(&ptr, hdr->topic, &hdr->topic_length);
  return RT_OK;
}
