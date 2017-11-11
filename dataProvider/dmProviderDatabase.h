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
#ifndef __DM_PROVIDER_DATABASE_H__
#define __DM_PROVIDER_DATABASE_H__

#include "dmProviderInfo.h"
#include "dmProviderQuery.h"
#include "dmProviderOperation.h"
#include "dmPropertyInfo.h"

#include <map>
#include <string>
#include <vector>

class dmProviderDatabase
{
public:
  dmProviderDatabase(std::string const& dir);
  std::vector<dmPropertyInfo> get(char const* query);

  dmProviderQuery* createQuery();
  dmProviderQuery* createQuery(dmProviderOperation op, char const* s);

private:
  void loadFromDir(std::string const& dir);
  void loadFile(char const* dir, char const* fname);

private:
  std::string m_dataModelDir;
  std::map<std::string, dmProviderInfo> m_providerInfo;
};

#endif
