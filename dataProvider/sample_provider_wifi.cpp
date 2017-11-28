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
      result.addValue(dmNamedValue(param.name(), "xcam_user"));
    }
    else
    {
      printf("\n Yet to implement get for param %s\n", param.name().c_str());
      result.addValue(dmNamedValue(param.name(), "dummy"));
    }
  }

  virtual void doSet(dmNamedValue const& param, dmQueryResult& result)
  {
    if (param.name() == "X_RDKCENTRAL-COM_NoiseLevel")
    {
      setNoiseLevel(param.name(), param.value().toString());
      result.addValue(dmNamedValue(param.name(), param.value()));
    }
    else
      result.addValue(dmNamedValue(param.name(), param.value()));
  }
};

int main()
{
  dmProviderHost* host = dmProviderHost::create();
  host->start();

  host->registerProvider("Device.WiFi", std::unique_ptr<dmProvider>(new MyProvider()));

  while (true)
  {
    sleep(1);
  }

  host->stop();
  return 0;
}
