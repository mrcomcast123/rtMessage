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

#include <map>
#include <memory>
#include <string>
#include <vector>
#include <cJSON.h>

#include "dmValue.h"

class dmPropertyInfo
{
public:
  std::string const& name() const;
  void setName(std::string const& name);
  // other stuff like type, min/max, etc
  // leave that out for now.
public:
  std::string m_propname;
};

class dmProviderInfo
{
public:
  std::string const& name() const;
  std::vector<dmPropertyInfo> const& properties() const;

  void setName(std::string const& name);
  void setProperties(std::vector<dmPropertyInfo> const& property);
public:
  std::string m_provider;
  std::vector<dmPropertyInfo> m_propertyInfo;
};


// make this a singleton
class dmProviderDatabase
{
public:
  dmProviderDatabase(std::string const& dir);
  ~dmProviderDatabase();
  void loadFromDir(std::string const& dir);
  void loadFile(char const* dir, char const* fname);

public:
  // if query is a complete object name
  // Device.DeviceInfo, then it'll return a vector of length 1
  // if query it wildcard, then it'll return a vector > 1
  std::vector<dmPropertyInfo> get(char const* query);
private:
  std::string m_directory;
  std::map<std::string, dmProviderInfo> providerDetails;
};

#endif
