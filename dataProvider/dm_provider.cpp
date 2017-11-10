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

#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <stdlib.h>
#include <cstring>
#include <memory>

#include "dmProvider.h"

void 
Usage()
{
  printf("Usage:\n ./dm_provider option parameter \n Eg: ./dm_provider getopt Device.DeviceInfo.Manufacturer \n Eg: ./dm_provider setopt Device.DeviceInfo.Manufacturer Sercomm");
}

int main(int argc, char *argv[])
{
  int index = 0;
  char *opt = NULL;
  char *param = NULL;
  char *get_value = NULL;
  char *set_value = NULL;
  int ret = 0;

  if (index + 2 <= argc - 1)
  {
    index++;
    opt = argv[index];
    index++;
    param = argv[index];
    if (strcmp(opt, "setopt") == 0)
    {
      index++;
      set_value = argv[index];
    }
    printf("\nOption: %s Parameter: %s\n", opt, param);
  }
  else
  {
    printf("\nNot enough arguments\n");
    Usage();
    return 1;
  }

  if (std::strcmp(opt, "getopt") == 0)
  {
    std::string path("./data/");
    std::shared_ptr<dmProviderDatabase> db(new dmProviderDatabase(path));
    auto providers = db->get(param);
  }
  else if (std::strcmp(opt, "setopt") == 0)
  {
    //ret = dmProvider_setParameter(connect, param, set_value);
  }
  else
  {
    printf("Invalid operation. Use getopt/setopt");
  }
}