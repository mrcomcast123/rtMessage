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
  MyProvider(std::string const&  providerName) : dmProvider(providerName) { }
protected:
  void doGet(dmPropertyInfo const& param, dmQueryResult& result)
  {
    if (param.name() == "ModelName")
    {
      result.addValue(dmNamedValue(param.name(), "xCam"));
    }
    else if (param.name() == "Manufacturer")
    {
      result.addValue(dmNamedValue(param.name(), "Comcast"));
    }
  }
};

int main()
{
  dmProviderHost* host = dmProviderHost::create();
  host->start();

  host->registerProvider(std::unique_ptr<dmProvider>(new MyProvider("general")));

  while (true)
  {
    sleep(1);
  }

  host->stop();
  return 0;
}
