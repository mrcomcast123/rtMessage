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
#ifndef __RT_MESSAGING_H__
#define __RT_MESSAGING_H__

#include "rtError.h"

#ifdef __cplusplus
extern "C" {
#endif

struct _rtMessage;
typedef struct _rtMessage* rtMessage;

/**
 * Allocate storage and initializes it as new message
 * @param pointer to the new message
 * @return rtError
 **/
rtError
rtMessage_Create(rtMessage* message);

/**
 * Copies only the data within the fields of the message
 * @param message to be copied
 * @param pointer to new copy of the message
 * @return rtError
 */
rtError
rtMessage_Clone(const rtMessage message, rtMessage* copy);

/* Allocates storage and initializes it as new message
 * @param pointer to the new message
 * @param fill the new message with this data
 * @return rtError
 **/
rtError
rtMessage_FromBytes(rtMessage* message, uint8_t const* buff, int n);

/**
 * Destroy a message; free the storage that it occupies.
 * @param pointer to message to be destroyed
 * @return rtError
 **/
rtError
rtMessage_Destroy(rtMessage message);

/**
 * Extract the data from a message as a byte sequence.
 * @param extract the data bytes from this message.
 * @param pointer to the byte sequence location
 * @param pointer to number of bytes in the message
 * @return rtError
 **/
rtError
rtMessage_ToByteArray(rtMessage message, uint8_t** buff, uint32_t* n);

/**
 * Add string field to the message
 * @param message to be modified
 * @param name of the field to be added
 * @param value of the field to be added
 * @return void
 **/
void
rtMessage_SetString(rtMessage message, char const* name, char const* value);

rtError
rtMessage_AddString(rtMessage message, char const* name, char const* value);

rtError
rtMessage_GetArrayLength(rtMessage const m, char const* name, int32_t* length);

rtError
rtMessage_GetStringItem(rtMessage const m, char const* name, int32_t idx, char* value, int len);


/**
 * Add integer field to the message
 * @param message to be modified
 * @param name of the field to be added
 * @param integer value of the field to be added
 * @return void
 **/
void
rtMessage_SetInt32(rtMessage message, char const* name, int32_t value);

/**
 * Add double field to the message
 * @param message to be modified
 * @param name of the field to be added
 * @param double value of the field to be added
 * @return void
 **/
void
rtMessage_SetDouble(rtMessage message, char const* name, double value);

/**
 * Add sub message field to the message
 * @param message to be modified
 * @param name of the field to be added
 * @param new message item to be added
 * @return rtError
 **/
rtError
rtMessage_SetMessage(rtMessage message, char const* name, rtMessage item);

/**
 * Get field value of type string using field name.
 * @param message to get field
 * @param name of the field
 * @param pointer to string value obtained.
 * @return rtError
 **/
rtError
rtMessage_GetString(rtMessage const m, char const* name, char** value);

/**
 * Get field value of type string using field name.
 * @param message to get field
 * @param name of the field
 * @param pointer to string value obtained.
 * @param size of value obtained
 * @return rtError
 **/
rtError
rtMessage_GetStringValue(rtMessage const m, char const* name, char* value, int n);

/**
 * Get field value of type integer using field name.
 * @param message to get field
 * @param name of the field
 * @param pointer to integer value obtained.
 * @return rtError
 **/
rtError
rtMessage_GetInt32(rtMessage const m, char const* name, int32_t* value);

/**
 * Get field value of type double using field name.
 * @param message to get field
 * @param name of the field
 * @param pointer to double value obtained.
 * @return rtError
 **/
rtError
rtMessage_GetDouble(rtMessage const m, char const* name, double* value);

/**
 * Get field value of type message using name
 * @param message to get field
 * @param name of the field
 * @param message obtained
 * @return rtError
 **/
rtError
rtMessage_GetMessage(rtMessage const m, char const* name, rtMessage* item);

/**
 * Format a message as string
 * @param message to be formatted
 * @param pointer to a string where message is to be stored
 * @param string length
 * @return rtError
 **/
rtError
rtMessage_ToString(rtMessage m, char** s, uint32_t* n);

/**
 * Get topic of message to be sent
 * @param message to get topic
 * @param name of the topic
 * @return rtError
 **/
rtError
rtMessage_GetSendTopic(rtMessage const m, char* topic);

/**
 * Set topic of message to be sent
 * @param message to set topic
 * @param name of the topic
 * @return rtError
 **/
rtError
rtMessage_SetSendTopic(rtMessage m, char const* topic);

#ifdef __cplusplus
}
#endif
#endif
