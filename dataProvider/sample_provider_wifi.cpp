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
#include <unistd.h>

class MyProvider : public dmProvider
{
public:
  MyProvider()
  {
    // use lambda for simple stuff
    onGet("X_RDKCENTRAL-COM_IPv4Address", []() -> dmValue {
      return "127.0.0.1";
    });

    // can use member function
    onGet("X_RDKCENTRAL-COM_NoiseLevel", std::bind(&MyProvider::getNoiseLevel, this));
  }

private:
  dmValue getNoiseLevel()
  {
    return "10dB";
  }

  void setNoiseLevel(const std::string& name, const std::string& value)
  {
    printf("\nSet %s as %s\n", name.c_str(), value.c_str());
  }

protected:
  // override the basic get and use ladder if/else
  virtual void doGet(dmPropertyInfo const& param, dmQueryResult& result)
  {
    if (param.name() == "X_RDKCENTRAL-COM_UserName")
    {
      result.addValue(param, "xcam_user");
    }
  }

  //
  // TODO
  // This should be more like doGet
  //      void doSet(dmPropertyInfo const& info, dmValue const& value, dmQueryResult& result);
  //
  virtual void doSet(dmNamedValue const& param, dmQueryResult& result)
  {
    if (param.name() == "X_RDKCENTRAL-COM_NoiseLevel")
    {
      setNoiseLevel(param.name(), param.value().toString());
      result.addValue(param.info(), param.value());
    }
    else
      result.addValue(param.info(), param.value());
  }
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
