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
#include "rtSocket.h"
#include "rtError.h"
#include "rtLog.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

rtError
rtSocketStorge_GetLength(struct sockaddr_storage* endpoint, socklen_t* len)
{
  switch (endpoint->ss_family)
  {
    case AF_INET:
      *len = sizeof(struct sockaddr_in);
      break;
    case AF_INET6:
      *len = sizeof(struct sockaddr_in6);
      break;
    default:
      abort();
  }
  return RT_OK;
}

rtError
rtSocketStorage_ToString(struct sockaddr_storage* ss, char* buff, int n, uint16_t* port)
{
  void*         addr;
  int           family;

  *port = 0;
  addr = NULL;
  family = 0;

  if (ss->ss_family == AF_INET)
  {
    struct sockaddr_in* v4 = (struct sockaddr_in *) ss;
    addr = &v4->sin_addr;
    *port = ntohs(v4->sin_port);
    family = AF_INET;
  }

  if (addr)
    inet_ntop(family, addr, buff, n);

  return RT_OK;
}

rtError
rtSocket_GetLocalEndpoint(int fd, struct sockaddr_storage* endpoint)
{
  int         ret;
  socklen_t   len;

  ret = 0;
  len = sizeof(struct sockaddr_in);
  memset(endpoint, 0, sizeof(struct sockaddr_storage));

  if ((ret = getsockname(fd, (struct sockaddr *)endpoint, &len)) == -1)
    rtLogError("getsockname:%s", rtStrError(errno));

  return RT_OK;
}
