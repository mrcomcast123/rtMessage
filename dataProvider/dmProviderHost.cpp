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
#include "dmProviderHost.h"
#include "dmProvider.h"
#include "dmProviderOperation.h"
#include "dmObject.h"
#include "dmQueryResult.h"

#include <sstream>
#include <cstring>
#include <iostream>
#include <algorithm>

#include <rtConnection.h>
#include <rtError.h>
#include <rtLog.h>

#include "dmUtility.h"
#include <stdlib.h>
namespace
{

  dmNamedValue
  makeNamedValue(dmPropertyInfo const& propInfo, char const* valueAsString)
  {
    // TODO conver to propert type from string
    return dmNamedValue(propInfo, dmValue(valueAsString));
  }
}

class dmProviderHostImpl : public dmProviderHost
{
public:
  dmProviderHostImpl()
  {

  }

  virtual ~dmProviderHostImpl()
  {
    if (m_con)
    {
      rtLog_Info("closing rtMessage connection");
      rtConnection_Destroy(m_con);
      m_con = nullptr;
    }
  }

private:
  
  virtual bool providerRegistered(std::string const& name)
  {
    std::stringstream topic;
    topic << "RDK.MODEL.";
    topic << name;
    std::string s = topic.str();

    rtLog_Info("register provider with topic:%s", s.c_str());
    rtError e = rtConnection_AddListener(m_con, s.c_str(), &dmProviderHostImpl::requestHandler, this);
    if (e != RT_OK)
    {
      rtLog_Warn("failed to register provider listener. %s", rtStrError(e));
      return false;
    }

    return true;
  }

  void start()
  {
    rtConnection_Create(&m_con, "USE_UNIQUE_NAME_HERE", "tcp://127.0.0.1:10001");
    std::unique_lock<std::mutex> lock(m_mutex);
    m_thread.reset(new std::thread(&dmProviderHostImpl::run, this));
  }

  void stop()
  {
    rtConnection_Destroy(m_con);
    std::unique_lock<std::mutex> lock(m_mutex);
    if (m_thread)
    {
      // TODO
    }
  }

  void run()
  {
    while (true)
    {
      rtError e = rtConnection_Dispatch(m_con);
      if (e != RT_OK)
      {
        rtLog_Warn("error during dispatch:%s", rtStrError(e));
      }
    }
  }

  static void requestHandler(rtMessageHeader const* hdr, uint8_t const* buff, uint32_t n,
    void* closure)
  {
    if (!rtMessageHeader_IsRequest(hdr))
    {
      rtLog_Error("got message that wasn't request in datamodel callback");
      return;
    }

    rtMessage req;
    rtError e = rtMessage_FromBytes(&req, buff, n);
    rtLog_Debug("req: %s", buff);

    if (e != RT_OK)
    {
      rtLog_Warn("failed to decode datamodel request");
      // TODO: return error
    }

    dmProviderHostImpl* host = reinterpret_cast<dmProviderHostImpl *>(closure);
    if (!host)
    {
      rtLog_Error("dmProviderHost is null");
      // TODO: return error
    }

    std::vector<dmQueryResult> results;
    dmProviderOperation op = host->decodeOperation(req);

    if (op == dmProviderOperation_Get)
    {
      std::string providerName;
      std::vector<dmPropertyInfo> params;
      std::string propName;
      host->decodeGetRequest(req, providerName, params, propName);
      host->doGet(providerName, params, results, propName);
    }
    else if (op == dmProviderOperation_Set)
    {
      std::string providerName;
      std::vector<dmNamedValue> params;
      std::string propName;
      host->decodeSetRequest(req, providerName, params, propName);
      host->doSet(providerName, params, results, propName);
    }

    rtMessage res;
    rtMessage_Create(&res);
    host->encodeResult(res, results);
    rtConnection_SendResponse(m_con, hdr, res, 1000);
    rtMessage_Release(res);
  }

  dmProviderOperation decodeOperation(rtMessage req)
  {
    char const* operation = nullptr;
    rtMessage_GetString(req, "method", &operation);
    if ((strcmp(operation, "set") == 0))
      return dmProviderOperation_Set;
    else
      return dmProviderOperation_Get;
  }

