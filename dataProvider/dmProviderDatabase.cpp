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

class dmQueryImpl : public dmQuery
{
public:
  dmQueryImpl()
  {
    rtConnection_Create(&m_con, "DMCLI", "tcp://127.0.0.1:10001");
  }

  ~dmQueryImpl()
  {
    rtConnection_Destroy(m_con);
    m_results.clear();
    reset();
  }

  virtual bool exec()
  {
    if (m_provider.empty())
    {
      rtLog_Warn("trying to execute a query withtout a provider");
      return false;
    }

    std::string topic("RDK.MODEL.");
    topic += m_provider;

    rtLog_Debug("sending dm query on topic:%s", topic.c_str());

    rtMessage req;
    rtMessage_Create(&req);
    rtMessage_SetInt32(req, "id", m_count);
    rtMessage_SetString(req, "method", m_operation.c_str());
    rtMessage_SetString(req, "provider", m_provider.c_str());

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
      char* param = nullptr;
      char* value = nullptr;

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
    m_provider.clear();
    m_value.clear();
    m_count = 0;
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

  virtual void setProviderName(std::string provider)
  {
    m_provider = provider;
  }

  virtual void setID(int count)
  {
    m_count = count;
  }

private:
  rtConnection m_con;
  dmQueryResult m_results;
  std::string m_query;
  std::string m_provider;
  std::string m_value;
  std::string m_operation;
  int m_count;
};

dmProviderDatabase::dmProviderDatabase(std::string const& dir)
  : m_dataModelDir(dir)
{
  loadFromDir(dir);
}

void
dmProviderDatabase::loadFromDir(std::string const& dirname)
{
  DIR* dir;
  struct dirent *ent;

  rtLog_Debug("loading database from directory:%s", dirname.c_str());
  if ((dir = opendir(m_dataModelDir.c_str())) != NULL)
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
dmProviderDatabase::loadFile(char const* dir, char const* fname)
{
  cJSON* json = NULL;

  std::string path = dir;
  path += "/";
  path += fname;

  rtLog_Debug("load file:%s", path.c_str());

  std::ifstream file;
  file.open(path.c_str(), std::ifstream::in);

  if (file.is_open())
  {
    file.seekg(0, file.end);
    int length = file.tellg();
    file.seekg(0, file.beg);

    char *buffer = new char[length+ 1];
    file.read(buffer, length);
    buffer[length] = '\0';

    json = cJSON_Parse(buffer);
    if (!json)
    {
      rtLog_Error("Failed to parse json from:%s. %s", path.c_str(),
        cJSON_GetErrorPtr());
    }
    else
    {
      dmProviderInfo providerInfo;
      providerInfo.setProviderName(cJSON_GetObjectItem(json, "provider")->valuestring);
      providerInfo.setObjectName(cJSON_GetObjectItem(json, "name")->valuestring);

      cJSON *props = cJSON_GetObjectItem(json, "properties"); 
      for (int i = 0, n = cJSON_GetArraySize(props); i < n; ++i)
      {
        cJSON* item = cJSON_GetArrayItem(props, i);

        dmPropertyInfo propertyInfo;
        propertyInfo.setName(cJSON_GetObjectItem(item, "name")->valuestring);
        propertyInfo.setIsOptional(cJSON_IsTrue(cJSON_GetObjectItem(item, "optional")));
        propertyInfo.setIsWritable(cJSON_IsTrue(cJSON_GetObjectItem(item, "writable")));
        propertyInfo.setType(dmValueType_fromString(cJSON_GetObjectItem(item, "type")->valuestring));
        providerInfo.addProperty(propertyInfo);
      }

      m_providerInfo.insert(std::make_pair(providerInfo.objectName(), providerInfo));
    }

    delete [] buffer;
  }
  else
  {
    rtLog_Warn("failed to open fie. %s. %s", fname, strerror(errno));
  }
}

std::string const
dmProviderDatabase::getProvider(char const* query)
{
  char* model = new char[100];
  char* parameter = new char[50];
  char* provider = new char[50];
  int found = 0;
  std::vector<dmPropertyInfo> getInfo;
  dmUtility dUtil;
  dUtil.splitQuery(query, model, parameter);
  std::string dataProvider;

  std::string data(parameter);
  if (data.find("=") != std::string::npos)
  {
    std::size_t position = data.find("=");
    std::string param = data.substr(0, position);
    strcpy(parameter, param.c_str());
  }

  for (auto map_iter = m_providerInfo.cbegin() ; map_iter != m_providerInfo.cend() ; map_iter++)
  {
    for (auto vec_iter = map_iter->second.properties().cbegin(); vec_iter != map_iter->second.properties().cend() ; vec_iter++)
    {
      dmPropertyInfo* dI = new dmPropertyInfo();
      if (strcmp(map_iter->first.c_str(), model) == 0)
      {
        if (strcmp(parameter, "") == 0)
        {
          rtLog_Error("wildcards not supported");
          break;
        }
        else if (strcmp(vec_iter->name().c_str(), parameter) == 0)
        {
          dI->setName(parameter);
          provider = strdup(map_iter->second.providerName().c_str());
          getInfo.push_back(*dI);
          found = 1;
        }
      }
      delete(dI);
    }
  }

  if (!found)
  {
    rtLog_Warn("parameter not found");
    dataProvider = "";
  }
  else
  {
    dataProvider.clear();
    dataProvider = std::string(provider);
  }

  delete [] parameter;
  delete [] model;
  delete [] provider;
  provider = NULL;
  return dataProvider;
}

dmQuery*
dmProviderDatabase::createQuery()
{
  return new dmQueryImpl();
}

dmQuery* 
dmProviderDatabase::createQuery(dmProviderOperation op, char const* s, const int id)
{
  dmQuery* q = new dmQueryImpl();
  std::string provider = getProvider(s);

  if (!provider.empty())
  {
    static_cast<dmQueryImpl*>(q)->setID(id);
    bool status = q->setQueryString(op, s);
    if(status)
      static_cast<dmQueryImpl*>(q)->setProviderName(provider);
  }
  return q;
}
