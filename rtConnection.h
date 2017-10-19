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
#ifndef __RTMSG_CONNECTION_H__
#define __RTMSG_CONNECTION_H__

#include "rtError.h"
#include "rtMessage.h"
#include "rtMessageHeader.h"

#define RTMSG_DEFAULT_ROUTER_LOCATION "tcp://127.0.0.1:10001"

struct _rtConnection;
typedef struct _rtConnection* rtConnection;

typedef void (*rtMessageCallback)(rtMessageHeader const* hdr, rtMessage m, void* closure);

typedef enum
{
  rtConnectionState_ReadHeaderPreamble,
  rtConnectionState_ReadHeader,
  rtConnectionState_ReadPayload
} rtConnectionState;

rtError
rtConnection_Create(rtConnection* con, char const* application_name, char const* router_config);

rtError
rtConnection_Destroy(rtConnection con);

rtError
rtConnection_SendMessage(rtConnection con, rtMessage msg, char const* topic);

rtError
rtConnection_SendBinary(rtConnection con, char const* topic, uint8_t const* p, uint32_t n);

rtError
rtConnection_SendRequest(rtConnection con, rtMessage const req, char const* topic,
  rtMessage* res, int32_t timeout);

rtError
rtConnection_AddListener(rtConnection con, char const* expression,
  rtMessageCallback callback, void* closure);

rtError
rtConnection_Dispatch(rtConnection con);

rtError
rtConnection_TimedDispatch(rtConnection con, int32_t timeout);

#endif
