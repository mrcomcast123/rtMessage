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
#include "dmProviderHost.h"
#include "dmProvider.h"
#include <unistd.h>

class MyProvider : public dmObject
{
public:
  MyProvider()
  {

  }

private:
  dmValue getManufacturer()
  {
    return "Comcast";
  }

  dmValue getModelName()
  {
    return "Xcam";
  }

  void setManufacturer(const std::string& name, const std::string& value)
  {
    printf("\nSet %s as %s\n", name.c_str(), value.c_str());
  }

protected:
  // override the basic get and use ladder if/else
  virtual void doGet(dmPropertyInfo const& param, dmQueryResult& result)
  {
    if (param.name() == "Manufacturer")
    {
      result.addValue(param, getManufacturer());
    }
    else if (param.name() == "ModelName")
    {
      result.addValue(param, getModelName());
    }
  }

  virtual void doSet(dmNamedValue const& param, dmQueryResult& result)
  {
    if (param.name() == "Manufacturer")
    {
      setManufacturer(param.name(), param.value().toString());
      result.addValue(param.info(), param.value());
    }
  }
};

int main()
{
  dmProviderHost* host = dmProviderHost::create();
  host->start();

  host->registerProvider("Device.DeviceInfo", new MyProvider());

  while (true)
  {
    sleep(1);
  }

  host->stop();
  return 0;
}
