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
#ifndef __DM_UTILITY_H__
#define __DM_UTILITY_H__

#include <string>

class dmUtility
{
public:
  static void splitQuery(char const* query, char* parameter)
  {
    std::string str(query);
    std::size_t position = str.find_last_of(".\\");

    parameter[0]= '\0';
    std::strcat(parameter, str.substr(position + 1).c_str());
    str.clear();
  }

  static bool has_suffix(const std::string &str, const std::string &suffix)
  {
    return str.size() >= suffix.size() && str.find(suffix, str.size() - suffix.size()) != str.npos;
  }

  static bool isWildcard(char const* s)
  {
    if (!s)
      return false;
    return (s[strlen(s) -1 ] == '.');
  }

  static std::string trimWildcard(std::string const& s)
  {
    if (s.empty())
      return s;

    return (s[s.size() -1] == '.')
      ? s.substr(0, s.size() -1)
      : s;
  }

  static std::string trimProperty(std::string const& s)
  {
    std::string t;
    std::string::size_type idx = s.rfind('.');
    if (idx != std::string::npos)
      t = s.substr(0, idx);
    else
      t = s;
    return t;
  }
};

#endif
