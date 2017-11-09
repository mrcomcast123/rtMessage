#ifndef __DM_PROVIDER_H__
#define __DM_PROVIDER_H__

#include <map>
#include <memory>
#include <string>
#include <vector>
#include <cJSON.h>

#include "dmValue.h"

class dmPropertyInfo
{
public:
  std::string const& name() const;
  void setName(std::string name);
  // other stuff like type, min/max, etc
  // leave that out for now.
public:
  std::string m_propname;
};

class dmProviderInfo
{
public:
  std::string const& name() const;
  std::vector<dmPropertyInfo> const& properties() const;

  void setName(std::string name);
  void setProperties(std::vector<dmPropertyInfo>);
public:
  std::string m_provider;
  std::vector<dmPropertyInfo> m_propertyInfo;
};


// make this a singleton
class dmProviderDatabase
{
public:
  dmProviderDatabase(std::string dir);
  void loadFromDir(std::string dir);
  void loadFile(char const* dir, char const* fname);

public:
  // if query is a complete object name
  // Device.DeviceInfo, then it'll return a vector of length 1
  // if query it wildcard, then it'll return a vector > 1
  std::vector<dmPropertyInfo> get(char const* query);
private:
  std::string m_directory;
  std::map<std::string, dmProviderInfo> providerDetails;
};

#endif
