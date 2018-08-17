#include "dmClient.h"
#include <stdio.h>
#include <stdlib.h>
#include <sstream>
#include <rtError.h>
#include "dmProviderDatabase.h"
#include "dmQuery.h"
#include "dmUtility.h"

void getResultValue(std::vector<dmQueryResult::Param> const& results, std::string const& name, dmValue& value)
{
    rtLog_Debug("getResultValue %s", name.c_str());
    for (auto const& param: results)
    {
      if(param.Info.fullName() == name)
      {
        rtLog_Debug("getResultValue found type=%d string=%s", param.Value.type(), param.Value.toString().c_str());
        value = param.Value;
      }
    }
}

class dmClientImpl : public dmClient
{
public:
  dmClientImpl(std::string const& datamodelDir, rtLogLevel logLevel)
  {
    rtLog_SetLevel(logLevel);
    m_db = new dmProviderDatabase(datamodelDir);
  }

  ~dmClientImpl()
  {
    delete m_db;
  }

  bool runQuery(dmProviderOperation operation, std::string const& parameter, dmClientNotifier* notifier)
  {
    if(isListWithWildcard(parameter))
    {
      std::string list_name = dmUtility::trimWildcard(parameter);
      std::string parent_name = dmUtility::trimProperty(list_name);
      rtLog_Debug("dmcli_get list=%s parent=%s", list_name.c_str(), parent_name.c_str());

      std::string num_entries_param = list_name + std::string("NumberOfEntries");
      int num_entries;
      if(runOneQuery(operation, num_entries_param, nullptr, &num_entries))
      {
        rtLog_Debug("dmcli_get list num_entries=%d", num_entries);

        bool success = true;
        for(int i = 1; i <= num_entries; ++i)
        {
          std::stringstream list_item_param;
          list_item_param << list_name << "." << i << ".";
          std::vector<dmQueryResult::Param> res;
          if(!runOneQuery(operation, list_item_param.str(), notifier))
            success = false;//if only a list item query fails, we keep going but still at end, return false
        }
        return success;
      }
      else
      {
        notifier->onError(RT_FAIL, "dmcli_get list query failed");
        return false;
      }
    }
    else
    {
      return runOneQuery(operation, parameter, notifier);
    }
  }

private:

  bool runOneQuery(dmProviderOperation op, std::string const& parameter, dmClientNotifier* notifier, int* numEntries = nullptr)
  {
    std::unique_ptr<dmQuery> query(m_db->createQuery(op, parameter.c_str()));

    if (!query)
    {
      std::string msg = "failed to create query";
      rtLog_Warn("%s", msg.c_str());
      if(notifier)
        notifier->onError(RT_FAIL, msg);
      return false;
    }

    if (!query->exec())
    {
      std::string msg = "failed to exec query";
      rtLog_Warn("%s", msg.c_str());
      if(notifier)
        notifier->onError(RT_FAIL, msg);
      return false;
    }

    dmQueryResult const& results = query->results();

    if (results.status() == RT_OK)
    {
      if(notifier)
        notifier->onResult(results);
      if(numEntries)
      {
        dmValue value(0);
        getResultValue(results.values(), parameter, value);
        *numEntries = atoi(value.toString().c_str());//todo fix type not coming across wire
      }
      return true;
    }
    else
    {
      rtLog_Warn("Error: status=%d msg=%s", results.status(), results.statusMsg().c_str());
      if(notifier)
        notifier->onError(results.status(), results.statusMsg());
      return false;
    }
  }

  bool isListWithWildcard(std::string const& param)
  {
    if(!param.empty() && param.back() == '.')//is last char wildcard
    {
      std::shared_ptr<dmProviderInfo> info = m_db->getProviderByObjectName(param);
      if(info && info->isList())//is object a list
        return true;
    }
    return false;
  }

  dmProviderDatabase* m_db;
};

dmClient* dmClient::create(std::string const& datamodelDir, rtLogLevel logLevel)
{
  return new dmClientImpl(datamodelDir, logLevel);
}

void dmClient::destroy(dmClient* client)
{
  if(client)
    delete client;
}


