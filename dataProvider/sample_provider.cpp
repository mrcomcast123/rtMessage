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
    onGet("ModelName", []() -> dmValue {
      return "xCam";
    });

    // can use member function
    onGet("Manufacturer", std::bind(&MyProvider::getManufacturer, this));
    onGet("X_RDKCENTRAL-COM_NoiseLevel", std::bind(&MyProvider::getNoiseLevel, this));
  }

private:
  dmValue getManufacturer()
  {
    return "Comcast";
  }

  dmValue getNoiseLevel()
  {
    return "10dB";
  }

  void setManufacturer(const std::string& name, const std::string& value)
  {
    printf("\nSet %s as %s\n", name.c_str(), value.c_str());
  }

protected:
  // override the basic get and use ladder if/else
  virtual void doGet(dmPropertyInfo const& param, dmQueryResult& result)
  {
    if (param.name() == "SerialNumber")
    {
      result.addValue(dmNamedValue(param.name(), 12345));
    }
    else
    {
      printf("\n Yet to implement get for param %s", param.name().c_str());
    }
  }

  virtual void doSet(dmNamedValue const& param, dmQueryResult& result)
  {
    if (param.name() == "Manufacturer")
    {
      setManufacturer(param.name(), param.value().toString());
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

  //host->registerProvider(std::unique_ptr<dmProvider>(new MyProvider("general")));
  host->registerProvider("Device.DeviceInfo", std::unique_ptr<dmProvider>(new MyProvider()));

  while (true)
  {
    sleep(1);
  }

  host->stop();
  return 0;
}
