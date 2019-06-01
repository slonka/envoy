#include "extensions/common/dynamic_forward_proxy/dns_cache_impl.h"

#include "test/mocks/network/mocks.h"
#include "test/mocks/thread_local/mocks.h"

using testing::InSequence;
using testing::Return;
using testing::SaveArg;

// fixfix test inline resolve.

namespace Envoy {
namespace Extensions {
namespace Common {
namespace DynamicForwardProxy {
namespace {

std::list<Network::Address::InstanceConstSharedPtr>
makeAddressList(const std::list<std::string> address_list) {
  std::list<Network::Address::InstanceConstSharedPtr> ret;
  for (const auto& address : address_list) {
    ret.emplace_back(Network::Utility::parseInternetAddress(address));
  }
  return ret;
}

class MockUpdateCallbacks : public DnsCache::UpdateCallbacks {
public:
  MOCK_METHOD2(onDnsHostAddOrUpdate,
               void(const std::string& host, const DnsCache::SharedHostInfoSharedPtr& address));
  MOCK_METHOD1(onDnsHostRemove, void(const std::string& host));
};

class DnsCacheImplTest : public testing::Test {
public:
  DnsCacheImplTest() {
    EXPECT_CALL(dispatcher_, createDnsResolver(_)).WillOnce(Return(resolver_));
    dns_cache_ = std::make_unique<DnsCacheImpl>(dispatcher_, tls_);
    update_callbacks_handle_ = dns_cache_->addUpdateCallbacks(update_callbacks_);
  }

