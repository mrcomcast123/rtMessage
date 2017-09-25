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
#include "rtMessage.h"

#include <unistd.h>

int main()
{
  rtConnection con;
  rtConnection_Create(&con, "APP1", "tcp://127.0.0.1:10001");

  while (1)
  {
    rtMessage m;
    rtMessage_Create(&m);
    rtMessage_AddFieldInt32(m, "field1", 1);
    rtMessage_AddFieldInt32(m, "field2", 2);
    rtMessage_AddFieldString(m, "field3", "hello world");
    rtMessage_SetSendTopic(m, "A.B.C");
    rtConnection_Send(con, m);
    rtMessage_Destroy(m);
    sleep(1);
  }

  rtConnection_Destroy(con);
  return 0;
}

