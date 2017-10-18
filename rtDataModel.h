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
#ifndef __RT_DATAMODEL_H__
#define __RT_DATAMODEL_H__

#include <rtError.h>
#include <rtConnection.h>
#include <rtMessage.h>

typedef void* rtDataModelHandle;

#define RTDM_FIELDNAME_MAX 256
#define RTDM_TOPIC_PREFIX "RDK.MODEL."

typedef struct
{
} rtDataModelAttributeInfo;

typedef struct {
  rtError (*init)(rtDataModelHandle* handle);
  rtError (*read)(rtDataModelHandle* handle, rtMessage const req, rtMessage res);
  rtError (*write)(rtDataModelHandle* handle, rtMessage const req);
  rtError (*getattr)(rtDataModelHandle* handle, rtMessage const req, rtMessage res);
} rtDataModelOperations;

rtError rtDataModelRegisterProvider(rtConnection con, char const* object_name, rtDataModelOperations* ops);

#endif
