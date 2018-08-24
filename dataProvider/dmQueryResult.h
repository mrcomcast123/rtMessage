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
#ifndef __DM_QUERY_RESULT_H__
#define __DM_QUERY_RESULT_H__

#include "dmValue.h"
#include "dmPropertyInfo.h"

#include <vector>

class dmProviderDatabase;

class dmQueryResult
{
  friend class dmProviderDatabase;

public:
  struct Param
  {
    Param(int code, char const* msg, dmValue const& val, dmPropertyInfo const& info);

    int StatusCode;
    std::string StatusMessage;
    dmValue Value;
    dmPropertyInfo Info;
    std::string fullName;
  };

  dmQueryResult();

  void clear();
  void merge(dmQueryResult const& resuls);
  void setStatus(int status);
  void setStatusMsg(std::string statusmsg);
  void addValue(dmPropertyInfo const& prop, dmValue const& val, 
    int code = 0, char const* message = nullptr);

  inline int status() const
    { return m_status; }

  inline std::vector<Param> const& values() const
    { return m_values; }

  inline std::string const& statusMsg() const
    { return m_statusMsg; }

  //basic list support supporting a single list object as the last object in the param full name
  inline int const& index() const
    { return m_index; }

  inline void setIndex(int index)
    { m_index = index; }

  void updateFullNames();
private:
  int m_status;
  std::vector<Param> m_values;
  std::string m_statusMsg;
  int m_index;
};

#endif
