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
#include "dmObject.h"

void
dmObject::doGet(std::vector<dmPropertyInfo> const& params, std::vector<dmQueryResult>& result)
{
  dmQueryResult temp;
  temp.setStatus(RT_PROP_NOT_FOUND);

  for (auto const& propInfo : params)
  {
    auto itr = m_provider_functions.find(propInfo.name());
    if ((itr != m_provider_functions.end()) && (itr->second.getter != nullptr))
    {
      itr->second.getter(propInfo, temp);
    }
    else
    {
      doGet(propInfo, temp);
    }

    if (temp.status() == RT_PROP_NOT_FOUND)
    {
      // TODO
      rtLog_Debug("property:%s not found", propInfo.name().c_str());
    }
    else
    {
      if(propInfo.index() != -1)
        temp.setIndex(propInfo.index()+1);
      result.push_back(temp);
    }

    temp.clear();
  }
}

void
dmObject::doGet(dmPropertyInfo const& /*param*/, dmQueryResult& /*result*/)
{
  // EMPTY: overridden by the user 
}

void
dmObject::doSet(std::vector<dmNamedValue> const& params, std::vector<dmQueryResult>& result)
{
  dmQueryResult temp;
  temp.setStatus(RT_PROP_NOT_FOUND);

  for (auto const& value : params)
  {
    auto itr = m_provider_functions.find(value.name());
    if ((itr != m_provider_functions.end()) && (itr->second.setter != nullptr))
    {
      itr->second.setter(value.info(), value.value(), temp);
    }
    else
    {
      doSet(value.info(), value.value(), temp);
    }

    if (temp.status() == RT_PROP_NOT_FOUND)
    {
      // TODO
      rtLog_Debug("property:%s not found", value.name().c_str());
    }
    else
    {
      if(value.info().index() != -1)
        temp.setIndex(value.info().index()+1);
      result.push_back(temp);
    }

    temp.clear();
  }
}

void
dmObject::doSet(dmPropertyInfo const& /*info*/, dmValue const& /*value*/, dmQueryResult& /*result*/)
{
  // EMPTY: overridden by the user 
}


// inserts function callback
void
dmObject::onGet(std::string const& propertyName, getter_function func)
{
  auto itr = m_provider_functions.find(propertyName);
  if (itr != m_provider_functions.end())
  {
    itr->second.getter = func;
  }
  else
  {
    provider_functions funcs;
    funcs.setter = nullptr;
    funcs.getter = func;
    m_provider_functions.insert(std::make_pair(propertyName, funcs));
  }
}

// inserts function callback
void
dmObject::onSet(std::string const& propertyName, setter_function func)
{
  auto itr = m_provider_functions.find(propertyName);
  if (itr != m_provider_functions.end())
  {
    itr->second.setter = func;
  }
  else
  {
    provider_functions funcs;
    funcs.setter = func;
    funcs.getter = nullptr;
    m_provider_functions.insert(std::make_pair(propertyName, funcs));
  }
}
