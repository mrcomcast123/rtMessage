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
#ifndef __DM_OBJECT_H__
#define __DM_OBJECT_H__

#include <string>
#include <vector>
#include <map>
#include <memory>
#include "rtError.h"
#include "dmProviderInfo.h"
#include "dmUtility.h"
#include "dmQueryResult.h"
#include "dmProviderDatabase.h"

typedef dmProviderInfo dmObjectInfo;

class dmObject
{
public:
  dmObject()
  {
  }

  std::string& fullName()
  {
    return m_fullName;
  }

  std::string& name()
  {
    return m_name;
  }

  std::shared_ptr<dmObjectInfo> getInfo()
  {
    return m_info;
  }

  dmObject* getParent()
  {
    return m_parent;
  }

  void setParent(dmObject* parent)
  {
    m_parent = parent;
  }

  dmObject* getChildByIndex(uint32_t index)
  {
    //rtLog_Debug("getChildByIndex(%p) %u on parent %s with %d children", this, index, m_name.c_str(), (int)m_children.size());
    if(index < m_children.size())
      return m_children[index];
    rtLog_Warn("index %d out of range", index);
    return nullptr;
  }

  dmObject* getChildByName(const char* name)
  {
    //rtLog_Debug("getChildByName(%p) %s on parent %s with %d children", this, name, m_name.c_str(), (int)m_children.size());
    for(size_t i = 0; i < m_children.size(); ++i)
    {
      rtLog_Debug("getChildByName(%p) child: %s", this, m_children[i]->name().c_str());
      if(m_children[i]->name() == name)
        return m_children[i];
    }
    rtLog_Warn("child %s not found", name);
    return nullptr;
  }

  size_t getChildCount()
  {
    return m_children.size();
  }

  void addChild(dmObject* child)
  {
    rtLog_Debug("addChild(%p) %s to parent %s", this, child->name().c_str(), m_name.c_str());
    m_children.push_back(child);
    child->setParent(this);
  }  

  void addListItem(dmObject* item)
  {
    rtLog_Debug("addListItem(%p) %s to parent %s\n", this, item->name().c_str(), m_name.c_str());
    std::string listName = item->name();
    dmObject* plist = getChildByName(listName.c_str());
    if(!plist)
    {
      plist = new dmObject();
      plist->setFullName(item->fullName());
      plist->m_instanceName = m_instanceName + "." + listName;
      addChild(plist);
    }
    plist->addChild(item); 
  }

  virtual void doGet(std::vector<dmPropertyInfo> const& params, std::vector<dmQueryResult>& result);
  virtual void doSet(std::vector<dmNamedValue> const& params, std::vector<dmQueryResult>& result);

protected:
  virtual void doGet(dmPropertyInfo const& info, dmQueryResult& result);
  virtual void doSet(dmPropertyInfo const& info, dmValue const& value, dmQueryResult& result);

  using getter_function = std::function<void (dmPropertyInfo const& info, dmQueryResult& result)>;
  using setter_function = std::function<void (dmPropertyInfo const& info, dmValue const& value, dmQueryResult& result)>;

  void onGet(std::string const& propertyName, getter_function func);
  void onSet(std::string const& propertyName, setter_function func);

  
private:
  void setFullName(std::string const& fullName)
  {
    m_fullName = fullName;
    m_name = dmUtility::trimPropertyName(fullName);
  }
/*
  void finalize(dmProviderDatabase* db)
  {
    rtLog_Warn("dmObject::finalize for %s", m_id.c_str());
    m_info = db->getProviderByObjectName(m_id);
    for(size_t i = 0; i < m_children.size(); ++i)
       m_children[i]->finalize(db);
  }
*/
  std::string m_instanceName;
  std::string m_fullName;//the path of the object info, e.g. Device.WiFi or Device.WiFi.EndPoint (??? should we include the list index ???)
  std::string m_name;//the last name, e.g. Wifi or EndPoint
  std::shared_ptr<dmObjectInfo> m_info;
  dmObject* m_parent;
  std::vector<dmObject*> m_children;

  struct provider_functions
  {
    getter_function getter;
    setter_function setter;
  };
  std::map< std::string, provider_functions > m_provider_functions;

  friend class dmProviderHost;
  friend class dmProvider;
};

#endif
