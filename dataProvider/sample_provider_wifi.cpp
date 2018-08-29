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
#include <rtLog.h>
#include <rtError.h>
#include <unistd.h>

class AdapterObject : public dmObject
{
public:
  AdapterObject(int id)
  {
    char buff[100];
    sprintf(buff, "Adapter-%d", id);
    mname = buff;
    onGet("Name", [this](dmPropertyInfo const& info, dmQueryResult& result) -> void { result.addValue(info, mname.c_str()); });
    onSet("Name", [this](dmPropertyInfo const& info, dmValue const& value, dmQueryResult& result) -> void { mname = value.toString(); result.addValue(info, mname.c_str()); });
  }
  std::string mname;
};

class InterfaceObject : public dmObject
{
public:
  InterfaceObject(int id)
  {
    char buff[100];
    sprintf(buff, "Interface-%d", id);
    mname = buff;
    onGet("Name", [this](dmPropertyInfo const& info, dmQueryResult& result) -> void { result.addValue(info, mname.c_str()); });
    onSet("Name", [this](dmPropertyInfo const& info, dmValue const& value, dmQueryResult& result) -> void { mname = value.toString(); result.addValue(info, mname.c_str()); });
  }
  std::string mname;
};

class RouterObject : public dmObject
{
public:
  RouterObject()
  {
    mname = "Router";
    onGet("Name", [this](dmPropertyInfo const& info, dmQueryResult& result) -> void { result.addValue(info, mname.c_str()); });
    onSet("Name", [this](dmPropertyInfo const& info, dmValue const& value, dmQueryResult& result) -> void { mname = value.toString(); result.addValue(info, mname.c_str()); });
  }
  std::string mname;
};

class EndPointObject : public dmObject
{
public:
  EndPointObject(const char* name, const char* security, const char* channel, const char* mode, const char* status) 
    : mssid(name), msecurityType(security), mchannel(channel), mmode(mode), mstatus(status)
  {
    onGet("Status", std::bind(&EndPointObject::getStatus, this, std::placeholders::_1, std::placeholders::_2));
    onSet("Status", std::bind(&EndPointObject::setStatus, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    onGet("SSID", std::bind(&EndPointObject::getSSID, this, std::placeholders::_1, std::placeholders::_2));
    onSet("SSID", std::bind(&EndPointObject::setSSID, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  }
  void regChild(dmProviderHost* host)
  {
  }
private:
  void getStatus(dmPropertyInfo const& info, dmQueryResult& result)
  {
    rtLog_Debug("EndPointObject::getStatus index=%u\n", info.index());
    result.addValue(info, mstatus);
  }

  void setStatus(dmPropertyInfo const& info, dmValue const& value, dmQueryResult& result)
  {
    rtLog_Debug("EndPointObject::setStatus index=%u value=%s\n", info.index(), value.toString().c_str());
    mstatus = value.toString();
    result.addValue(info, value);
  }

  void getSSID(dmPropertyInfo const& info, dmQueryResult& result)
  {
    rtLog_Debug("EndPointObject::getSSID index=%u\n", info.index());
    result.addValue(info, mssid);
  }

  void setSSID(dmPropertyInfo const& info, dmValue const& value, dmQueryResult& result)
  {
    rtLog_Debug("EndPointObject::setSSID index=%u value=%s\n", info.index(), value.toString().c_str());
    mssid = value.toString();
    result.addValue(info, value);
  }

  std::string mssid;
  std::string msecurityType;
  std::string mchannel;
  std::string mmode;
  std::string mstatus;
};

class WiFiObject : public dmObject
{
public:
  WiFiObject() : m_noiseLevel("10dB"), m_userName("xcam_user")
  {
    rtLog_Debug("WiFiObject::CTOR");

    // use lambda for simple stuff
    onGet("X_RDKCENTRAL-COM_IPv4Address", [](dmPropertyInfo const& info, dmQueryResult& result) -> void {
      result.addValue(info, "127.0.0.1");
    });

    // can use member function
    onGet("X_RDKCENTRAL-COM_NoiseLevel", std::bind(&WiFiObject::getNoiseLevel, this, std::placeholders::_1, std::placeholders::_2));
    onSet("X_RDKCENTRAL-COM_NoiseLevel", std::bind(&WiFiObject::setNoiseLevel, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    onGet("EndPointNumberOfEntries", [](dmPropertyInfo const& info, dmQueryResult& result) -> void { 
      result.addValue(info, 3);
    });
  }

private:
  void getNoiseLevel(dmPropertyInfo const& info, dmQueryResult& result)
  {
    rtLog_Debug("WiFiObject::getNoiseLevel %s\n", m_noiseLevel.c_str());
    result.addValue(info, m_noiseLevel.c_str());
  }

  void setNoiseLevel(dmPropertyInfo const& info, dmValue const& value, dmQueryResult& result)
  {
    rtLog_Debug("WiFiObject::setNoiseLevel %s\n", value.toString().c_str());
    m_noiseLevel = value.toString();
    result.addValue(info, value);
  }

protected:
  // override the basic get and use ladder if/else
  virtual void doGet(dmPropertyInfo const& info, dmQueryResult& result)
  {
    rtLog_Debug("WiFiObject::doGet %s\n", info.name().c_str());
    if (info.name() == "X_RDKCENTRAL-COM_UserName")
    {
      result.addValue(info, m_userName);
    }
  }

  virtual void doSet(dmPropertyInfo const& info, dmValue const& value, dmQueryResult& result)
  {
    rtLog_Debug("WiFiObject::doSet %s: %s\n", info.name().c_str(), value.toString().c_str());
    if (info.name() == "X_RDKCENTRAL-COM_UserName")
    {
      m_userName = value.toString();
      result.addValue(info, value);
    }
  }
private:
  std::string m_noiseLevel;
  std::string m_userName;
};

int main(int argc, char **argv)
{
  dmUtility::QueryParser::test(argv[1]);return 0;  
  
  dmProviderHost* host = dmProviderHost::create();
  host->start();

  host->registerProvider("Device.WiFi", new WiFiObject());
  host->addObject("Device.WiFi.EndPoint.1", new EndPointObject("NETGEAR1","WPA","0","12","Online"));
  host->addObject("Device.WiFi.EndPoint.2", new EndPointObject("RAWHIDE123","WPA+PKI","1","3","Online"));
  host->addObject("Device.WiFi.EndPoint.3", new EndPointObject("XB3","Comcast","0","11","Offline"));
  host->addObject("Device.WiFi.EndPoint.1.Adapter", new AdapterObject(1));
  host->addObject("Device.WiFi.EndPoint.2.Adapter", new AdapterObject(2));
  host->addObject("Device.WiFi.EndPoint.3.Adapter", new AdapterObject(3));
  host->addObject("Device.WiFi.EndPoint.1.Interface", new InterfaceObject(1));
  host->addObject("Device.WiFi.EndPoint.2.Interface", new InterfaceObject(2));
  host->addObject("Device.WiFi.EndPoint.3.Interface", new InterfaceObject(3));
  host->addObject("Device.WiFi.Router", new RouterObject());
  host->printTree();
  while (true)
  {
    sleep(1);
  }

  host->stop();
  return 0;
}
