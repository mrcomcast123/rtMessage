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

#include <unistd.h>

int main()
{
  rtLogSetLevel(RT_LOG_INFO);

  int count = 0;
  rtConnection con;
  rtConnection_Create(&con, "APP1", "tcp://127.0.0.1:10001");

  while (1)
  {
    rtMessage m;
    rtMessage_Create(&m);
    rtMessage_SetInt32(m, "field1", count++);
    rtMessage_SetString(m, "field2", "hello world");
    rtMessage item;
    rtMessage_Create(&item);
    rtMessage_SetString(item, "field3", "I am extra message");
    rtMessage_SetMessage(m, "new", item);

    rtConnection_Send(con, m, "A.B.C");
    rtLogInfo("send");

    rtMessage_Destroy(m);
    sleep(1);

    rtMessage_Create(&m);
    rtMessage_SetInt32(m, "field1", 1234);

    rtConnection_Send(con, m, "A.B.C.FOO.BAR");
    rtLogInfo("send");

    rtMessage_Destroy(m);
    sleep(1);
  }

  rtConnection_Destroy(con);
  return 0;
}

