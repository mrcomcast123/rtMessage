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

#include <stdlib.h>
#include <string.h>

typedef struct
{
  rtConnection con;
  rtDataModelOperations* ops;
  rtDataModelHandle handle;
} rtDataModelProviderContext;

static void sendResponse(rtMessage const req, rtMessage res, rtDataModelProviderContext* ctx, rtError err)
{
  int32_t request_id;
  rtMessage_GetInt32(req, "id", &request_id);

  if (err != RT_OK)
  {
    rtMessage error;
    rtMessage_Create(&error);
    rtMessage_SetString(error, "message", rtStrError(err));
    rtMessage_SetInt32(error, "code", err);
    rtMessage_SetMessage(res, "error", error);
    rtMessage_SetInt32(res, "id", request_id);
    rtMessage_Destroy(error);
  }

  err = rtConnection_Send(ctx->con, res, "REPLY_TOPIC");
  if (err != RT_OK)
    rtLogWarn("failed to send response. %s", rtStrError(err));

  rtMessage_Destroy(res);
}

static void onGet(rtMessage const req, rtDataModelProviderContext* ctx)
{
  rtError err;
  rtMessage res;
  rtMessage_Create(&res);
  err = ctx->ops->read(ctx->handle, req, res);
  sendResponse(req, res, ctx, err);
}

static void onSet(rtMessage const req, rtDataModelProviderContext* ctx)
{
  rtError err;
  rtMessage res;
  rtMessage_Create(&res);
  err = ctx->ops->write(ctx->handle, req);
  sendResponse(req, res, ctx, err);
}

static void
onMessage(rtMessage req, void* closure)
{
  char method[64];
  rtError err;
  rtMessage res;
  rtDataModelProviderContext* ctx;

  memset(method, 0, sizeof(method));
  ctx = (rtDataModelProviderContext *) closure;

  err = rtMessage_Create(&res);
  if (err != RT_OK)
  {
    rtLogError("failed to create message. %s", rtStrError(err));
    return;
  }

  err = rtMessage_GetStringValue(req, "method", method, sizeof(method));
  if (err != RT_OK)
  {
    rtLogError("request doesn't include an operation. %s", rtStrError(err));
    return;
  }

  if (strcasecmp(method, "get") == 0)
    onGet(req, ctx);
  else if (strcasecmp(method, "set") == 0)
    onSet(req, ctx);
}

rtError
rtDataModelRegisterProvider(rtConnection con, char const* object_name, rtDataModelOperations* ops)
{
  rtError err;
  rtDataModelHandle handle;
  rtDataModelProviderContext* ctx;
  char subscription[RTDM_FIELDNAME_MAX];

  if (ops->init)
  {
    err = ops->init(&handle);
    if (err != RT_OK)
    {
      rtLogError("failed to initialize operations. %s", rtStrError(err));
      return RT_FAIL;
    }
  }
  else
  {
    rtLogError("invalid argument");
    return RT_ERROR_INVALID_ARG;
  }

  memset(subscription, 0, sizeof(subscription));
  strcpy(subscription, RTDM_TOPIC_PREFIX);
  strcat(subscription, object_name);
  ctx = (rtDataModelProviderContext *) malloc(sizeof(rtDataModelProviderContext));
  ctx->con = con;
  ctx->ops = ops;
  ctx->handle = handle;

  err = rtConnection_AddListener(con, subscription, onMessage, ctx);
  if (err != RT_OK)
  {
    rtLogWarn("failed to add listener for object: %s. %s", subscription, rtStrError(err));
    free(ctx);
    return RT_FAIL;
  }

  return RT_OK;
}
