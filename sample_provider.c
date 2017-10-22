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
#include "rtDataModel.h"
#include "rtLog.h"

// requests will look like json-rpc
// http://www.jsonrpc.org/specification#response_object
rtError
myProvider_Init(rtDataModelHandle* handle)
{
  (void) handle;
  return RT_OK;
}

rtError
myProvider_Read(rtDataModelHandle* handle, rtMessage const req, rtMessage res)
{
  (void) handle;
  (void) req;
  (void) res;
  return RT_OK;
}

rtError
myProvider_Write(rtDataModelHandle* handle, rtMessage const req)
{
  (void) handle;
  (void) req;
  return RT_OK;
}

rtError
myProvider_GetAttr(rtDataModelHandle* handle, rtMessage const req, rtMessage res)
{
  (void) handle;
  (void) req;
  (void) res;
  return RT_OK;
}

int main()
{
  rtError err;
  rtConnection con;
  rtDataModelOperations ops;
  ops.init = myProvider_Init;
  ops.read = myProvider_Read;
  ops.write = myProvider_Write;
  ops.getattr = myProvider_GetAttr;

  rtLog_SetLevel(RT_LOG_INFO);
  rtConnection_Create(&con, "APP2", "tcp://127.0.0.1:10001");

  // rtConnection_AddListener(con, "A.*.C", onMessage, NULL);
  // rtConnection_AddListener(con, "A.B.C.>", onMessage, NULL);
  rtDataModelRegisterProvider(con, "Device.DeviceInfo", &ops);
  rtDataModelRegisterProvider(con, "Device.DeviceInfo.ProcessStatus.Process", &ops);

  while ((err = rtConnection_Dispatch(con)) == RT_OK)
    rtLog_Info("dispatch:%s", rtStrError(err));

  rtConnection_Destroy(con);
  return 0;
}
