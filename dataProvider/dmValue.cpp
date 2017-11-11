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

#include "dmValue.h"
#include <sstream>

dmValue::dmValue(std::string const& s)
  : m_type(dmValueType_String)
  , m_string(s) { }

dmValue::dmValue(char const* s)
  : m_type(dmValueType_String)
  , m_string(s) { }

dmValue::dmValue(int8_t n)
  : m_type(dmValueType_Int8)
  , m_value(n) { }

dmValue::dmValue(int16_t n)
  : m_type(dmValueType_Int16)
  , m_value(n) { }

dmValue::dmValue(int32_t n)
  : m_type(dmValueType_Int32)
  , m_value(n) { }

dmValue::dmValue(int64_t n)
  : m_type(dmValueType_Int64)
  , m_value(n) { }


dmValue::dmValue(uint8_t n)
  : m_type(dmValueType_UInt8)
  , m_value(n) { }

dmValue::dmValue(uint16_t n)
  : m_type(dmValueType_UInt16)
  , m_value(n) { }

dmValue::dmValue(uint32_t n)
  : m_type(dmValueType_UInt32)
  , m_value(n) { }

dmValue::dmValue(uint64_t n)
  : m_type(dmValueType_UInt64)
  , m_value(n) { }

dmValue::dmValue(float f)
  : m_type(dmValueType_Single)
  , m_value(f) { }

dmValue::dmValue(double d)
  : m_type(dmValueType_Double)
  , m_value(d) { }

std::string
dmValue::toString() const
{
  std::stringstream buff;
  switch (m_type)
  {
    case dmValueType_String:
      buff << m_string;
      break;
    case dmValueType_Int8:
      buff << m_value.int8Value;
      break;
    case dmValueType_Int16:
      buff << m_value.int16Value;
      break;
    case dmValueType_Int32:
      buff << m_value.int32Value;
      break;
    case dmValueType_Int64:
      buff << m_value.int64Value;
      break;
    case dmValueType_UInt8:
      buff << m_value.uint8Value;
      break;
    case dmValueType_UInt16:
      buff << m_value.uint16Value;
      break;
    case dmValueType_UInt32:
      buff << m_value.uint32Value;
      break;
    case dmValueType_UInt64:
      buff << m_value.uint64Value;
      break;
    case dmValueType_Single:
      buff << m_value.singleValue;
      break;
    case dmValueType_Double:
      buff << m_value.doubleValue;
      break;
  }
  return buff.str();
}
