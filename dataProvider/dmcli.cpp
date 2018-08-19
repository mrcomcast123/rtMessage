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
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <iostream>
#include <string>
#include <algorithm>
#include "dmClient.h"

void print_help()
{
  printf("dmcli [OPTIONS] [COMMAND] [PARAMS]\n");
  printf("\t-d  --datamodel   Set datamodel directory\n");
  printf("\t-h  --help        Print this help and exit\n");
  printf("\t-v  --verbose     Print verbose output. same as -l 0\n");
  printf("\t-l  --log-level   Set log level 0(verbose)-4\n");
  printf("\t-g  --get         Gets a list of parameters\n");
  printf("\t-s  --set         Sets a list of parameters\n");
  printf("\n");
  printf("Examples:\n");
  printf("\t1.) dmcli -g Device.DeviceInfo.ModelName,Device.DeviceInfo.SerialNumber\n");
  printf("\t2.) dmcli -s Services.Service.Foo.Bar=1,Services.Sevice.Foo.Baz=2\n");
  printf("\n");
}

//for non-list query, expect a single onResult or onError
//for lists, 1 query per list item is made so expect onResult or onError for each item in the list
class Notifier : public dmClientNotifier
{
public:
  virtual void onResult(const dmQueryResult& result)
  {
    size_t maxParamLength = 0;
    for (auto const& param : result.values())
    {
        if (param.fullName.length() > maxParamLength)
          maxParamLength = param.fullName.length();
    }

    std::cout << std::endl;
    for (auto const& param: result.values())
    {
      std::cout << "    ";
      std::cout.width(maxParamLength);
      std::cout << std::left;
      std::cout << param.fullName;
      std::cout << " = ";
      std::cout << param.Value.toString();
      std::cout << std::endl;
    }
    std::cout << std::endl;
  }
  virtual void onError(int status, std::string const& message)
  {
    std::cout << std::endl << "    Error " << status << ": " << message << std::endl;
  }
};

int main(int argc, char *argv[])
{
  int exit_code = 0;
  std::string param_list;
  rtLogLevel log_level = RT_LOG_FATAL;

  #ifdef DEFAULT_DATAMODELDIR
  std::string datamodel_dir = DEFAULT_DATAMODELDIR;
  #else
  std::string datamode_dir;
  #endif

  dmProviderOperation op = dmProviderOperation_Get;

  if (argc == 1)
  {
    print_help();
    return 0;
  }

  while (1)
  {
    int option_index;
    static struct option long_options[] = 
    {
      { "get", required_argument, 0, 'g' },
      { "set", required_argument, 0, 's' },
      { "help", no_argument, 0, 'h' },
      { "datamode-dir", required_argument, 0, 'd' },
      { "verbose", no_argument, 0, 'v' },
      { "log-level", no_argument, 0, 'l' },
      { 0, 0, 0, 0 }
    };

    int c = getopt_long(argc, argv, "d:g:hs:vl:", long_options, &option_index);
    if (c == -1)
      break;

    switch (c)
    {
      case 'd':
        datamodel_dir = optarg;
        break;

      case 'g':
        param_list = optarg;
        op = dmProviderOperation_Get;
        break;

      case 's':
        param_list = optarg;
        op = dmProviderOperation_Set;
        break;

      case 'h':
        print_help();
        exit(0);
        break;

      case 'v':
        log_level = RT_LOG_DEBUG;
        break;

      case 'l':
        log_level = (rtLogLevel)atoi(optarg);
        break;

      case '?':
        break;
    }
  }

  if (param_list.empty())
  {
    printf("\nNo parameter list found\n");
    print_help();
    exit(1);
  }

  dmClient* client = dmClient::create(datamodel_dir.c_str(), log_level);

  std::string delimiter = ",";
  std::string paramlist(param_list);
  std::string token;

  size_t begin = 0;
  size_t end = 0;

  while (begin != std::string::npos)
  {
    // split token on ',' and remove whitespace
    end = param_list.find(',', begin);
    std::string token = param_list.substr(begin, (end - begin));
    token.erase(std::remove_if(token.begin(), token.end(), [](char c)
    {
      return ::isspace(c);
    }), token.end());

    Notifier notifier;
    client->runQuery(op, token, &notifier);

    begin = end == std::string::npos ? std::string::npos : end + 1;
  }

  dmClient::destroy(client);

  return exit_code;
}
