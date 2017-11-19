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
#include "rtError.h"
#include "rtMessage.h"

#include <cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

struct _rtMessage
{
  cJSON* json;
};

/**
 * Allocate storage and initializes it as new message
 * @param pointer to the new message
 * @return rtError
 **/
rtError
rtMessage_Create(rtMessage* message)
{
  *message = (rtMessage) malloc(sizeof(struct _rtMessage));
  if (message)
  {
    (*message)->json = cJSON_CreateObject();
    return RT_OK;
  }
  return RT_FAIL;
}

/**
 * Copies only the data within the fields of the message
 * @param message to be copied
 * @param pointer to new copy of the message
 * @return rtError
 */
rtError
rtMessage_Clone(rtMessage const message, rtMessage* copy)
{
  *copy = (rtMessage) malloc(sizeof(struct _rtMessage));
  if (copy)
  {
    (*copy)->json = cJSON_Duplicate(message->json, 1);
    return RT_OK;
  }
  return RT_FAIL;
}

/* Allocates storage and initializes it as new message
 * @param pointer to the new message
 * @param fill the new message with this data
 * @return rtError
 **/
rtError
rtMessage_FromBytes(rtMessage* message, uint8_t const* bytes, int n)
{
  (void) n;

  #if 0
  printf("------------------------------------------\n")
  for (i = 0; i < 256; ++i)
  {
    if (i > 0)
      printf(" ");
    if (i % 16 == 0)
      printf("\n");
    printf("0x%02x", bytes[i]);
  }
  printf("\n\n");
  #endif

  *message = (rtMessage) malloc(sizeof(struct _rtMessage));
  if (message)
  {
    (*message)->json = cJSON_Parse((char *) bytes);
    return RT_OK;
  }
  return RT_FAIL;
}

/**
 * Destroy a message; free the storage that it occupies.
 * @param pointer to message to be destroyed
 * @return rtError
 **/
rtError
rtMessage_Destroy(rtMessage message)
{
  if (message)
  {
    if (message->json)
      cJSON_Delete(message->json);
    free(message);
    return RT_OK;
  }
  return RT_FAIL;
}

/**
 * Extract the data from a message as a byte sequence.
 * @param extract the data bytes from this message.
 * @param pointer to the byte sequence location 
 * @param pointer to number of bytes in the message
 * @return rtErro
 **/
rtError
rtMessage_ToByteArray(rtMessage message, uint8_t** buff, uint32_t *n)
{
  return rtMessage_ToString(message, (char **) buff, n);
}

/**
 * Format message as string
 * @param message to be converted to string
 * @param pointer to a string where message is to be stored
 * @param  pointer to number of bytes in the message
 * @return rtStatus
 **/
rtError
rtMessage_ToString(rtMessage const m, char** s, uint32_t* n)
{
  *s = cJSON_PrintUnformatted(m->json);
  *n = strlen(*s);
  return RT_OK;
}

/**
 * Add string field to the message
 * @param message to be modified
 * @param name of the field to be added
 * @param value of the field to be added
 * @return void
 **/
void
rtMessage_SetString(rtMessage message, char const* name, char const* value)
{
   cJSON_AddItemToObject(message->json, name, cJSON_CreateString(value));
}

/**
 * Add integer field to the message
 * @param message to be modified
 * @param name of the field to be added
 * @param integer value of the field to be added
 * @return void
 **/
void
rtMessage_SetInt32(rtMessage message, char const* name, int32_t value)
{
  cJSON_AddNumberToObject(message->json, name, value); 
}

/**
 * Add double field to the message
 * @param message to be modified
 * @param name of the field to be added
 * @param double value of the field to be added
 * @return void
 **/
void
rtMessage_SetDouble(rtMessage message, char const* name, double value)
{
  cJSON_AddItemToObject(message->json, name, cJSON_CreateNumber(value));
}

/**
 * Add sub message field to the message
 * @param message to be modified
 * @param name of the field to be added
 * @param new message item to be added
 * @return rtError
 **/
rtError
rtMessage_SetMessage(rtMessage message, char const* name, rtMessage item)
{
  if (!message || !item)
    return RT_ERROR_INVALID_ARG;
  if (item->json)
  {
    cJSON* obj = cJSON_Duplicate(item->json, 1);
    cJSON_AddItemToObject(message->json, name, obj);
  }
  return RT_OK;
}

/**
 * Get field value of type string using field name.
 * @param message to get field
 * @param name of the field
 * @param pointer to string value obtained.
 * @return rtError
 **/
