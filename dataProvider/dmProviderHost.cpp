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

extern "C"
{
#include <rtConnection.h>
#include <rtError.h>
#include <rtLog.h>
}

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
    rtLog_Info("\nRegister for datamodel requests on:%s\n", s.c_str());

    rtError e = rtConnection_AddListener(m_con, s.c_str(),
        &dmProviderHostImpl::requestHandler, this);

    rtLog_Info("\n register provider:%s \n", rtStrError(e));

    return e == RT_OK;
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
    int id = 0;
    printf("\n requestHandler called  with request : %s \n", buff);

    if (!rtMessageHeader_IsRequest(hdr))
    {
      rtLog_Warn("got message that wasn't request in datamodel callback");
    }

    rtMessage req;
    rtError e = rtMessage_FromBytes(&req, buff, n);
    rtMessage_GetInt32(req, "id", &id);

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

    // TODO: send response
    rtMessage res;
    rtMessage_Create(&res);
    rtMessage_SetInt32(res, "id", id);
    host->encodeResult(&res, result);
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
    // TODO
    char* itemstring;
    uint32_t num;
    char* propName = new char[50];
    char* provider = new char[50];
    char* parameter = new char[50];
    dmUtility dUtil;

    dmPropertyInfo *dI = new dmPropertyInfo();

    rtMessage item;
    rtMessage_Create(&item);
    rtMessage_GetString(req, "provider", &provider);
    rtMessage_GetMessage(req, "params", &item);
    rtMessage_ToString(item, &itemstring, &num);
    rtLog_Info("\nSub item: \t%.*s", num, itemstring);

    name = strdup(provider);

    rtMessage_GetString(item, "name", &propName);
    dUtil.splitQuery(propName, provider, parameter);
    std::string property(parameter);
    dI->setName(parameter);
    params.push_back(*dI);

    delete [] propName;
    delete [] provider;
    delete [] parameter;
  }

  void decodeSetRequest(rtMessage req, std::string& name, std::vector<dmNamedValue>& params)
  {
    // TODO
  }

  void encodeResult(rtMessage* res, dmQueryResult const& result)
  {
    rtMessage_SetInt32(*res, "status", result.status());
    for (auto vIt = result.values().begin(); vIt != result.values().end(); vIt++)
    {
      dmValue temp = vIt->Value.value();
      std::string paramvalue = temp.toString();
      std::string parameter = vIt->Value.name();
      rtMessage_SetString(*res, "name", parameter.c_str());
      rtMessage_SetString(*res, "value", paramvalue.c_str());
      paramvalue.clear();
      parameter.clear();
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
dmProviderHost::registerProvider(std::string const& name, std::unique_ptr<dmProvider> provider)
{
  bool b = providerRegistered(name);
  if (b)
    m_providers.insert(std::make_pair(name, std::move(provider)));
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
