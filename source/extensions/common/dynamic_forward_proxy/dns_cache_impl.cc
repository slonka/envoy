#include "extensions/common/dynamic_forward_proxy/dns_cache_impl.h"

namespace Envoy {
namespace Extensions {
namespace Common {
namespace DynamicForwardProxy {

// fixfix TTL
// fixfix re-resolve
// fixfix stats

DnsCacheImpl::DnsCacheImpl(Event::Dispatcher& main_thread_dispatcher,
                           ThreadLocal::SlotAllocator& tls)
    : main_thread_dispatcher_(main_thread_dispatcher),
      resolver_(main_thread_dispatcher.createDnsResolver({})), tls_slot_(tls.allocateSlot()) {
  tls_slot_->set([](Event::Dispatcher&) { return std::make_shared<ThreadLocalHostInfo>(); });
  updateTlsHostsMap();
}

DnsCacheImpl::~DnsCacheImpl() {
  // fixfix cancel active queries.
}

DnsCacheImpl::LoadDnsCacheHandlePtr DnsCacheImpl::loadDnsCache(absl::string_view host,
                                                               LoadDnsCacheCallbacks& callbacks) {
  ENVOY_LOG(debug, "thread local lookup for DNS '{}'", host);
  auto& tls_host_info = tls_slot_->getTyped<ThreadLocalHostInfo>();
  auto tls_host = tls_host_info.host_map_->find(host);
  if (tls_host != tls_host_info.host_map_->end()) {
    ENVOY_LOG(debug, "thread local hit for DNS '{}'", host);
    return nullptr;
  } else {
    ENVOY_LOG(debug, "thread local miss for DNS '{}', posting to main thread", host);
    main_thread_dispatcher_.post([this, host = std::string(host)]() { startResolve(host); });
    return std::make_unique<LoadDnsCacheHandleImpl>(tls_host_info.pending_resolutions_, host,
                                                    callbacks);
  }
}

DnsCacheImpl::AddUpdateCallbacksHandlePtr
DnsCacheImpl::addUpdateCallbacks(UpdateCallbacks& callbacks) {
  return std::make_unique<AddUpdateCallbacksHandleImpl>(update_callbacks_, callbacks);
}

void DnsCacheImpl::startResolve(const std::string& host) {
  // fixfix
  auto& primary_host = primary_hosts_[host];
  if (primary_host != nullptr) {
    ENVOY_LOG(debug, "main thread resolve for DNS '{}' skipped. Entry present", host);
    return;
  }

  ENVOY_LOG(debug, "starting main thread resolve for DNS '{}'", host);
  primary_host = std::make_unique<PrimaryHostInfo>();
  primary_host->active_query_ = resolver_->resolve(
      host, Network::DnsLookupFamily::Auto,
      [this, &primary_host = *primary_host](
          const std::list<Network::Address::InstanceConstSharedPtr>&& address_list) {
        finishResolve(primary_host, address_list);
      });
}

void DnsCacheImpl::finishResolve(
    PrimaryHostInfo& primary_host_info,
    const std::list<Network::Address::InstanceConstSharedPtr>& address_list) {
  ENVOY_LOG(debug, "main thread resolve complete for DNS '{}'. {} results", "fixfix",
            address_list.size());
  ASSERT(primary_host_info.info_ == nullptr); // fixfix handle re-resolve.
  primary_host_info.active_query_ = nullptr;
  primary_host_info.info_ = std::make_shared<SharedHostInfo>();
  primary_host_info.info_->address_ = !address_list.empty() ? address_list.front() : nullptr;
  runAddUpdateCallbacks("fixfix", primary_host_info.info_);
  updateTlsHostsMap();
}

void DnsCacheImpl::runAddUpdateCallbacks(const std::string& host,
                                         const SharedHostInfoSharedPtr& host_info) {
  for (auto callbacks : update_callbacks_) {
    callbacks->onDnsHostAddOrUpdate(host, host_info);
  }
}

void DnsCacheImpl::updateTlsHostsMap() {
  TlsHostMapSharedPtr new_host_map = std::make_shared<TlsHostMap>();
  for (const auto& primary_host : primary_hosts_) {
    // Do not include hosts that are new and have not yet been resolved.
    if (primary_host.second->info_ != nullptr) {
      new_host_map->emplace(primary_host.first, primary_host.second->info_);
    }
  }

  // fixfix cleanup
  tls_slot_->runOnAllThreads([this, new_host_map]() {
    auto& tls_host_info = tls_slot_->getTyped<ThreadLocalHostInfo>();
    tls_host_info.host_map_ = new_host_map;
    for (auto pending_resolution = tls_host_info.pending_resolutions_.begin();
         pending_resolution != tls_host_info.pending_resolutions_.end();) {
      if (tls_host_info.host_map_->count((*pending_resolution)->host_) != 0) {
        auto& callbacks = (*pending_resolution)->callbacks_;
        (*pending_resolution)->cancel();
        pending_resolution = tls_host_info.pending_resolutions_.erase(pending_resolution);
        callbacks.onLoadDnsCacheComplete();
      } else {
        ++pending_resolution;
      }
    }
  });

  // fixfix run update callbacks.
}

} // namespace DynamicForwardProxy
} // namespace Common
} // namespace Extensions
} // namespace Envoy