  void decodeGetRequest(rtMessage req, std::string& name, std::vector<dmPropertyInfo>& params, std::string& propName)
  {
    char const* providerName = nullptr;
    dmPropertyInfo propertyInfo;

    rtMessage_GetString(req, "provider", &providerName);
    if (providerName)
      name = providerName;

    rtMessage item;
    rtMessage_GetMessage(req, "params", &item);

    char const* propertyName = nullptr;
    rtMessage_GetString(item, "name", &propertyName);
    propName = propertyName;

    rtLog_Debug("decodeGetRequest property name=%s\n", (propertyName != nullptr ? propertyName : ""));

    std::shared_ptr<dmProviderInfo> objectInfo = db->getProviderByQueryString(propertyName);
    if (objectInfo)
    {
      rtLog_Debug("decodeGetRequest object found %s", providerName);
      if(objectInfo->isList())
      {
        uint32_t index;
        std::string indexlessPropertyName;
        dmUtility::parseListProperty(propertyName, index, indexlessPropertyName);

        if (dmUtility::isWildcard(indexlessPropertyName.c_str()))
          params = objectInfo->properties();
        else
          params.push_back(objectInfo->getPropertyInfo(indexlessPropertyName.c_str()));
        
        for(int i = 0; i < (int)params.size(); ++i)
          params[i].setIndex(index-1);//index from 1 based to 0 based
      }
      else
      {
        if (dmUtility::isWildcard(propertyName))
          params = objectInfo->properties();
        else
          params.push_back(objectInfo->getPropertyInfo(propertyName));
      }
    }
    else
    {
      rtLog_Debug("decodeGetRequest object not found %s", providerName);
    }

    rtMessage_Release(item);
  }

  void decodeSetRequest(rtMessage req, std::string& name, std::vector<dmNamedValue>& params, std::string& propName)
  {
    char const* providerName = nullptr;

    rtMessage_GetString(req, "provider", &providerName);
    if (providerName)
      name = providerName;

    // TODO: need to handle multiple sets in single call

    rtMessage item;
    rtMessage_GetMessage(req, "params", &item);

    char const* propertyName = nullptr;
    rtMessage_GetString(item, "name", &propertyName);
    propName = propertyName;

    char const* value = nullptr;
    rtMessage_GetString(item, "value", &value);

    rtLog_Debug("decoderSetRequest property name=%s value=%s\n", (propertyName != nullptr ? propertyName : ""), (value != nullptr ? value : ""));

    std::shared_ptr<dmProviderInfo> objectInfo = db->getProviderByQueryString(propertyName);

    if (objectInfo)
    {
      rtLog_Debug("decodeSetRequest object found %s", propertyName);

      std::string propertyLastName;
      uint32_t index;

      if(objectInfo->isList())
      {
        std::string indexlessPropertyName;
        dmUtility::parseListProperty(propertyName, index, indexlessPropertyName);
        
        propertyLastName = dmUtility::trimPropertyName(indexlessPropertyName);
      }
      else
        propertyLastName = dmUtility::trimPropertyName(propertyName);

      std::vector<dmPropertyInfo> props = objectInfo->properties();

      auto itr = std::find_if(
        props.begin(),
        props.end(),
        [propertyLastName](dmPropertyInfo const& info) { 
          rtLog_Debug("decodeSetRequest find_if %s compare to %s = %d\n", info.name().c_str(), propertyLastName.c_str(), (int)(info.name() == propertyLastName));
          return info.name() == propertyLastName; 
        });

      if (itr != props.end())
      {
        if(objectInfo->isList())
          itr->setIndex(index-1);
        params.push_back(makeNamedValue(*itr, value));

      }
    }
    else
    {
      rtLog_Debug("decodeSetRequest object not found %s", providerName);
    }

    rtMessage_Release(item);
  }

