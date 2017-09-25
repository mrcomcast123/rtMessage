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
#include "rtConnection.h"
#include "rtEncoder.h"
#include "rtError.h"
#include "rtLog.h"
#include "rtMessageHeader.h"
#include "rtSocket.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define RTMSG_LISTENERS_MAX 64
#define RTMSG_SEND_BUFFER_SIZE (1024 * 4094)

static int32_t next_subscription_id = 1;
static int32_t get_next_subscription_id()
{
  return next_subscription_id++;
}

struct _rtListener
{
  int used;
  void* closure;
  char* expression;
  int32_t subscription_id;
  rtMessageCallback callback;
};

struct _rtConnection
{
  int fd;
  struct sockaddr_storage local_endpoint;
  struct sockaddr_storage remote_endpoint;

  uint8_t* send_buffer;
  uint32_t sequence_number;
  char* application_name;

  struct _rtListener listeners[RTMSG_LISTENERS_MAX];
};

static rtError rtConnection_SendInternal(rtConnection con, rtMessage msg);

rtError
rtConnection_Create(rtConnection* con, char const* application_name, char const* router_config)
{
  int                   i;
  int                   port;
  int                   ret;
  char                  addr[64];
  struct sockaddr_in*   v4;
  socklen_t             socket_length;

  port = 0;
  ret = 0;
  memset(addr, 0, sizeof(addr));
  v4 = NULL;
  socket_length = 0;
  i = 0;

  char const* p = strchr(router_config, '/');
  if (p)
    p += 2;
  else
    return RT_ERROR_INVALID_ARG;

  if (p)
  {
    char const* q = strchr(p, ':');
    if (q)
    {
      strncpy(addr, p, (q-p));
      q++;
      port = strtol(q, NULL, 10);
    }
  }
  else
  {
    return RT_ERROR_INVALID_ARG;
  }

  rtConnection c = (rtConnection) malloc(sizeof(struct _rtConnection));
  if (!c)
    return rtErrorFromErrno(ENOMEM);

  for (i = 0; i < RTMSG_LISTENERS_MAX; ++i)
  {
    c->listeners[i].used = 0;
    c->listeners[i].closure = NULL;
    c->listeners[i].callback = NULL;
    c->listeners[i].subscription_id = -1;
  }

  c->send_buffer = (uint8_t *) malloc(RTMSG_SEND_BUFFER_SIZE);
  c->sequence_number = 1;
  c->application_name = strdup(application_name);

  c->fd = socket(AF_INET, SOCK_STREAM, 0);
  fcntl(c->fd, F_SETFD, fcntl(c->fd, F_GETFD) | FD_CLOEXEC);

  memset(&c->local_endpoint, 0, sizeof(struct sockaddr_storage));
  memset(&c->remote_endpoint, 0, sizeof(struct sockaddr_storage));

  // only support v4 right now
  v4 = (struct sockaddr_in *) &c->remote_endpoint;
  ret = inet_pton(AF_INET, addr, &v4->sin_addr);
  if (ret == 1)
  {
    v4->sin_family = AF_INET;
    v4->sin_port = htons(port);
    c->local_endpoint.ss_family = AF_INET;
    socket_length = sizeof(struct sockaddr_in);
  }

  ret = connect(c->fd, (struct sockaddr *)&c->remote_endpoint, socket_length);
  if (ret == -1)
  {
    free(c);
    // error
  }

  rtSocket_GetLocalEndpoint(c->fd, &c->local_endpoint);

  {
    uint16_t local_port;
    uint16_t remote_port;
    char local_addr[64];
    char remote_addr[64];

    rtSocketStorage_ToString(&c->local_endpoint, local_addr, sizeof(local_addr), &local_port);
    rtSocketStorage_ToString(&c->remote_endpoint, remote_addr, sizeof(remote_addr), &remote_port);
    rtLogInfo("connect %s:%d -> %s:%d", local_addr, local_port, remote_addr, remote_port);
  }

  *con = c;
  return 0;
}

rtError
rtConnection_Destroy(rtConnection con)
{
  if (con)
  {
    if (con->fd != -1)
    {
      shutdown(con->fd, SHUT_RDWR);
      close(con->fd);
    }
    if (con->send_buffer)
      free(con->send_buffer);
    if (con->application_name)
      free(con->application_name);
    free(con);

  }
  return 0;
}

rtError
rtConnection_Send(rtConnection con, rtMessage msg)
{
  return rtConnection_SendInternal(con, msg);
}

rtError
rtConnection_SendInternal(rtConnection con, rtMessage msg)
{
  rtError         err;
  rtMessageHeader header;
  ssize_t         bytes_sent;
  uint8_t const*  payload;

  rtMessageHeader_Init(&header);

  err = rtMessage_GetSendTopic(msg, header.topic);
  if (err != RT_OK)
    return err;

  err = rtMessage_GetAsBytes(msg, &payload, &header.payload_length);
  if (err != RT_OK)
    return err;

  header.topic_length = strlen(header.topic);
  header.length = header.topic_length + RTMSG_HEADER_FIZED_SIZE;
  header.sequence_number = con->sequence_number++;
  header.flags = 0;

  err = rtMessageHeader_Encode(&header, con->send_buffer);
  if (err != RT_OK)
    return err;

  #if 0
  for (i = 0; i < header.length; ++i)
  {
    if (i % 16 == 0)
      printf("\n");
    printf("0x%02x ", con->send_buffer[i]);
  }
  printf("\n");
  #endif

  bytes_sent = send(con->fd, con->send_buffer, header.length, 0);
  if (bytes_sent != header.length)
  {
    if (bytes_sent == -1)
      return rtErrorFromErrno(errno);
    return RT_FAIL;
  }

  bytes_sent = send(con->fd, payload, header.payload_length, 0);
  if (bytes_sent != header.payload_length)
  {
    if (bytes_sent == -1)
      return rtErrorFromErrno(errno);
    return RT_FAIL;
  }

  return RT_OK;
}

rtError
rtConnection_AddListener(rtConnection con, char const* expression, rtMessageCallback callback, void* closure)
{
  int i;

  for (i = 0; i < RTMSG_LISTENERS_MAX; ++i)
  {
    if (!con->listeners[i].used)
      break;
  }

  if (i >= RTMSG_LISTENERS_MAX)
    return rtErrorFromErrno(ENOMEM);

  con->listeners[i].used = 1;
  con->listeners[i].subscription_id = get_next_subscription_id();
  con->listeners[i].closure = closure;
  con->listeners[i].callback = callback;
  con->listeners[i].expression = strdup(expression);

  rtMessage m;
  rtMessage_Create(&m);
  rtMessage_AddFieldString(m, "type", "subscribe");
  rtMessage_AddFieldString(m, "topic", expression);
  rtMessage_AddFieldInt32(m, "route_id", con->listeners[i].subscription_id); 
  rtConnection_SendInternal(con, m);
  rtMessage_Destroy(m);

  return 0;
}

rtError
rtConnection_Dispatch(rtConnection con)
{
  (void) con;
  // TODO
  return RT_OK;
}