  NiceMock<Event::MockDispatcher> dispatcher_;
  std::shared_ptr<Network::MockDnsResolver> resolver_{std::make_shared<Network::MockDnsResolver>()};
  NiceMock<ThreadLocal::MockInstance> tls_;
  std::unique_ptr<DnsCache> dns_cache_;
  MockUpdateCallbacks update_callbacks_;
  DnsCache::AddUpdateCallbacksHandlePtr update_callbacks_handle_;
};

class MockLoadDnsCacheCallbacks : public DnsCache::LoadDnsCacheCallbacks {
public:
  MOCK_METHOD0(onLoadDnsCacheComplete, void());
};

// fixfix
TEST_F(DnsCacheImplTest, ResolveSuccess) {
  InSequence s;

  MockLoadDnsCacheCallbacks callbacks;
  Network::DnsResolver::ResolveCb resolve_cb;
  EXPECT_CALL(*resolver_, resolve("foo.com", _, _))
      .WillOnce(DoAll(SaveArg<2>(&resolve_cb), Return(&resolver_->active_query_)));
  DnsCache::LoadDnsCacheHandlePtr handle = dns_cache_->loadDnsCache("foo.com", callbacks);
  EXPECT_NE(handle, nullptr);

  EXPECT_CALL(callbacks, onLoadDnsCacheComplete());
  resolve_cb(makeAddressList({"10.0.0.1"}));
}

// fixfix
TEST_F(DnsCacheImplTest, ResolveFailure) {
  InSequence s;

  MockLoadDnsCacheCallbacks callbacks;
  Network::DnsResolver::ResolveCb resolve_cb;
  EXPECT_CALL(*resolver_, resolve("foo.com", _, _))
      .WillOnce(DoAll(SaveArg<2>(&resolve_cb), Return(&resolver_->active_query_)));
  DnsCache::LoadDnsCacheHandlePtr handle = dns_cache_->loadDnsCache("foo.com", callbacks);
  EXPECT_NE(handle, nullptr);

  EXPECT_CALL(callbacks, onLoadDnsCacheComplete());
  resolve_cb(makeAddressList({}));
}

// fixfix
TEST_F(DnsCacheImplTest, CancelResolve) {
  InSequence s;

  MockLoadDnsCacheCallbacks callbacks;
  Network::DnsResolver::ResolveCb resolve_cb;
  EXPECT_CALL(*resolver_, resolve("foo.com", _, _))
      .WillOnce(DoAll(SaveArg<2>(&resolve_cb), Return(&resolver_->active_query_)));
  DnsCache::LoadDnsCacheHandlePtr handle = dns_cache_->loadDnsCache("foo.com", callbacks);
  EXPECT_NE(handle, nullptr);

  handle.reset();
  resolve_cb(makeAddressList({"10.0.0.1"}));
}

// fixfix
TEST_F(DnsCacheImplTest, MultipleResolveSameHost) {
  InSequence s;

  MockLoadDnsCacheCallbacks callbacks1;
  Network::DnsResolver::ResolveCb resolve_cb;
  EXPECT_CALL(*resolver_, resolve("foo.com", _, _))
      .WillOnce(DoAll(SaveArg<2>(&resolve_cb), Return(&resolver_->active_query_)));
  DnsCache::LoadDnsCacheHandlePtr handle1 = dns_cache_->loadDnsCache("foo.com", callbacks1);
  EXPECT_NE(handle1, nullptr);

  MockLoadDnsCacheCallbacks callbacks2;
  DnsCache::LoadDnsCacheHandlePtr handle2 = dns_cache_->loadDnsCache("foo.com", callbacks2);
  EXPECT_NE(handle2, nullptr);

  EXPECT_CALL(callbacks2, onLoadDnsCacheComplete());
  EXPECT_CALL(callbacks1, onLoadDnsCacheComplete());
  resolve_cb(makeAddressList({"10.0.0.1"}));
}

// fixfix
TEST_F(DnsCacheImplTest, MultipleResolveDifferentHost) {
  InSequence s;

  MockLoadDnsCacheCallbacks callbacks1;
  Network::DnsResolver::ResolveCb resolve_cb1;
  EXPECT_CALL(*resolver_, resolve("foo.com", _, _))
      .WillOnce(DoAll(SaveArg<2>(&resolve_cb1), Return(&resolver_->active_query_)));
  DnsCache::LoadDnsCacheHandlePtr handle1 = dns_cache_->loadDnsCache("foo.com", callbacks1);
  EXPECT_NE(handle1, nullptr);

  MockLoadDnsCacheCallbacks callbacks2;
  Network::DnsResolver::ResolveCb resolve_cb2;
  EXPECT_CALL(*resolver_, resolve("bar.com", _, _))
      .WillOnce(DoAll(SaveArg<2>(&resolve_cb2), Return(&resolver_->active_query_)));
  DnsCache::LoadDnsCacheHandlePtr handle2 = dns_cache_->loadDnsCache("bar.com", callbacks2);
  EXPECT_NE(handle2, nullptr);

  EXPECT_CALL(callbacks2, onLoadDnsCacheComplete());
  resolve_cb2(makeAddressList({"10.0.0.1"}));

  EXPECT_CALL(callbacks1, onLoadDnsCacheComplete());
  resolve_cb1(makeAddressList({"10.0.0.2"}));
}

// fixfix
TEST_F(DnsCacheImplTest, CacheHit) {
  InSequence s;

  MockLoadDnsCacheCallbacks callbacks;
  Network::DnsResolver::ResolveCb resolve_cb;
  EXPECT_CALL(*resolver_, resolve("foo.com", _, _))
      .WillOnce(DoAll(SaveArg<2>(&resolve_cb), Return(&resolver_->active_query_)));
  DnsCache::LoadDnsCacheHandlePtr handle = dns_cache_->loadDnsCache("foo.com", callbacks);
  EXPECT_NE(handle, nullptr);

  EXPECT_CALL(callbacks, onLoadDnsCacheComplete());
  resolve_cb(makeAddressList({"10.0.0.1"}));

  EXPECT_EQ(nullptr, dns_cache_->loadDnsCache("foo.com", callbacks));
}

} // namespace
} // namespace DynamicForwardProxy
} // namespace Common
} // namespace Extensions
} // namespace Envoy
