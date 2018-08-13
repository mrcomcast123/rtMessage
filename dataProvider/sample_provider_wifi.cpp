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

class WiFiProvider : public dmProvider
{
public:
  WiFiProvider() : m_noiseLevel("10dB"), m_userName("xcam_user")
  {
    rtLog_Debug("WiFiProvider::CTOR");

    // use lambda for simple stuff
    onGet("X_RDKCENTRAL-COM_IPv4Address", [](dmPropertyInfo const& info, dmQueryResult& result) -> void {
      result.addValue(info, "127.0.0.1");
    });

    // can use member function
    onGet("X_RDKCENTRAL-COM_NoiseLevel", std::bind(&WiFiProvider::getNoiseLevel, this, std::placeholders::_1, std::placeholders::_2));
    onSet("X_RDKCENTRAL-COM_NoiseLevel", std::bind(&WiFiProvider::setNoiseLevel, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    onGet("EndPointNumberOfEntries", [](dmPropertyInfo const& info, dmQueryResult& result) -> void { 
      result.addValue(info, 3);
    });
  }

private:
  void getNoiseLevel(dmPropertyInfo const& info, dmQueryResult& result)
  {
    rtLog_Debug("WiFiProvider::getNoiseLevel %s\n", m_noiseLevel.c_str());
    result.addValue(info, m_noiseLevel.c_str());
  }

  void setNoiseLevel(dmPropertyInfo const& info, dmValue const& value, dmQueryResult& result)
  {
    rtLog_Debug("WiFiProvider::setNoiseLevel %s\n", value.toString().c_str());
    m_noiseLevel = value.toString();
    result.addValue(info, value);
  }

protected:
  // override the basic get and use ladder if/else
  virtual void doGet(dmPropertyInfo const& info, dmQueryResult& result)
  {
    rtLog_Debug("WiFiProvider::doGet %s\n", info.name().c_str());
    if (info.name() == "X_RDKCENTRAL-COM_UserName")
    {
      result.addValue(info, m_userName);
    }
  }

  virtual void doSet(dmPropertyInfo const& info, dmValue const& value, dmQueryResult& result)
  {
    rtLog_Debug("WiFiProvider::doSet %s: %s\n", info.name().c_str(), value.toString().c_str());
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

class EndPointProvider : public dmProvider
{
public:
  EndPointProvider()
  {
    m_endPoints.push_back({"NETGEAR1","WPA","0","12","Online"});
    m_endPoints.push_back({"RAWHIDE123","WPA+PKI","1","3","Online"});
    m_endPoints.push_back({"XB3","Comcast","0","11","Offline"});
    onGet("Status", std::bind(&EndPointProvider::getStatus, this, std::placeholders::_1, std::placeholders::_2));
    onSet("Status", std::bind(&EndPointProvider::setStatus, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    onGet("SSID", std::bind(&EndPointProvider::getSSID, this, std::placeholders::_1, std::placeholders::_2));
    onSet("SSID", std::bind(&EndPointProvider::setSSID, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  }
protected:
  virtual size_t getListSize()
  {
    return m_endPoints.size();
  }
private:
  void getStatus(dmPropertyInfo const& info, dmQueryResult& result)
  {
    rtLog_Debug("WiFiProvider::getStatus index=%u\n", info.index());
    if(info.index() < m_endPoints.size())
    {
      result.addValue(info, m_endPoints[info.index()].status);
    }
    else
    {
      result.setStatus(RT_ERROR_INVALID_ARG);
      result.setStatusMsg("Index out of range");
    }
  }

  void setStatus(dmPropertyInfo const& info, dmValue const& value, dmQueryResult& result)
  {
    rtLog_Debug("WiFiProvider::setStatus index=%u value=%s\n", info.index(), value.toString().c_str());
    if(info.index() < m_endPoints.size())
    {
      m_endPoints[info.index()].status = value.toString();
      result.addValue(info, value);
    }
    else
    {
      result.setStatus(RT_ERROR_INVALID_ARG);
      result.setStatusMsg("Index out of range");
    }
  }

  void getSSID(dmPropertyInfo const& info, dmQueryResult& result)
  {
    rtLog_Debug("WiFiProvider::getSSID index=%u\n", info.index());
    if(info.index() < m_endPoints.size())
    {
      result.addValue(info, m_endPoints[info.index()].ssid);
    }
    else
    {
      result.setStatus(RT_ERROR_INVALID_ARG);
      result.setStatusMsg("Index out of range");
    }
  }

  void setSSID(dmPropertyInfo const& info, dmValue const& value, dmQueryResult& result)
  {
    rtLog_Debug("WiFiProvider::setSSID index=%u value=%s\n", info.index(), value.toString().c_str());
    if(info.index() < m_endPoints.size())
    {
      m_endPoints[info.index()].ssid = value.toString();
      result.addValue(info, value);
    }
    else
    {
      result.setStatus(RT_ERROR_INVALID_ARG);
      result.setStatusMsg("Index out of range");
    }
  }

  struct EndPoint
  {
    std::string ssid;
    std::string securityType;
    std::string channel;
    std::string mode;
    std::string status;
  };
  std::vector<EndPoint> m_endPoints;
};

int main()
{
  dmProviderHost* host = dmProviderHost::create();
  host->start();

  host->registerProvider("Device.WiFi", std::unique_ptr<dmProvider>(new WiFiProvider()));
  host->registerProvider("Device.WiFi.EndPoint", std::unique_ptr<dmProvider>(new EndPointProvider()));

  while (true)
  {
    sleep(1);
  }

  host->stop();
  return 0;
}
