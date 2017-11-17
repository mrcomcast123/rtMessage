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
#include <cstring>

#include "dmProvider.h"

dmProvider::dmProvider()
{
}

dmProvider::~dmProvider()
{
}

void
dmProvider::doGet(std::vector<dmPropertyInfo> const& params, dmQueryResult& result)
{
  dmQueryResult temp;
  for (auto const& propInfo : params)
  {
    doGet(propInfo, temp);
    result.merge(temp);
    temp.clear();
  }
}

void
dmProvider::doGet(dmPropertyInfo const& param, dmQueryResult& result)
{

}

void
dmProvider::doSet(std::vector<dmNamedValue> const& params, dmQueryResult& result)
{
  dmQueryResult temp;
  for (auto const& value : params)
  {
    doSet(value, temp);
    result.merge(temp);
    temp.clear();
  }
}

void
dmProvider::doSet(dmNamedValue const& param, dmQueryResult& result)
{

}
