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

#include <functional>
#include <map>
#include <vector>

class dmProvider
{
public:
  dmProvider();
  virtual ~dmProvider();

  inline std::string const& name() const
    { return m_name; }

  virtual void doGet(std::vector<dmPropertyInfo> const& params, std::vector<dmQueryResult>& result);
  virtual void doSet(std::vector<dmNamedValue> const& params, std::vector<dmQueryResult>& result);

protected:
  virtual void doGet(dmPropertyInfo const& info, dmQueryResult& result);
  virtual void doSet(dmPropertyInfo const& info, dmValue const& value, dmQueryResult& result);

  using getter_function = std::function<void (dmPropertyInfo const& info, dmQueryResult& result)>;
  using setter_function = std::function<void (dmPropertyInfo const& info, dmValue const& value, dmQueryResult& result)>;

  void onGet(std::string const& propertyName, getter_function func);
  void onSet(std::string const& propertyName, setter_function func);

  virtual size_t getListSize();
protected:
  std::string m_name;

private:
  struct provider_functions
  {
    getter_function getter;
    setter_function setter;
  };

  std::map< std::string, provider_functions > m_provider_functions;
};

#endif
