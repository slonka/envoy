#pragma once

#include "envoy/config/cluster/dynamic_forward_proxy/v2alpha/cluster.pb.h"
#include "envoy/config/cluster/dynamic_forward_proxy/v2alpha/cluster.pb.validate.h"

#include "common/upstream/cluster_factory_impl.h"

#include "extensions/clusters/well_known_names.h"
#include "extensions/common/dynamic_forward_proxy/dns_cache.h"

namespace Envoy {
namespace Extensions {
namespace Clusters {
namespace DynamicForwardProxy {

class Cluster : public Upstream::BaseDynamicClusterImpl,
                public Extensions::Common::DynamicForwardProxy::DnsCache::UpdateCallbacks {
public:
  Cluster(const envoy::api::v2::Cluster& cluster,
          const envoy::config::cluster::dynamic_forward_proxy::v2alpha::ClusterConfig& config,
          Runtime::Loader& runtime, Singleton::Manager& singleton_manager,
          Event::Dispatcher& main_thread_dispatcher, ThreadLocal::SlotAllocator& tls,
          Server::Configuration::TransportSocketFactoryContext& factory_context,
          Stats::ScopePtr&& stats_scope, bool added_via_api);

  // Upstream::Cluster
  Upstream::Cluster::InitializePhase initializePhase() const override {
    return Upstream::Cluster::InitializePhase::Primary;
  }

  // Upstream::ClusterImplBase
  void startPreInit() override {
    // Nothing to do during initialization.
    onPreInitComplete();
  }

  // Extensions::Common::DynamicForwardProxy::DnsCache::UpdateCallbacks
  void onDnsHostAddOrUpdate(
      const std::string& host,
      const Extensions::Common::DynamicForwardProxy::DnsCache::SharedHostInfoSharedPtr& host_info)
      override;
  void onDnsHostRemove(const std::string& host) override;

private:
  struct LoadBalancer : public Upstream::LoadBalancer {
    Upstream::HostConstSharedPtr chooseHost(Upstream::LoadBalancerContext* /*context*/) override {
      ASSERT(false); // fixfix
    }
  };

  struct LoadBalancerFactory : public Upstream::LoadBalancerFactory {
    LoadBalancerFactory(Cluster& cluster) : cluster_(cluster) {}

    // Upstream::LoadBalancerFactory
    Upstream::LoadBalancerPtr create() override { return std::make_unique<LoadBalancer>(); }

    Cluster& cluster_;
  };

  struct ThreadAwareLoadBalancer : public Upstream::ThreadAwareLoadBalancer {
    ThreadAwareLoadBalancer(Cluster& cluster) : cluster_(cluster) {}

    // Upstream::ThreadAwareLoadBalancer
    Upstream::LoadBalancerFactorySharedPtr factory() override {
      return std::make_shared<LoadBalancerFactory>(cluster_);
    }
    void initialize() override {}

    Cluster& cluster_;
  };

  const Extensions::Common::DynamicForwardProxy::DnsCacheManagerSharedPtr dns_cache_manager_;
  const Extensions::Common::DynamicForwardProxy::DnsCacheSharedPtr dns_cache_;
  const Extensions::Common::DynamicForwardProxy::DnsCache::AddUpdateCallbacksHandlePtr
      update_callbacks_handle_;

  friend class ClusterFactory;
};

class ClusterFactory : public Upstream::ConfigurableClusterFactoryBase<
                           envoy::config::cluster::dynamic_forward_proxy::v2alpha::ClusterConfig> {
public:
  ClusterFactory()
      : ConfigurableClusterFactoryBase(
            Extensions::Clusters::ClusterTypes::get().DynamicForwardProxy) {}

private:
  std::pair<Upstream::ClusterImplBaseSharedPtr, Upstream::ThreadAwareLoadBalancerPtr>
  createClusterWithConfig(
      const envoy::api::v2::Cluster& cluster,
      const envoy::config::cluster::dynamic_forward_proxy::v2alpha::ClusterConfig& proto_config,
      Upstream::ClusterFactoryContext& context,
      Server::Configuration::TransportSocketFactoryContext& socket_factory_context,
      Stats::ScopePtr&& stats_scope) override;
};

} // namespace DynamicForwardProxy
} // namespace Clusters
} // namespace Extensions
} // namespace Envoy
