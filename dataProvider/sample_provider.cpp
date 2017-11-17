#include "dmProviderHost.h"
#include "dmProvider.h"

#include <memory> 
#include <cstring>

/*Sample Provider class to supply generic parameters */
class generalProvider : public dmProvider
{
public:
  generalProvider(std::string providerName)
  {
    m_providerName = providerName;
  }

  ~generalProvider()
  {

  }

  void doGet(std::vector<dmPropertyInfo> const& params, dmQueryResult& result)
  {
    dmQueryResult temp;
    for (auto const& propInfo : params)
    {
      doGet(propInfo, temp);
      result.merge(temp);
      temp.clear();
    }
  }

  void doSet(std::vector<dmNamedValue> const& params, dmQueryResult& result)
  {  
    dmQueryResult temp;
    for (auto const& value : params)
    {
      doSet(value, temp);
      result.merge(temp);
      temp.clear();
    }
  }

  std::string dummyGet(std::string param)
  {
    std::string value;
    if (strcmp(param.c_str(), "ModelName") == 0)
      value = "XCAM";
    else if (strcmp(param.c_str(), "Manufacturer") == 0)
      value = "Comcast";
    else
      value = "DummyValue";
    return value;
  } 

protected:
  void doGet(dmPropertyInfo const& param, dmQueryResult& result)
  {
  	result.setStatus(0);
  	std::string value = dummyGet(param.name());
  	result.addValue(dmNamedValue(param.name(), dmValue(value)), 1, "Success");
  }

  void doSet(dmNamedValue const& params, dmQueryResult& result)
  {
    printf("\n Called dmProvider doSet \n");
  }

private:
  std::string m_providerName;
};

int main()
{
  std::string providerName("general");   
  std::unique_ptr<generalProvider> provider(new generalProvider(providerName));
  while (1)
  {  	
  	dmProviderHost* dmHost = dmProviderHost::create();
    dmHost->start();  
    bool status = dmHost->registerProvider(providerName, std::move(provider));
    dmHost->run();
    dmHost->stop();    
  }
  provider.release();
}
