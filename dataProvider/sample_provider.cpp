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
    if(strcmp(opt, "setopt") == 0)
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
    //auto providers = db->get(param);
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
