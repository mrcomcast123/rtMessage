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

typedef void (*rtMessageCallback)(rtMessageHeader const* hdr, uint8_t const* buff,
  uint32_t n, void* closure);

typedef enum
{
  rtConnectionState_ReadHeaderPreamble,
  rtConnectionState_ReadHeader,
  rtConnectionState_ReadPayload
} rtConnectionState;

/**
 * Creates an rtConnection
 * @param con
 * @param application_name
 * @param router_config
 * @return error
 */
rtError
rtConnection_Create(rtConnection* con, char const* application_name, char const* router_config);

/**
 * Destroy an rtConnection
 * @param con
 * @return error
 */
rtError
rtConnection_Destroy(rtConnection con);

/**
 * Sends a message
 * @param con
 * @param msg
 * @param topic
 * @return error
 */
rtError
rtConnection_SendMessage(rtConnection con, rtMessage msg, char const* topic);

/**
 * Sends a binary payload
 * @param con
 * @param topic
 * @param pointer to buffer
 * @param length of buffer
 * @return error
 */
rtError
rtConnection_SendBinary(rtConnection con, char const* topic, uint8_t const* p, uint32_t n);

/**
 * Sends a request and receive a response
 * @param con
 * @param req
 * @param topic
 * @param response
 * @param timeout
 * @return error
 */
rtError
rtConnection_SendRequest(rtConnection con, rtMessage const req, char const* topic,
  rtMessage* res, int32_t timeout);

/**
 * Register a callback for message receipt
 * @param con
 * @param topic expression
 * @param callback handler
 * @param closure
 * @return error
 */
rtError
rtConnection_AddListener(rtConnection con, char const* expression,
  rtMessageCallback callback, void* closure);

/**
 * Dispatch incoming messages
 * @param con
 * @return error
 */
rtError
rtConnection_Dispatch(rtConnection con);

/**
 * Dispatch with a timeout
 * @param con
 * @param timeout
 * return error
 */
rtError
rtConnection_TimedDispatch(rtConnection con, int32_t timeout);

#endif
