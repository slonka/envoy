#include "extensions/filters/http/dynamic_forward_proxy/proxy_filter.h"

#include "extensions/common/dynamic_forward_proxy/dns_cache.h"

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace DynamicForwardProxy {

ProxyFilterConfig::ProxyFilterConfig(
    const envoy::config::filter::http::dynamic_forward_proxy::v2alpha::FilterConfig& proto_config,
    Singleton::Manager& singleton_manager, Event::Dispatcher& main_thread_dispatcher,
    ThreadLocal::SlotAllocator& tls)
    : dns_cache_manager_(Common::DynamicForwardProxy::getCacheManager(singleton_manager,
                                                                      main_thread_dispatcher, tls)),
      dns_cache_(dns_cache_manager_->getCache(proto_config.dns_cache_config())) {}

Http::FilterHeadersStatus ProxyFilter::decodeHeaders(Http::HeaderMap& headers, bool) {
  // fixfix handle port in host header.
  cache_load_handle_ =
      config_->cache().loadDnsCache(headers.Host()->value().getStringView(), *this);
  if (cache_load_handle_ == nullptr) {
    ASSERT(false); // fixfix
  }

  return Http::FilterHeadersStatus::StopAllIterationAndWatermark;
}

void ProxyFilter::onLoadDnsCacheComplete() { decoder_callbacks_->continueDecoding(); }

} // namespace DynamicForwardProxy
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
