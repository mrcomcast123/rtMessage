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
#include "dmProviderInfo.h"
#include "dmProviderDatabase.h"
#include "dmObject.h"

#include <map>
#include <vector>

class dmProvider 
{
public:
  dmProvider();
  virtual ~dmProvider();

  inline std::string const& name() const
    { return m_name; }

  dmObject* getRootObject()
  { return m_root; }

  std::shared_ptr<dmProviderInfo> getInfo()
    { return m_info; }

private:
/*
  void finalize(dmProviderDatabase* db)
  {
    m_info = db->getProviderByObjectName(m_name);
    m_root->finalize(db);
  }
*/
  std::string m_name;
  std::shared_ptr<dmProviderInfo> m_info;
  dmObject* m_root;
  friend class dmProviderHost;
};

#endif
