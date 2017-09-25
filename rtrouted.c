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
#include "rtLog.h"
#include "rtEncoder.h"
#include "rtError.h"
#include "rtMessageHeader.h"
#include "rtSocket.h"

#include <arpa/inet.h>
#include <errno.h>
#include <getopt.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define RTMSG_MAX_CONNECTED_CLIENTS 64
#define RTMSG_MAX_ROUTES (RTMSG_MAX_CONNECTED_CLIENTS * 8)
#define RTMSG_CLIENT_MAX_TOPICS 64
#define RTMSG_CLIENT_READ_BUFFER_SIZE (1024 * 8)
#define RTMSG_MAX_LISTENERS 4
#define RTMSG_INVALID_FD -1
#define RTMSG_MAX_EXPRESSION_LEN 128

typedef struct
{
  int                       fd;
  struct sockaddr_storage   endpoint;
  uint8_t*                  read_buffer;
  rtConnectionState         state;
  int                       bytes_read;
  int                       bytes_to_read;
  rtMessageHeader           header;
} rtConnectedClient;

typedef struct
{
  void* closure;
  rtError (*message_handler)(rtMessageHeader* hdr, uint8_t* buff, int n);
  char expression[RTMSG_MAX_EXPRESSION_LEN];
  int in_use;
} rtRouteEntry;

typedef struct
{
  int fd;
  int set;
  struct sockaddr_storage endpoint;
} rtListener;

rtConnectedClient clients[RTMSG_MAX_CONNECTED_CLIENTS];
rtListener        listeners[RTMSG_MAX_LISTENERS];
rtRouteEntry      routes[RTMSG_MAX_ROUTES];

static rtError 
rtRouted_PrintMessage(rtMessageHeader* hdr, uint8_t* buff, int n)
{
  (void) hdr;
  printf("message debug\n");
  printf("'%.*s'\n", n, (char *) buff);
  return RT_OK;
}

static int
rtRouted_IsTopicMatch(char const* topic, char const* exp)
{
  char const* t = topic;
  char const* e = exp;

  while (*t && *e)
  {
    if (*e == '*')
    {
      while (*t && *t != '.')
        t++;
      e++;
    }

    if (*e == '>')
    {
      while (*t)
        t++;
      e++;
    }

    if (!(*t || *e))
      break;

    if (*t != *e)
      break;

    t++;
    e++;
  }

  return !(*t || *e);
}

static void
rtConnectedClient_Init(rtConnectedClient* clnt, int fd, struct sockaddr_storage* remote_endpoint)
{
  clnt->fd = fd;
  clnt->state = rtConnectionState_ReadHeaderPreamble;
  clnt->bytes_read = 0;
  clnt->bytes_to_read = 4;
  clnt->read_buffer = (uint8_t *) malloc(RTMSG_CLIENT_READ_BUFFER_SIZE);
  memcpy(&clnt->endpoint, remote_endpoint, sizeof(struct sockaddr_storage));
}

static void
rtRouter_DispatchMessage(rtMessageHeader* hdr, uint8_t* buff, int n)
{
  int i;
  
  for (i = 0; i < RTMSG_MAX_ROUTES; ++i)
  {
    if (!routes[i].in_use)
      break;

    if (rtRouted_IsTopicMatch(hdr->topic, routes[i].expression))
      routes[i].message_handler(hdr, buff, n);
  }
}

static void
rtConnectedClient_Destroy(rtConnectedClient* clnt)
{
  close(clnt->fd);
  clnt->fd = RTMSG_INVALID_FD;
  if (clnt->read_buffer)
    free(clnt->read_buffer);

#if 0
  for (i = 0; i < RTMSG_CLIENT_MAX_TOPICS; ++i)
  {
    if (clnt->subscriptions[i].topic)
      free(clnt->subscriptions[i].topic);
    clnt->subscriptions[i].id = -1;
    clnt->subscriptions[i].topic = NULL;
  }
#endif

}

