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

#include <rtConnection.h>
#include <rtError.h>
#include <rtLog.h>

#include "dmUtility.h"

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

    if (e != RT_OK)
    {
      rtLog_Warn("failed to decode datamodel request");
    }

    dmProviderHostImpl* host = reinterpret_cast<dmProviderHostImpl *>(closure);

    dmQueryResult result;
    dmProviderOperation op = host->decodeOperation(req);

    if (op == dmProviderOperation_Get)
    {
      std::string provider_name;
      std::vector<dmPropertyInfo> params;
      host->decodeGetRequest(req, provider_name, params);
      host->doGet(provider_name, params, result);
    }
    else if (op == dmProviderOperation_Set)
    {
      std::string provider_name;
      std::vector<dmNamedValue> params;
      host->decodeSetRequest(req, provider_name, params);
      host->doSet(provider_name, params, result);
    }

    rtMessage res;
    rtMessage_Create(&res);
    host->encodeResult(res, result);
    rtConnection_SendResponse(m_con, hdr, res, 1000);
    rtMessage_Destroy(res);
  }

  dmProviderOperation decodeOperation(rtMessage req)
  {
    // TODO:
    return dmProviderOperation_Get;
  }

  void decodeGetRequest(rtMessage req, std::string& name, std::vector<dmPropertyInfo>& params)
  {
    char const* provider_name = nullptr;
    rtMessage_GetString(req, "provider", &provider_name);
    if (provider_name)
      name = provider_name;

    rtMessage item;
    rtMessage_GetMessage(req, "params", &item);

    char const* property_name;
    rtMessage_GetString(item, "name", &property_name);

    char* param = new char[256];
    dmUtility::splitQuery(property_name, param);

    dmPropertyInfo propertyInfo;
    propertyInfo.setName(param);

    rtMessage_Destroy(item);
    delete [] param;

    params.push_back(propertyInfo);
  }

  void decodeSetRequest(rtMessage req, std::string& name, std::vector<dmNamedValue>& params)
  {
    // TODO
  }

  void encodeResult(rtMessage& res, dmQueryResult const& resultSet)
  {
    int status_code = resultSet.status();
    for (auto const& result : resultSet.values())
    {
      if (result.StatusCode != 0 && status_code == 0)
        status_code = result.StatusCode;

      dmNamedValue const& val = result.Value;
      rtMessage_SetString(res, "name", val.name().c_str());
      rtMessage_SetString(res, "value", val.value().toString().c_str());

      // TODO: currently response only handles single value, should handle list
      break;
    }

    rtMessage_SetInt32(res, "status", status_code);
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
dmProviderHost::registerProvider(std::unique_ptr<dmProvider> provider)
{
  bool b = providerRegistered(provider->name());
  if (b)
    m_providers.insert(std::make_pair(provider->name(), std::move(provider)));
  return b;
}

void
dmProviderHost::doGet(std::string const& providerName, std::vector<dmPropertyInfo> const& params,
    dmQueryResult& result)
{
  auto itr = m_providers.find(providerName);
  if (itr != m_providers.end())
  {
    itr->second->doGet(params, result);
  }
}

void
dmProviderHost::doSet(std::string const& providerName, std::vector<dmNamedValue> const& params,
    dmQueryResult& result)
{
  auto itr = m_providers.find(providerName);
  if (itr != m_providers.end())
  {
    itr->second->doSet(params, result);
  }
}
