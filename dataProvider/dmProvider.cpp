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

extern "C"
{
#include "rtMessage.h"
#include "rtError.h"
}

#include "dmProvider.h"

void splitQuery(char const* query, char* model, char* parameter);
void doGet(std::string const& providerName, std::vector<dmPropertyInfo> &propertyName);

void splitQuery(char const* query, char* model, char* parameter)
{
  std::string str(query);
  std::size_t position = str.find_last_of(".\\");  
  model[0]= '\0';
  std::strcat(model , str.substr(0, position).c_str()); 
  parameter[0]= '\0'; 
  std::strcat(parameter, str.substr(position+1).c_str());
  str.clear();
}

void dmPropertyInfo::setName(std::string const& name)
{
  m_propname = name;
}

std::string const& dmPropertyInfo::name() const
{
  return m_propname;
}

void dmProviderInfo::setName(std::string const& name)
{
  m_provider = name;
}

std::string const& dmProviderInfo::name() const
{
  return m_provider;
}

void dmProviderInfo::setProperties(std::vector<dmPropertyInfo> const& propInfo)
{
  m_propertyInfo = propInfo;
}

std::vector<dmPropertyInfo> const& dmProviderInfo::properties() const
{
  return m_propertyInfo;
}

dmProviderDatabase::dmProviderDatabase(std::string const& dir):m_directory(dir)
{
  loadFromDir(dir);
}

dmProviderDatabase::~dmProviderDatabase()
{
  providerDetails.clear();
}

void dmProviderDatabase::loadFromDir(std::string const& dir)
{
  DIR* directory;
  struct dirent *ent;

  if ((directory = opendir(m_directory.c_str())) != NULL) 
  {
     /* print all the files and directories within directory */
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

void dmProviderDatabase::loadFile(char const* dir, char const* fname)
{
  cJSON* json = NULL;
  char *file = new char[80];
  
  std::strcat(file, dir);
  std::strcat(file, fname);

  std::string root(fname);
  root.erase(root.length()-5);

  printf("\nRoot %s", root.c_str());

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
      dmProviderInfo* dP = new dmProviderInfo(); 
      std::string provider = cJSON_GetObjectItem(json, "provider")->valuestring;
      dP->setName(provider);
      std::vector<dmPropertyInfo> propInfo;
      cJSON *properties = cJSON_GetObjectItem(json, "properties"); 
      for (int i = 0 ; i < cJSON_GetArraySize(properties); i++)
      {
        dmPropertyInfo* dI = new dmPropertyInfo();
        cJSON *subitem = cJSON_GetArrayItem(properties, i);
        dI->setName(cJSON_GetObjectItem(subitem, "name")->valuestring);
        propInfo.push_back(*dI);
        dP->setProperties(propInfo);
        delete (dI);
      }       
      providerDetails.insert(std::make_pair(root, *dP));
      delete (dP);
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

std::vector<dmPropertyInfo> dmProviderDatabase::get(char const* query)
{
  char* model = new char[100];
  char* parameter = new char[50];
  char* provider = new char[50];
  int found = 0;
  std::vector<dmPropertyInfo> getInfo;
  splitQuery(query, model, parameter);

  for (auto map_iter = providerDetails.cbegin() ; map_iter != providerDetails.cend() ; map_iter++)
  {
    for (auto vec_iter = map_iter->second.properties().cbegin(); vec_iter != map_iter->second.properties().cend() ; vec_iter++)
    {
      dmPropertyInfo* dI = new dmPropertyInfo();
      if (strcmp(map_iter->first.c_str(), model) == 0)
      {
        if (strcmp(parameter, "") == 0)
        {
          printf("\nWild Card will be supported in future\n");
          exit(0);
        }
        else if (strcmp(vec_iter->name().c_str(), parameter) == 0)
        {
          dI->setName(parameter);
          strncpy(provider, map_iter->second.name().c_str(), sizeof(map_iter->second.name().c_str()));
          getInfo.push_back(*dI);
          found = 1;
        }
      }
      delete(dI);
    }
  }

  if (!found)
    printf("\n Paramter not found");
  else
  {
    printf("\nPROVIDER : %s PARAMETER : %s\n", provider, parameter);
    std::string dataProvider(provider);
    doGet(dataProvider, getInfo);
  }

  delete [] provider;
  delete [] parameter;
  delete [] model;
  return getInfo;
}

void doGet(std::string const& providerName, std::vector<dmPropertyInfo> &propertyName)
{
  rtConnection con;
  rtConnection_Create(&con, "DMCLIENT", "tcp://127.0.0.1:10001");

  dmPropertyInfo dI = propertyName.back();
  propertyName.pop_back();

  char* parameter = new char[strlen(dI.name().c_str())+1];

  strncpy(parameter, dI.name().c_str(), strlen(dI.name().c_str())+1);
  parameter[strlen(parameter)+1] = '\0';

  /* TODO send request to provider using rtmessage" */
  rtMessage res;
  rtMessage req;
  rtMessage_Create(&req);
  rtMessage_SetString(req, "propertyName", parameter);

  rtError err = rtConnection_SendRequest(con, req, providerName.c_str(), &res, 2000);
  printf("SendRequest:%s", rtStrError(err));

  if (err == RT_OK)
  {
    char* p = NULL;
    uint32_t len = 0;

    rtMessage_ToString(res, &p, &len);
    printf("\tres:%.*s\n", len, p);
    free(p);
  }

  rtMessage_Destroy(req);
  rtMessage_Destroy(res);

  rtConnection_Destroy(con);

  delete [] parameter;
}

int main()
{

}