static void
rtConnectedClient_Read(rtConnectedClient* clnt)
{
  ssize_t bytes_read;
  int bytes_to_read = (clnt->bytes_to_read - clnt->bytes_read);

  bytes_read = read(clnt->fd, &clnt->read_buffer[clnt->bytes_read], bytes_to_read);
  if (bytes_read == -1)
  {
    rtError e = rtErrorFromErrno(errno);
    rtLogWarn("read:%s", rtStrError(e));
    return;
  }

  if (bytes_read == 0)
  {
    rtConnectedClient_Destroy(clnt);
    return;
  }

  clnt->bytes_read += bytes_read;

  switch (clnt->state)
  {
    case rtConnectionState_ReadHeaderPreamble:
    {
      // read version/length of header
      if (clnt->bytes_read == clnt->bytes_to_read)
      {
        uint8_t* itr = &clnt->read_buffer[2];
        uint16_t header_length = 0;
        rtEncoder_DecodeUInt16(&itr, &header_length);
        clnt->bytes_to_read += (header_length - 4);
        clnt->state = rtConnectionState_ReadHeader;
      }
    }
    break;

    case rtConnectionState_ReadHeader:
    {
      if (clnt->bytes_read == clnt->bytes_to_read)
      {
        rtMessageHeader_Decode(&clnt->header, clnt->read_buffer);
        clnt->bytes_to_read += clnt->header.payload_length;
        clnt->state = rtConnectionState_ReadPayload;
      }
    }
    break;

    case rtConnectionState_ReadPayload:
    {
      if (clnt->bytes_read == clnt->bytes_to_read)
      {
        rtRouter_DispatchMessage(&clnt->header, clnt->read_buffer + clnt->header.length,
          clnt->header.payload_length);
        clnt->bytes_to_read = 4;
        clnt->bytes_read = 0;
        clnt->state = rtConnectionState_ReadHeaderPreamble;
      }
    }
    break;
  }
}

static void
rtRouted_PushFd(fd_set* fds, int fd, int* maxFd)
{
  if (fd != RTMSG_INVALID_FD)
  {
    FD_SET(fd, fds);
    if (maxFd && fd > *maxFd)
      *maxFd = fd;
  }
}

static void
rtRouted_RegisterNewClient(int fd, struct sockaddr_storage* remote_endpoint)
{
  int i;
  char remote_address[64];
  uint16_t remote_port;

  i = 0;
  remote_address[0] = '\0';
  remote_port = 0;

  for (i = 0; i < RTMSG_MAX_CONNECTED_CLIENTS; ++i)
  {
    if (clients[i].fd == RTMSG_INVALID_FD)
      break;
  }

  if (i >= RTMSG_MAX_CONNECTED_CLIENTS)
  {
    rtLogError("no more free client slots available");
    return;
  }

  rtConnectedClient_Init(&clients[i], fd, remote_endpoint);

  rtSocketStorage_ToString(&clients[i].endpoint, remote_address, sizeof(remote_address), &remote_port);
  rtLogInfo("new client connection from %s:%d", remote_address, remote_port);
}

static void
rtRouted_AcceptClientConnection(rtListener* listener)
{
  int                       fd;
  socklen_t                 socket_length;
  struct sockaddr_storage   remote_endpoint;

  socket_length = sizeof(struct sockaddr_storage);
  memset(&remote_endpoint, 0, sizeof(struct sockaddr_storage));

  fd = accept(listener->fd, (struct sockaddr *)&remote_endpoint, &socket_length);
  if (fd == -1)
  {
    rtLogWarn("accept:%s", rtStrError(errno));
    return;
  }

  rtRouted_RegisterNewClient(fd, &remote_endpoint);

}

static void
rtRouted_BindListener(char const* addr, uint16_t port, int noDelay)
{
  int             i;
  int             ret;
  socklen_t       socketLength;
  rtListener*     listener;

  rtLogInfo("binding listener to %s:%d", addr, port);

  listener = NULL;
  for (i = 0; i < RTMSG_MAX_LISTENERS; ++i)
  {
    if (!listeners[i].set)
    {
      listener = &listeners[i];
      break;
    }
  }

  if (!listener)
  {
    abort();
  }

  struct sockaddr_in* listenEndpoint4 = (struct sockaddr_in *) &listener->endpoint;

  listener->fd = socket(AF_INET, SOCK_STREAM, 0);
  if (listener->fd == -1)
  {
    rtLogFatal("socket:%s", rtStrError(errno));
    exit(1);
  }

  if (noDelay)
  {
    uint32_t one = 1;
    setsockopt(listeners[0].fd, SOL_TCP, TCP_NODELAY, &one, sizeof(one));
  }

  // TODO: handle v6
  ret = inet_pton(AF_INET, addr, &listenEndpoint4->sin_addr);
  if (ret == -1)
  {
    rtLogWarn("failed to parse address '%s'. %s", addr, rtStrError(errno));
    exit(1);
  }
  listenEndpoint4->sin_family = AF_INET;
  listenEndpoint4->sin_port = htons(port);
  listener->endpoint.ss_family = AF_INET;
  socketLength = sizeof(struct sockaddr_in);

  {
    int reuse = 1;
    ret = setsockopt(listener->fd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse));
    if (ret == -1)
    {
      rtLogWarn("failed to set reuse addr on sockt. %s", rtStrError(errno));
      exit(1);
    }
  }

  ret = bind(listener->fd, (struct sockaddr *)&listener->endpoint, socketLength);
  if (ret == -1)
  {
    rtLogWarn("failed to bind socket. %s", rtStrError(errno));
    exit(1);
  }

  ret = listen(listener->fd, 4);
  if (ret == -1)
  {
    rtLogWarn("failed to set socket to listen mode. %s", rtStrError(errno));
    exit(1);
  }

  listener->set = 1;
}

