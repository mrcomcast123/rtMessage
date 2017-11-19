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
#ifndef __DM_PROVIDER_H__
#define __DM_PROVIDER_H__

#include "dmPropertyInfo.h"
#include "dmQueryResult.h"
#include <vector>

class dmProvider
{
public:
  dmProvider(std::string const& name);
  virtual ~dmProvider();

  inline std::string const& name() const
    { return m_name; }

  virtual void doGet(std::vector<dmPropertyInfo> const& params, dmQueryResult& result);
  virtual void doSet(std::vector<dmNamedValue> const& params, dmQueryResult& result);

protected:
  virtual void doGet(dmPropertyInfo const& param, dmQueryResult& result);
  virtual void doSet(dmNamedValue const& param, dmQueryResult& result);

protected:
  std::string m_name;
};

#endif
