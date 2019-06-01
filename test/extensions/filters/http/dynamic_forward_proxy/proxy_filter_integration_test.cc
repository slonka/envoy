#include "envoy/config/cluster/dynamic_forward_proxy/v2alpha/cluster.pb.h" // fixfix yaml config

#include "test/integration/http_integration.h"

// fixfix h1 and h2 tests.

namespace Envoy {
namespace {

class ProxyFilterIntegrationTest : public testing::TestWithParam<Network::Address::IpVersion>,
                                   public HttpIntegrationTest {
public:
  ProxyFilterIntegrationTest() : HttpIntegrationTest(Http::CodecClient::Type::HTTP1, GetParam()) {}

  void SetUp() override {
    setUpstreamProtocol(FakeHttpConnection::Type::HTTP1);

    const std::string filter =
        R"EOF(
name: envoy.filters.http.dynamic_forward_proxy
config: {}
            )EOF";
    config_helper_.addFilter(filter);

    config_helper_.addConfigModifier([](envoy::config::bootstrap::v2::Bootstrap& bootstrap) {
      auto* cluster_0 = bootstrap.mutable_static_resources()->mutable_clusters(0);
      cluster_0->clear_hosts();
      cluster_0->set_lb_policy(envoy::api::v2::Cluster::CLUSTER_PROVIDED);
      auto* cluster_type = cluster_0->mutable_cluster_type();
      cluster_type->set_name("envoy.clusters.dynamic_forward_proxy");
      envoy::config::cluster::dynamic_forward_proxy::v2alpha::ClusterConfig config;
      cluster_type->mutable_typed_config()->PackFrom(config);
    });

    HttpIntegrationTest::initialize();
  }
};

INSTANTIATE_TEST_SUITE_P(IpVersions, ProxyFilterIntegrationTest,
                         testing::ValuesIn(TestEnvironment::getIpVersionsForTest()),
                         TestUtility::ipTestParamsToString);

// fixfix
TEST_P(ProxyFilterIntegrationTest, RequestWithBody) {
  codec_client_ = makeHttpConnection(lookupPort("http"));
  Http::TestHeaderMapImpl request_headers{{":method", "POST"},
                                          {":path", "/test/long/url"},
                                          {":scheme", "http"},
                                          {":authority", "localhost"}};
  auto response =
      sendRequestAndWaitForResponse(request_headers, 1024, default_response_headers_, 1024);
  checkSimpleRequestSuccess(1024, 1024, response.get());
}

} // namespace
} // namespace Envoy
