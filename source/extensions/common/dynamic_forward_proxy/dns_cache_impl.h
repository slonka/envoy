#pragma once

#include "envoy/network/dns.h"
#include "envoy/thread_local/thread_local.h"

#include "common/common/cleanup.h"

#include "extensions/common/dynamic_forward_proxy/dns_cache.h"

#include "absl/container/flat_hash_map.h"

namespace Envoy {
namespace Extensions {
namespace Common {
namespace DynamicForwardProxy {

class DnsCacheImpl : public DnsCache, Logger::Loggable<Logger::Id::dfproxy> {
public:
  DnsCacheImpl(Event::Dispatcher& main_thread_dispatcher, ThreadLocal::SlotAllocator& tls);
  ~DnsCacheImpl();

  // DnsCache
  LoadDnsCacheHandlePtr loadDnsCache(absl::string_view host,
                                     LoadDnsCacheCallbacks& callbacks) override;
  AddUpdateCallbacksHandlePtr addUpdateCallbacks(UpdateCallbacks& callbacks) override;

private:
  using TlsHostMap = absl::flat_hash_map<std::string, SharedHostInfoSharedPtr>;
  using TlsHostMapSharedPtr = std::shared_ptr<TlsHostMap>;

  // fixfix
  struct LoadDnsCacheHandleImpl : public LoadDnsCacheHandle,
                                  RaiiListElement<LoadDnsCacheHandleImpl*> {
    LoadDnsCacheHandleImpl(std::list<LoadDnsCacheHandleImpl*>& parent, absl::string_view host,
                           LoadDnsCacheCallbacks& callbacks)
        : RaiiListElement<LoadDnsCacheHandleImpl*>(parent, this), host_(host),
          callbacks_(callbacks) {}

    const std::string host_;
    LoadDnsCacheCallbacks& callbacks_;
  };

  // fixfix
  struct ThreadLocalHostInfo : public ThreadLocal::ThreadLocalObject {
    TlsHostMapSharedPtr host_map_{std::make_shared<TlsHostMap>()};
    std::list<LoadDnsCacheHandleImpl*> pending_resolutions_;
  };

  // fixfix
  struct PrimaryHostInfo {
    SharedHostInfoSharedPtr info_;
    Network::ActiveDnsQuery* active_query_{};
  };

  using PrimaryHostInfoPtr = std::unique_ptr<PrimaryHostInfo>;

  // fixfix
  struct AddUpdateCallbacksHandleImpl : public AddUpdateCallbacksHandle,
                                        RaiiListElement<UpdateCallbacks*> {
    AddUpdateCallbacksHandleImpl(std::list<UpdateCallbacks*>& parent, UpdateCallbacks& callbacks)
        : RaiiListElement<UpdateCallbacks*>(parent, &callbacks) {}
  };

  void startResolve(const std::string& host);
  void finishResolve(PrimaryHostInfo& primary_host_info,
                     const std::list<Network::Address::InstanceConstSharedPtr>& address_list);
  void runAddUpdateCallbacks(const std::string& host, const SharedHostInfoSharedPtr& host_info);
  void updateTlsHostsMap();

  Event::Dispatcher& main_thread_dispatcher_;
  Network::DnsResolverSharedPtr resolver_;
  ThreadLocal::SlotPtr tls_slot_;
  std::list<UpdateCallbacks*> update_callbacks_;
  absl::flat_hash_map<std::string, PrimaryHostInfoPtr> primary_hosts_;
};

} // namespace DynamicForwardProxy
} // namespace Common
} // namespace Extensions
} // namespace Envoy
