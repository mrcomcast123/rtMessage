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
#include "rtMessage.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void onMessage(rtMessageHeader const* hdr, uint8_t const* buff, uint32_t n, void* closure)
{
  char* s;
  char* itemstring;
  uint32_t num;

  (void) closure;

  rtMessage m;
  rtMessage_FromBytes(&m, buff, n);

  rtMessage item;
  rtMessage_Create(&item);
  rtMessage_GetMessage(m, "new", &item);
  rtMessage_ToString(item, &itemstring, &num);
  rtLogInfo("\nSub item: \t%.*s", num, itemstring);
  rtMessage_ToString(m, &s, &n);

  rtLogInfo("\nTOPIC:%s", hdr->topic);
  rtLogInfo("\t%.*s", n, s);

  free(s);
  free(itemstring);
  rtMessage_Destroy(m);
}

int main()
{
  rtError err;
  rtConnection con;

  rtLogSetLevel(RT_LOG_INFO);
  rtConnection_Create(&con, "APP2", "tcp://127.0.0.1:10001");
  rtConnection_AddListener(con, "A.*.C", onMessage, NULL);
  rtConnection_AddListener(con, "A.B.C.>", onMessage, NULL);

  while (1)
  {
    err = rtConnection_Dispatch(con);
    rtLogInfo("dispatch:%s", rtStrError(err));
    if (err != RT_OK)
      sleep(1);
  }

  rtConnection_Destroy(con);
  return 0;
}