  void encodeResult(rtMessage& res, std::vector<dmQueryResult> const& resultSet)
  {
    for (dmQueryResult const& result : resultSet)
    {
      rtMessage msg;
      rtMessage_Create(&msg);
      int statusCode = result.status();
      std::string statusMessage = result.statusMsg();

      for (dmQueryResult::Param const& param : result.values())
      {
        // I don't like how we do this. This is trying to set the overall status of the
        // request to the first error it sees. For example, if we query two params, and
        // one is ok and the other is an error, the statuscode is set to the second error
        // code. If there are two errors it'll set it to the first. We should introduce an
        // top-level error code in the response message. possibly something that indicates
        // that there's partial failure.
        if (param.StatusCode != 0 && statusCode == 0)
          statusCode = param.StatusCode;

        if (statusMessage.empty() && !param.StatusMessage.empty())
          statusMessage = param.StatusMessage;

        rtMessage_SetString(msg, "name", param.Info.fullName().c_str());
        rtMessage_SetString(msg, "value", param.Value.toString().c_str());
      }

      rtMessage_SetInt32(msg, "index", result.index());
      rtMessage_SetInt32(msg, "status", statusCode);
      rtMessage_SetString(msg, "status_msg", statusMessage.c_str());
      rtMessage_AddMessage(res, "result", msg);
      rtMessage_Release(msg);
    }
  }

private:
  std::unique_ptr<std::thread> m_thread;
  std::mutex m_mutex;
  static rtConnection m_con;
};

rtConnection dmProviderHostImpl::m_con = nullptr;

dmProviderHost*
dmProviderHost::create()
{
  rtLog_SetLevel(RT_LOG_DEBUG);
  return new dmProviderHostImpl();
}

void dmProviderHost::printTree()
{
  for(auto it = m_providers.begin(); it != m_providers.end(); ++it)
  {
    printTree(it->second->getRootObject(), 0, 0);
  }
}

void dmProviderHost::printTree(dmObject* p, int curdep, int index)
{
  if(!p)
    return;
  char buffer[200];
  sprintf(buffer, "%*.s %s(%s)", curdep, "  ", p->name().c_str(), p->m_instanceName.c_str());
  rtLog_Warn("tree(%d): %s", curdep, buffer);
  curdep++;
  for(size_t i = 0; i < p->getChildCount(); ++i)
    printTree(p->getChildByIndex(i),curdep,i);  
}

bool
dmProviderHost::addObject(std::string const& fullInstanceName, dmObject* obj)
{
  dmUtility::QueryParser qp(fullInstanceName);
  if(qp.getNodeCount() < 2)
    return false;
  std::string fullName = qp.getNode(qp.getNodeCount()-1).fullName;
  std::string parentInstanceName = qp.getNode(qp.getNodeCount()-2).instanceName;

  rtLog_Warn("addObject obj=%p instance=%s name=%s parent=%s", obj, fullInstanceName.c_str(), fullName.c_str(), parentInstanceName.c_str());

  dmObject* parent = getObject(parentInstanceName);
  rtLog_Warn("parent %p %s", parent, (parent ? parent->fullName().c_str() : "null"));

  if(parent)
  {
    std::shared_ptr<dmProviderInfo> objectInfo = db->getProviderByObjectName(fullName); 
    if (objectInfo)
    {
      obj->m_info = objectInfo;
      obj->setFullName(fullName);
      obj->m_instanceName = fullInstanceName;
      if(objectInfo->isList())
      {
        parent->addListItem(obj);
      }
      else
      {
        parent->addChild(obj);
      }
      return true;
    }
    else
    {
      rtLog_Error("Provider info not found for %s", fullInstanceName.c_str());
      return false;
    }
  }
  else
  {
    rtLog_Error("Parent not found for %s", fullInstanceName.c_str());
    return false;
  }
}

bool
dmProviderHost::registerProvider(std::string const& name, dmObject* obj)
{
  bool b = false;
  std::shared_ptr<dmProviderInfo> objectInfo = db->getProviderByObjectName(name); 
  if (objectInfo)
  {
    b = providerRegistered(objectInfo->providerName());
    if (b)
    {
      std::shared_ptr<dmProvider> provider = std::shared_ptr<dmProvider>(new dmProvider());
      provider->m_info = obj->m_info = objectInfo;
      provider->m_root = obj;
      obj->setFullName(name);
      obj->m_instanceName = name;
      m_providers.insert(std::make_pair(objectInfo->providerName(), provider));
    }
  }
  else
  {
    rtLog_Error("registerProvider failed to find info in database for object:%s", name.c_str());
  }
  return b;
}

