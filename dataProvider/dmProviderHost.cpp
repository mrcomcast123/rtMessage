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

#include <sstream>
#include <cstring>
#include <iostream>
#include <algorithm>

#include <rtConnection.h>
#include <rtError.h>
#include <rtLog.h>

#include "dmUtility.h"

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
      host->decodeGetRequest(req, providerName, params);
      host->doGet(providerName, params, results);
    }
    else if (op == dmProviderOperation_Set)
    {
      std::string providerName;
      std::vector<dmNamedValue> params;
      host->decodeSetRequest(req, providerName, params);
      host->doSet(providerName, params, results);
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

  void decodeGetRequest(rtMessage req, std::string& name, std::vector<dmPropertyInfo>& params)
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

    rtLog_Debug("decodeGetRequest property name=%s\n", (propertyName != nullptr ? propertyName : ""));

    std::shared_ptr<dmProviderInfo> objectInfo = db->getProviderByName(providerName);
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

  void decodeSetRequest(rtMessage req, std::string& name, std::vector<dmNamedValue>& params)
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

    char const* value = nullptr;
    rtMessage_GetString(item, "value", &value);

    rtLog_Debug("decoderSetRequest property name=%s value=%s\n", (propertyName != nullptr ? propertyName : ""), (value != nullptr ? value : ""));
    std::shared_ptr<dmProviderInfo> objectInfo = db->getProviderByName(providerName);

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

bool
dmProviderHost::registerProvider(char const* object, std::unique_ptr<dmProvider> provider)
{
  bool b = false;
  std::shared_ptr<dmProviderInfo> objectInfo = db->getProviderByObjectName(object); 
  if (objectInfo)
  {
    b = providerRegistered(objectInfo->providerName());
    if (b)
      m_providers.insert(std::make_pair(objectInfo->providerName(), std::move(provider)));
  }
  else
  {
    rtLog_Error("failed to find provider info in database for object:%s", object);
  }
  return b;
}

void
dmProviderHost::doGet(std::string const& providerName, std::vector<dmPropertyInfo> const& params,
    std::vector<dmQueryResult>& result)
{
  auto itr = m_providers.find(providerName);
  if (itr != m_providers.end())
  {
    rtLog_Debug("dmProviderHost::doGet %s found", providerName.c_str());
    itr->second->doGet(params, result);
  }
  else
  {
    rtLog_Debug("dmProviderHost::doGet %s not found", providerName.c_str());
  }
}

void
dmProviderHost::doSet(std::string const& providerName, std::vector<dmNamedValue> const& params,
    std::vector<dmQueryResult>& result)
{
  auto itr = m_providers.find(providerName);
  if (itr != m_providers.end())
  {
    rtLog_Debug("dmProviderHost::doSet %s found", providerName.c_str());
    itr->second->doSet(params, result);
  }
  else
  {
    rtLog_Debug("dmProviderHost::doSet %s not found", providerName.c_str());
  }
}
