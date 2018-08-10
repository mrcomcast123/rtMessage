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
#include <unistd.h>

class MyProvider : public dmProvider
{
public:
  MyProvider() : m_noiseLevel("10dB"), m_userName("xcam_user")
  {
    rtLog_Debug("MyProvider::CTOR");

    // use lambda for simple stuff
    onGet("X_RDKCENTRAL-COM_IPv4Address", []() -> dmValue {
      return "127.0.0.1";
    });

    // can use member function
    onGet("X_RDKCENTRAL-COM_NoiseLevel", std::bind(&MyProvider::getNoiseLevel, this));
    onSet("X_RDKCENTRAL-COM_NoiseLevel", std::bind(&MyProvider::setNoiseLevel, this, std::placeholders::_1));
  }

private:
  dmValue getNoiseLevel()
  {
    rtLog_Debug("MyProvider::getNoiseLevel %s\n", m_noiseLevel.c_str());
    return m_noiseLevel.c_str();
  }

  void setNoiseLevel(dmValue const& value)
  {
    rtLog_Debug("MyProvider::setNoiseLevel %s\n", value.toString().c_str());
    m_noiseLevel = value.toString();
  }

protected:
  // override the basic get and use ladder if/else
  virtual void doGet(dmPropertyInfo const& param, dmQueryResult& result)
  {
    rtLog_Debug("MyProvider::doGet %s\n", param.name().c_str());
    if (param.name() == "X_RDKCENTRAL-COM_UserName")
    {
      result.addValue(param, m_userName);
    }
  }

  virtual void doSet(dmPropertyInfo const& info, dmValue const& value, dmQueryResult& result)
  {
    rtLog_Debug("MyProvider::doSet %s: %s\n", info.name().c_str(), value.toString().c_str());
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

class EndpointProvider : public dmProvider
{
public:
  EndpointProvider()
  {
    onGet("EndpointNumberofItems", []() -> dmValue { return 3; });
  }
};

int main()
{
  dmProviderHost* host = dmProviderHost::create();
  host->start();

  host->registerProvider("Device.WiFi", std::unique_ptr<dmProvider>(new MyProvider()));
  host->registerProvider("Device.WiFi.EndPoint", std::unique_ptr<dmProvider>(new EndpointProvider()));

  while (true)
  {
    sleep(1);
  }

  host->stop();
  return 0;
}
