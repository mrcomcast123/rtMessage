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
#include "dmProviderDatabase.h"
#include "dmQuery.h"

#include <iostream>
#include <dirent.h>
#include <cstddef>
#include <string>
#include <cstring>
#include <map>
#include <iterator>
#include <fstream>
#include <iomanip>


#include "rtConnection.h"
#include "rtMessage.h"
#include "rtError.h"
#include "rtLog.h"

#include <cJSON.h>

#include "dmUtility.h"

namespace
{
  std::map<std::string, cJSON*> modelInfoDB;

  bool matches_object(char const* query, char const* obj)
  {
    char const* p = strrchr(query, '.');
    int n = static_cast<int>((intptr_t) p - (intptr_t) query);
    return strncmp(query, obj, n) == 0;
  }
}

class dmQueryImpl : public dmQuery
{
public:
  dmQueryImpl()
    : m_provider_name(nullptr)
  {
    rtConnection_Create(&m_con, "DMCLI", "tcp://127.0.0.1:10001");
  }

  ~dmQueryImpl()
  {
    rtConnection_Destroy(m_con);
  }

  virtual bool exec()
  {
    if (!m_provider_name)
    {
      rtLog_Warn("trying to execute a query withtout a provider");
      return false;
    }

    std::string topic("RDK.MODEL.");
    topic += m_provider_name;

    rtLog_Debug("sending dm query on topic:%s", topic.c_str());

    rtMessage req;
    rtMessage_Create(&req);
//    rtMessage_SetInt32(req, "id", m_count);
    rtMessage_SetString(req, "method", m_operation.c_str());
    rtMessage_SetString(req, "provider", m_provider_name);

    rtMessage item;
    rtMessage_Create(&item);
    rtMessage_SetString(item, "name", m_query.c_str());

    if (strcmp(m_operation.c_str(), "set") == 0)
      rtMessage_SetString(item, "value", m_value.c_str());
    rtMessage_SetMessage(req, "params", item);

    rtMessage res;
    rtError err = rtConnection_SendRequest(m_con, req, topic.c_str(), &res, 2000);

    if (err == RT_OK)
    {
      int status;
      char const* param = nullptr;
      char const* value = nullptr;

      if (rtMessage_GetString(res, "name", &param) != RT_OK)
        rtLog_Error("failed to get 'name' from paramter");
      if (rtMessage_GetString(res, "value", &value) != RT_OK)
        rtLog_Error("failed to get 'value' from parameter");
      if (rtMessage_GetInt32(res, "status", &status) != RT_OK)
        rtLog_Error("failed to get 'status' from response");

      if (param != nullptr && value != nullptr)
        m_results.addValue(dmNamedValue(param, dmValue(value)), status);

      rtMessage_Destroy(res);
    }

    rtMessage_Destroy(req);
    reset();

    return true;
  }

  virtual void reset()
  {
    m_operation.clear();
    m_query.clear();
    m_provider_name = nullptr;
    m_value.clear();
    // m_count = 0;
  }

  virtual bool setQueryString(dmProviderOperation op, char const* s)
  {
    switch (op)
    {
      case dmProviderOperation_Get:
      {
        m_operation = "get";
        m_query = s;
      }
      break;
      case dmProviderOperation_Set:
      {
        m_operation = "set";
        std::string data(s);
        if (data.find("=") != std::string::npos)
        {
          std::size_t position = data.find("=");
          m_query = data.substr(0, position);
          m_value = data.substr(position+1);
        }
        else
        {
          rtLog_Error("SET operation without value");
          return false;
        }
        break;
      }
    }
    return true;
  }

  virtual dmQueryResult const& results()
  {
    return m_results;
  }

  void setProviderName(char const* provider_name)
  {
    m_provider_name = provider_name;
  }

private:
  rtConnection m_con;
  dmQueryResult m_results;
  std::string m_query;
  std::string m_value;
  std::string m_operation;
  char const* m_provider_name;

  // TODO: int m_count;
  // should use static counter or other way to inject json-rpc request/id
};

dmProviderDatabase::dmProviderDatabase(std::string const& dir)
  : m_modelDirectory(dir)
{
  loadFromDir(dir);
}

void
dmProviderDatabase::loadFromDir(std::string const& dirname)
{
  DIR* dir;
  struct dirent *ent;

  rtLog_Debug("loading database from directory:%s", dirname.c_str());
  if ((dir = opendir(dirname.c_str())) != NULL)
  {
    while ((ent = readdir(dir)) != NULL)
    {
      if (strcmp(ent->d_name,".") != 0 && strcmp(ent->d_name,"..") != 0)
        loadFile(dirname.c_str(), ent->d_name);
    }
    closedir(dir);
  } 
  else
  {
    rtLog_Warn("failed to open directory:%s. %s", dirname.c_str(),
      strerror(errno));
  }
}

void
dmProviderDatabase::loadFile(std::string const& dir, char const* fname)
{
  std::string path = dir;
  path += "/";
  path += fname;

  rtLog_Debug("load file:%s", path.c_str());

  std::ifstream file;
  file.open(path.c_str(), std::ifstream::in);
  if (!file.is_open())
  {
    rtLog_Warn("failed to open fie. %s. %s", fname, strerror(errno));
    return;
  }

  file.seekg(0, file.end);
  int length = file.tellg();
  file.seekg(0, file.beg);

  std::vector<char> buff(length + 1, '\0');
  file.read(buff.data(), length);

  cJSON* json = cJSON_Parse(buff.data());
  if (!json)
  {
    rtLog_Error("Failed to parse json from:%s. %s", path.c_str(), cJSON_GetErrorPtr());
  }
  else
  {
    cJSON* name = cJSON_GetObjectItem(json, "name");
    modelInfoDB.insert(std::make_pair(name->valuestring, json));
  }
}

char const*
dmProviderDatabase::getProvider(char const* query) const
{
  for (auto itr : modelInfoDB)
  {
    if (matches_object(itr.first.c_str(), query))
    {
      cJSON* obj = cJSON_GetObjectItem(itr.second, "provider");
      return obj->valuestring;
    }
  }

  return nullptr;
}

dmQuery*
dmProviderDatabase::createQuery() const
{
  return new dmQueryImpl();
}

dmQuery* 
dmProviderDatabase::createQuery(dmProviderOperation op, char const* query_string) const
{
  char const* provider_name = getProvider(query_string);
  if (!provider_name)
  {
    rtLog_Warn("failed to find provider for query string:%s", query_string);
    return nullptr;
  }

  dmQueryImpl* query = new dmQueryImpl();
  query->setQueryString(op, query_string);
  query->setProviderName(provider_name);
  return query;
}
