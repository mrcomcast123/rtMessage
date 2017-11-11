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

#include <rtConnection.h>

static void
rtMessageHandler(rtMessageHeader const* hdr, uint8_t const* buff, uint32_t n, void* closure)
{
  dmProviderHost* host = reinterpret_cast<dmProviderHost *>(closure);

  // TODO:

  // call get/set

  // return response
}

dmProviderHost::dmProviderHost()
{
  // TODO:
  // create rtConnection
  // register rtMessageHandler as callback
}

void
dmProviderHost::start()
{
  std::unique_lock<std::mutex> lock(m_mutex);
  m_thread.reset(new std::thread(&dmProviderHost::run, this));
}

void
dmProviderHost::stop()
{
  std::unique_lock<std::mutex> lock(m_mutex);
  if (m_thread)
  {
    // TODO
  }
}

void
dmProviderHost::run()
{
  while (true)
  {
    // TODO receive from rtMessage, dispatch, then send response
  }
}

void
dmProviderHost::registerProvider(std::string const& name, std::unique_ptr<dmProvider> provider)
{
  m_providers.insert(std::make_pair(name, std::move(provider)));
}

void
dmProviderHost::doGet(std::string const& providerName, std::vector<dmPropertyInfo> const& params,
    dmQueryResult& result)
{
  auto itr = m_providers.find(providerName);
  if (itr != m_providers.end())
  {
    itr->second->doGet(params, result);
  }
}

void
dmProviderHost::doSet(std::string const& providerName, std::vector<dmNamedValue> const& params,
    dmQueryResult& result)
{
  auto itr = m_providers.find(providerName);
  if (itr != m_providers.end())
  {
    itr->second->doSet(params, result);
  }
}