rtError
rtMessage_GetString(rtMessage const  message, const char* name, char** value)
{
  cJSON* p = cJSON_GetObjectItem(message->json, name);
  if (p)
  {
    *value = p->valuestring;
    return RT_OK;
  }
  return RT_FAIL;
}

/**
 * Get field value of type string using field name.
 * @param message to get field
 * @param name of the field
 * @param pointer to string value obtained.
 * @param size of value obtained
 * @return rtError
 **/
rtError
rtMessage_GetStringValue(rtMessage const message, char const* name, char* fieldvalue, int n)
{
  cJSON* p = cJSON_GetObjectItem(message->json, name);
  if (p)
  {
    char *value = p->valuestring;
    int const len = (int) strlen(value);
    if (len <= n)
    {
      snprintf(fieldvalue, n, "%s", value);
      return RT_OK;
    }
    return RT_FAIL;
  }
  return RT_FAIL;
}

/**
 * Get field value of type integer using field name.
 * @param message to get field
 * @param name of the field
 * @param pointer to integer value obtained.
 * @return rtError
 **/
rtError
rtMessage_GetInt32(rtMessage const message,const char* name, int32_t* value)
{  
  cJSON* p = cJSON_GetObjectItem(message->json, name);
  if (p)
  {
    *value = p->valueint;
    return RT_OK;
  }
  return RT_FAIL;
}

/**
 * Get field value of type double using field name.
 * @param message to get field
 * @param name of the field
 * @param pointer to double value obtained.
 * @return rtError
 **/
rtError
rtMessage_GetDouble(rtMessage const  message,const char* name,double* value)
{
  cJSON* p = cJSON_GetObjectItem(message->json, name);
  if (p)
  {
    *value = p->valuedouble;
    return RT_OK;
  }
  return RT_FAIL;
}

/**
 * Get field value of type message using name
 * @param message to get field
 * @param name of the field
 * @param message obtained
 * @return rtError
 **/
rtError
rtMessage_GetMessage(rtMessage const message, char const* name, rtMessage* clone)
{
  *clone = (rtMessage) malloc(sizeof(struct _rtMessage));

  cJSON* p = cJSON_GetObjectItem(message->json, name);
  if (p)
  {
     (*clone)->json = cJSON_Duplicate(p, cJSON_True);
     return RT_OK;
  }
  return RT_FAIL;
}

/**
 * Get topic of message to be sent
 * @param message to get topic
 * @param name of the topic
 * @return rtError
 **/
rtError
rtMessage_GetSendTopic(rtMessage const m, char* topic)
{
  rtError err = RT_OK;
  cJSON* obj = cJSON_GetObjectItem(m->json, "_topic");
  if (obj)
    strcpy(topic, obj->valuestring);
  else
    err = RT_FAIL;
  return err;
}

/**
 * Set topic of message to be sent
 * @param message to set topic
 * @param name of the topic
 * @return rtError
 **/
rtError
rtMessage_SetSendTopic(rtMessage const m, char const* topic)
{
  cJSON* obj = cJSON_GetObjectItem(m->json, "_topic");
  if (obj)
    cJSON_ReplaceItemInObject(m->json, "_topic", cJSON_CreateString(topic));
  else
    cJSON_AddItemToObject(m->json, "_topic", cJSON_CreateString(topic));
  if (obj)
    cJSON_Delete(obj);
  return RT_OK;
}

rtError
rtMessage_AddString(rtMessage m, char const* name, char const* value)
{
  cJSON* obj = cJSON_GetObjectItem(m->json, name);
  if (!obj)
  {
    obj = cJSON_CreateArray();
    cJSON_AddItemToObject(m->json, name, obj);
  }
  cJSON_AddItemToArray(obj, cJSON_CreateString(value));
  return RT_OK;
}

rtError
rtMessage_GetArrayLength(rtMessage const m, char const* name, int32_t* length)
{
  cJSON* obj = cJSON_GetObjectItem(m->json, name);
  if (!obj)
    *length = 0;
  else
    *length = cJSON_GetArraySize(obj);
  return RT_OK;
}

rtError
rtMessage_GetStringItem(rtMessage const m, char const* name, int32_t idx, char* value, int len)
{
  cJSON* obj = cJSON_GetObjectItem(m->json, name);
  if (!obj)
    return RT_PROPERTY_NOT_FOUND;
  if (idx >= cJSON_GetArraySize(obj))
    return RT_FAIL;

  cJSON* item = cJSON_GetArrayItem(obj, idx);
  if (item)
  {
    strncpy(value, item->valuestring, len);
  }
  return RT_OK;
}