std::shared_ptr<dmProvider> dmProviderHost::getProvider(std::string const& param) const
{ 
  dmUtility::QueryParser qp(param);
  rtLog_Debug("getProvider for %s", qp.getNodeCount() > 0 ? qp.getNode(qp.getNodeCount()-1).fullName.c_str() : "null");
  for(size_t i = 0; i < qp.getNodeCount(); ++i)
  {
    rtLog_Debug("getProvider node: %s", qp.getNode(i).fullName.c_str());
    std::shared_ptr<dmProviderInfo> objectInfo = db->getProviderByObjectName(qp.getNode(i).fullName); 
    if (objectInfo)
    {
      auto itr = m_providers.find(objectInfo->providerName());
      if (itr != m_providers.end())
      {
        rtLog_Debug("getProvider found");
        return itr->second;
      }
    }
  }
  rtLog_Debug("getProvider failed");
  return nullptr;
}

dmObject* 
dmProviderHost::getObject(std::string const& propName) const
{
  dmUtility::QueryParser qp(propName);
  std::shared_ptr<dmProvider> provider;
  size_t i;
  rtLog_Debug("getObject qp count=%d last=%s", (int)qp.getNodeCount(), (qp.getNodeCount() > 0 ? qp.getNode(qp.getNodeCount()-1).fullName.c_str() : "null"));
  for(i = 0; i < qp.getNodeCount() && !provider; ++i)
  {
    rtLog_Debug("getObject looking for root at: %s", qp.getNode(i).fullName.c_str());
    std::shared_ptr<dmProviderInfo> objectInfo = db->getProviderByObjectName(qp.getNode(i).fullName); 
    if (objectInfo)
    {
      auto itr = m_providers.find(objectInfo->providerName());
      if (itr != m_providers.end())
      {
        rtLog_Debug("getObject root found %s", itr->second->name().c_str());
        provider = itr->second;
        break;
      }
    }
    rtLog_Debug("getObject root not found");
  }

  if(!provider)
  {
    rtLog_Debug("getObject provider not found");
    return nullptr;
  }

  dmObject* obj = provider->getRootObject();
  if(!obj)
  {
    rtLog_Debug("getObject root object null");
    return nullptr;
  }
  
  for(i++;i < qp.getNodeCount(); ++i)
  {
    if(qp.getNode(i).name.empty())//wildcard
    {
      //object found so break out
      break;
    }
    //first check if its a regular parameter instead of an object or list
    dmPropertyInfo info = obj->getInfo()->getPropertyInfo(qp.getNode(i).name.c_str());
    if(info.type() != dmValueType_Unknown)
    {
      if(i < qp.getNodeCount() - 1)
      {
        rtLog_Debug("getObject found a non-tree item property before reach end of param list");
        return nullptr;
      }
      //object found so break out
      break;
    }
    rtLog_Debug("getObject searching %s for child %s", obj->fullName().c_str(), qp.getNode(i).name.c_str());
    obj = obj->getChildByName(qp.getNode(i).name.c_str());
    if(obj)
    {
      if(qp.getNode(i).index)
      {
        rtLog_Debug("getObject searching %s list for item %d", obj->fullName().c_str(), qp.getNode(i).index);
        obj = obj->getChildByIndex(qp.getNode(i).index-1);
        if(obj)
        {
          rtLog_Debug("getObject list item found");
        }
        else
        {
          rtLog_Debug("getObject list item not found");
        }
      }
    }
    else
    {
      rtLog_Debug("getObject child not");
    }
  }
  return obj;
}

void
dmProviderHost::doGet(std::string const& providerName, std::vector<dmPropertyInfo> const& params,
    std::vector<dmQueryResult>& result, std::string& propName)
{
  dmObject* obj = getObject(propName);

  if(obj)
  {
    rtLog_Debug("dmProviderHost::doGet found");
    obj->doGet(params, result);
  }
  else
  {
    rtLog_Debug("dmProviderHost::doGet not found");
  }
}

void
dmProviderHost::doSet(std::string const& providerName, std::vector<dmNamedValue> const& params,
    std::vector<dmQueryResult>& result, std::string& propName)
{
  dmObject* obj = getObject(propName);

  if(obj)
  {
    rtLog_Debug("dmProviderHost::doSet found");
    obj->doSet(params, result);
  }
  else
  {
    rtLog_Debug("dmProviderHost::doSet not found");
  }
}
