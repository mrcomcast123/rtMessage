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


extern "C"
{
#include "rtConnection.h"
#include "rtMessage.h"
#include "rtError.h"
#include "rtLog.h"
#include <cJSON.h>
}

#include "dmUtility.h"

class dmQueryImpl : public dmQuery
{
public:
  dmQueryImpl()
  {
    rtConnection_Create(&m_con, "DMCLIENT", "tcp://127.0.0.1:10001");
  }

  ~dmQueryImpl()
  {
    rtConnection_Destroy(m_con);
    m_results.clear();
    reset();
  }

  virtual bool exec()
  {
    if(m_provider.empty())
      return false;

    char* param = new char[50];
    char* value = new char[50];
    int status = 0;

    std::string topic("RDK.MODEL.");
    topic += m_provider;

    rtLog_Info("\nTOPIC TO SEND : %s", topic.c_str());

    rtLog_SetLevel(RT_LOG_DEBUG);

    /* Send request to provider using rtmessage" */
    rtMessage res;
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

    rtError err = rtConnection_SendRequest(m_con, req, topic.c_str(), &res, 2000);
    rtLog_Info("\nSendRequest : %s", rtStrError(err));

    if (err == RT_OK)
    {
      /* Receive response and store in dmQueryResult */
      char* p = NULL;
      uint32_t len = 0;

      rtMessage_ToString(res, &p, &len);
      rtMessage_GetString(res, "name", &param);
      rtMessage_GetString(res, "value", &value);
      rtMessage_GetInt32(res, "status", &status);
      std::string parameter(param);
      std::string paramvalue(value);
      m_results.addValue(dmNamedValue(parameter, dmValue(paramvalue)), status, "Success");
      rtLog_Info("\nResponse : %.*s\n", len, p);
      free(p);
    }

    rtMessage_Destroy(req);
    rtMessage_Destroy(res);
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
          printf("\nSet operation expects value to be set\n");
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
dmProviderDatabase::loadFromDir(std::string const& dir)
{
  DIR* directory;
  struct dirent *ent;

  if ((directory = opendir(m_dataModelDir.c_str())) != NULL) 
  {
     /* Print all the files and directories within directory */
    while ((ent = readdir(directory)) != NULL) 
    {
      if (strcmp(ent->d_name,".")!=0 && strcmp(ent->d_name,"..")!=0 )
        loadFile(dir.c_str(), ent->d_name);
    }
    closedir(directory);
  } 
  else
  {
     printf("\n Could not open directory");
  }
}

void
dmProviderDatabase::loadFile(char const* dir, char const* fname)
{
  cJSON* json = NULL;
  char *file = new char[100];
  
  std::strcat(file, dir);
  std::strcat(file, fname);

  std::string root(fname);
  root.erase(root.length()-5);

  printf("\nRoot %s", root.c_str());

  printf("\nFile %s", file);

  std::ifstream ifile;
  ifile.open(file, std::ifstream::in);

  if (ifile.is_open())
  {
    ifile.seekg(0, ifile.end);
    int length = ifile.tellg();
    printf("\n Length %d \n", length);
    ifile.seekg(0, ifile.beg);

    char *buffer = new char[length];
    ifile.read(buffer,length);

    json = cJSON_Parse(buffer);
    if (!json)
    {
      printf("\nError before: [%s]\n", cJSON_GetErrorPtr());
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
      m_providerInfo.insert(std::make_pair(root, providerInfo));
    }
    delete [] buffer;
  }
  else
  {
    printf("\n Cannot open json file");
  }

  root.clear();
  delete [] file;
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
          printf("\nWild Card will be supported in future\n");
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
    printf("\n Parameter not found \n");
    dataProvider = "";
  }
  else
  {
    printf("\nPROVIDER : %s PARAMETER : %s\n", provider, parameter);
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
