#include <iostream>
#include <dirent.h>
#include <cstddef>
#include <string>
#include <cstring>
#include <map>
#include <iterator>
#include <fstream>
#include <iomanip>

#include "dmProvider.h"

void splitQuery(char const* query, char* model, char* parameter);

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

void dmPropertyInfo::setName(std::string name)
{
  m_propname = name;
}

std::string const& dmPropertyInfo::name() const
{
  return m_propname;
}

void dmProviderInfo::setName(std::string name)
{
  m_provider = name;
}

std::string const& dmProviderInfo::name() const
{
  return m_provider;
}

void dmProviderInfo::setProperties(std::vector<dmPropertyInfo> propInfo)
{
  m_propertyInfo = propInfo;
}

std::vector<dmPropertyInfo> const& dmProviderInfo::properties() const
{
  return m_propertyInfo;
}

dmProviderDatabase::dmProviderDatabase(std::string dir):m_directory(dir)
{
  loadFromDir(dir);
}

void dmProviderDatabase::loadFromDir(std::string dir)
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
      }       
      providerDetails.insert(std::make_pair(root, *dP));
      for( auto map_iter = providerDetails.cbegin() ; map_iter != providerDetails.cend() ; ++map_iter )
      {         
        std::cout  << "\n\tRoot" << "\t" << "Provider" << "\t" << "Parameter" << std::endl;
        for( auto vec_iter = map_iter->second.properties().cbegin(); vec_iter != map_iter->second.properties().cend() ; ++vec_iter )
        {
          std::cout << '|' << std::setw(10) << map_iter->first << '|' << std::setw(10) << map_iter->second.name() << " ";
          std::cout << '|' << std::setw(10) << vec_iter->name() << "  " << std::endl ;
        }
      }
      providerDetails.clear();          
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
  std::string dataPath("./data/");
  loadFromDir(dataPath);
}

void doGet(std::string const& providerName, std::vector<dmPropertyInfo> const& propertyName)
{

}

int main()
{

}

