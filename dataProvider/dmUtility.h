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
#include <sstream>
#include <string.h>
#include "rtLog.h"

class dmUtility
{
public:
  static void splitQuery(char const* query, char* parameter)
  {
    std::string str(query);
    std::size_t position = str.find_last_of(".\\");

    parameter[0]= '\0';
    strcat(parameter, str.substr(position + 1).c_str());
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

  static std::string trimPropertyName(std::string const &s)
  {
    std::string t;
    std::string::size_type idx = s.rfind('.');
    if (idx != std::string::npos)
      t = s.substr(idx + 1);
    else
      t = s;
    return t;
  }

  static bool isListIndex(const char* s)
  {
    if(!s)
      return false;
    int ln = (int)strlen(s);
    if(ln == 0)
      return false;
    int i = ln - 1;
    while(i >= 0)
      if(s[i] == '.')
        break;
      else
        i--;
    return atoi(s+i+1) != 0;
  }

  //list property is in form [[STRING.]...]NUMBER.[STRING]
  //NUMBER must be a valid number [1-n]
  //example: Device.WiFi.EndPoint.1.Status
  //example: Device.WiFi.EndPoint.9745.Status
  //wildcard:  Device.WiFi.EndPoint.6.
  //invalid formats: [[STRING.]...]NUMBER.NUMBER : ex: Device.WiFi.EndPoint.3.2
  static bool parseListProperty(const std::string& s, uint32_t& index, std::string& name)
  {
    std::string::size_type idx = s.rfind('.');
    if (idx != std::string::npos && idx>0)
    {
      std::string::size_type idx2 = s.rfind('.', idx-1);
      if (idx2 != std::string::npos)
      {
        std::string last = s.substr(idx + 1);
        std::string pathUpToLast = s.substr(0, idx);
        std::string secondLast = pathUpToLast.substr(idx2 + 1);
        uint32_t i = atoi(secondLast.c_str());
        if (i && 
            (last.size() == 0 || //wildcard
             !atoi(last.c_str()))) //non number
        {
          index = i;
          name = pathUpToLast.substr(0, idx2+1) + last;
          return true;
        }
      }
    }
    return false;
  }

  //basic list support supporting a single list object as the last object in the param full name
  static std::string getFullNameWithIndex(const std::string& name, int index)
  {
    if(index < 1)
      return name;

    std::string first = trimProperty(name);
    std::string last = trimPropertyName(name);
    std::stringstream ss;
    ss << first << "." << index << "." << last;
    return ss.str();
  }

//  static std::string getObjectIdFromInstanceId(const std::string& instanceId)
 // { 
  //}

class QueryParser
{
public:
  struct Node
  {
    std::string name; //e.g. EndPoint
    std::string fullName; //e.g. Device.WiFi.EndPoint
    std::string instanceName; //e.g. Device.WiFi.EndPoint.1 or, more complex, Device.WiFi.EndPoint.1.Foo.3.Blah
    int index;
  };

  QueryParser(const std::string& p)
  {
    parse(p);
  }
  void parse(const std::string& p)
  {
    using namespace std;
    if(p.length() == 0)
      return;
    size_t n1 = 0;
    size_t n2 = p.find('.');
    Node node;
    node.index = 0;
    string fullName;
    while(true)
    {
      string token = p.substr(n1, n2-n1);
      int index = atoi(token.c_str());
      if(index)
      {
        if(nodes.size() == 0)
        {
          rtLog_Warn("Invalid paramter");
          return;
        }
        nodes.back().index = index;

        std::stringstream ss;
        ss << nodes.back().instanceName << "." << index;
        nodes.back().instanceName = ss.str();
      }
      else
      {
        if(!fullName.empty())
          fullName += ".";
        fullName += token;
        node.fullName = fullName;
        node.name = token;
        node.instanceName = nodes.size() == 0 ? fullName : nodes.back().instanceName + "." + token;
        nodes.push_back(node);
      }
      if(n2 == string::npos || n2 > p.length()-1)
        break;
      n1 = n2+1;
      n2 = p.find('.', n1);
    }
    /*
    //check if the last token is a value setter and remove value
    if(nodes.size() > 0)
    {
      size_t n = nodes.back().name.find('=');
      if(n != string::npos)
      {
rtLog_Warn("SETTER !!! %s", nodes.back().name.c_str());
        nodes.back().name = nodes.back().name.substr(0,n);
rtLog_Warn("SETTER --> %s", nodes.back().name.c_str());
      }
    }
    */
  }
  size_t getNodeCount() const { return nodes.size(); }
  const QueryParser::Node& getNode(int i) const  { return nodes[i]; }

  static void test(const std::string& p)
  {
    rtLog_Warn("QueryParserTest %s", p.c_str());
    QueryParser q(p);
    for(int i = 0; i < q.getNodeCount(); ++i)
      rtLog_Warn("QueryParserTest node %d: %s, %s, %s, %d", i, q.getNode(i).instanceName.c_str(), q.getNode(i).fullName.c_str(), q.getNode(i).name.c_str(), q.getNode(i).index);
  }
private:
  std::vector<Node> nodes;
};

};

#endif