int main(int argc, char* argv[])
{
  int c;
  int i;
  int run_in_foreground;
  int use_no_delay;
  int ret;
  int port;

  run_in_foreground = 0;
  use_no_delay = 0;
  ret = 0;
  port = 10001;

  for (i = 0; i < RTMSG_MAX_CONNECTED_CLIENTS; ++i)
  {
    clients[i].fd = RTMSG_INVALID_FD;
    memset(&clients[i].endpoint, 0, sizeof(struct sockaddr_storage));
  }

  for (i = 0; i < RTMSG_MAX_ROUTES; ++i)
  {
    routes[i].closure = NULL;
    routes[i].message_handler = NULL;
    routes[i].expression[0] = '\0';
    routes[i].in_use = 0;
  }

  for (i = 0; i < RTMSG_MAX_LISTENERS; ++i)
  {
    listeners[i].set = 0;
    listeners[i].fd = RTMSG_INVALID_FD;
    memset(&listeners[i].endpoint, 0, sizeof(struct sockaddr_storage));
  }

  rtLogSetLevel(RT_LOG_DEBUG);
  rtLogSetLogHandler(NULL);

  while (1)
  {
    int option_index = 0;
    static struct option long_options[] = 
    {
      {"foreground",  no_argument,        0, 'f'},
      {"no-delay",    no_argument,        0, 'd' },
      {"log-level",   required_argument,  0, 'l' },
      {"debug-route", no_argument,        0, 'r' },
      {0, 0, 0, 0}
    };

    c = getopt_long(argc, argv, "dfl:r", long_options, &option_index);
    if (c == -1)
      break;

    switch (c)
    {
      case 'd':
        use_no_delay = 0;
        break;
      case 'f':
        run_in_foreground = 1;
        break;
      case 'l':
        rtLogSetLevel(rtLogLevelFromString(optarg));
        break;
      case 'r':
      {
        routes[0].closure = NULL;
        routes[0].in_use = 1;
        strcpy(routes[0].expression, ">");
        routes[0].message_handler = &rtRouted_PrintMessage;
      }
      case '?':
        break;
      default:
        fprintf(stderr, "?? getopt returned character code 0%o ??\n", c);
    }
  }

  if (!run_in_foreground)
  {
    ret = daemon(0 /*chdir to "/"*/, 0 /*redirect stdout/stderr to /dev/null*/ );
    rtLogFatal("failed to fork off daemon. %s", rtStrError(errno));
  }
  else
  {
    rtLogInfo("running in foreground");
  }

  rtRouted_BindListener("127.0.0.1", port, use_no_delay);

  while (1)
  {
    int                         max_fd;
    fd_set                      read_fds;
    fd_set                      err_fds;
    struct timeval              timeout;

    max_fd= -1;
    FD_ZERO(&read_fds);
    FD_ZERO(&err_fds);
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    for (i = 0; i < RTMSG_MAX_LISTENERS; ++i)
    {
      rtRouted_PushFd(&read_fds, listeners[i].fd, &max_fd);
      rtRouted_PushFd(&err_fds, listeners[i].fd, &max_fd);
    }

    for (i = 0; i < RTMSG_MAX_CONNECTED_CLIENTS; ++i)
    {
      rtRouted_PushFd(&read_fds, clients[i].fd, &max_fd);
      rtRouted_PushFd(&err_fds, clients[i].fd, &max_fd);
    }

    ret = select(max_fd + 1, &read_fds, NULL, &err_fds, &timeout);
    if (ret == 0)
      continue;

    if (ret == -1)
    {
      rtLogWarn("select:%s", rtStrError(errno));
      continue;
    }

    for (i = 0; i < RTMSG_MAX_LISTENERS; ++i)
    {
      if (listeners[i].fd == RTMSG_INVALID_FD)
        continue;

      if (FD_ISSET(listeners[0].fd, &read_fds))
        rtRouted_AcceptClientConnection(&listeners[0]);
    }

    for (i = 0;  i < RTMSG_MAX_CONNECTED_CLIENTS; ++i)
    {
      if (clients[i].fd == RTMSG_INVALID_FD)
        continue;

      if (FD_ISSET(clients[i].fd, &read_fds))
        rtConnectedClient_Read(&clients[i]);
    }
  }

  return 0;
}
