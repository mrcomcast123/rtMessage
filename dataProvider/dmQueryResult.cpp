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
#include "dmQueryResult.h"
#include "dmUtility.h"
#include <rtError.h>
#include <rtLog.h>

#include <iterator>
#include <cstring>
#include <stdio.h>
#include <iostream>
#include <string>

static const int kDefaultQueryResult = RT_OK;

dmQueryResult::dmQueryResult()
  : m_status(kDefaultQueryResult)
  , m_index(-1)
{
}

void
dmQueryResult::clear()
{
  m_status = kDefaultQueryResult;
  m_values.clear();
  m_statusMsg.clear();
  m_index = -1;
}

void
dmQueryResult::merge(dmQueryResult const& result)
{
  m_values.insert(std::end(m_values), std::begin(result.m_values), std::end(result.m_values));
  if (m_status == 0 && result.m_status != 0)
    m_status = result.m_status;
  if (!result.m_statusMsg.empty())
    setStatusMsg(result.m_statusMsg);
}

void
dmQueryResult::setStatus(int status)
{
  m_status = status;
}

void
dmQueryResult::setStatusMsg(std::string statusmsg)
{
   m_statusMsg = statusmsg;
}

void
dmQueryResult::addValue(dmPropertyInfo const& prop, dmValue const& val, int code, 
  char const* message)
{
  m_status = code;
  m_values.push_back(dmQueryResult::Param(code, message, val, prop));

  if (message)
  {
    setStatusMsg(std::string(message));
  }
}

void dmQueryResult::updateFullNames()
{
  for (auto& param : m_values)
    if(m_index > 0)
      param.fullName = dmUtility::getFullNameWithIndex(param.Info.fullName(), m_index);
    else
      param.fullName = param.Info.fullName();
}

dmQueryResult::Param::Param(int code, char const* msg, dmValue const& val, dmPropertyInfo const& info)
  : StatusCode(code)
  , Value(val)
  , Info(info)
{
  if (msg)
    StatusMessage = msg;
}
