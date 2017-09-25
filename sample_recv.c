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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void onMessage(rtMessage m, void* closure)
{
  char* s;
  uint32_t n;

  (void) closure;

  rtMessage_ConvertToString(m, &s, &n);
  printf("new message\n");
  printf("%.*s\n", n, s);
  printf("\n");
  free(s);
}

int main()
{
  rtError err;
  rtConnection con;
  rtConnection_Create(&con, "APP2", "tcp://127.0.0.1:10001");
  rtConnection_AddListener(con, "A.*.C", onMessage, NULL);

  while ((err = rtConnection_Dispatch(con)) == RT_OK)
  {
    // nothing to do
  }

  rtConnection_Destroy(con);
  return 0;
}
