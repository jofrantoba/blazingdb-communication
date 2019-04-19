#ifndef BLAZINGDB_UC_INTERNAL_BUFFERS_LINKER_BUFFER_HPP_
#define BLAZINGDB_UC_INTERNAL_BUFFERS_LINKER_BUFFER_HPP_

#include "AccessibleBuffer.hpp"
#include "RemoteBuffer.hpp"
#include "transports/ZCopyTransport.hpp"

namespace blazingdb {
namespace uc {
namespace internal {

class UC_NOEXPORT LinkerBuffer : public AccessibleBuffer {
public:
  explicit LinkerBuffer(const void *const          pointer,
                        const std::size_t          size,
                        const uct_ep_h &           ep,
                        const ucs_async_context_t &async_context,
                        const uct_worker_h &       worker,
                        const uct_iface_h &        iface)
      : AccessibleBuffer{pointer, size},
        ep_{ep},
        async_context_{async_context},
        worker_{worker},
        iface_{iface} {}

  std::unique_ptr<Transport>
  Link(Buffer *buffer) const final {
    auto remoteBuffer = dynamic_cast<RemoteBuffer *>(buffer);
    if (nullptr == remoteBuffer) {
      throw std::runtime_error(
          "Bad buffer. Use a buffer created by a peer agent");
    }
    remoteBuffer->Fetch(pointer(), mem());
    return std::make_unique<ZCopyTransport>(*this,
                                            *remoteBuffer,
                                            ep_,
                                            remoteBuffer->md_attr(),
                                            async_context_,
                                            worker_,
                                            iface_);
  }

private:
  const uct_ep_h &           ep_;
  const ucs_async_context_t &async_context_;
  const uct_worker_h &       worker_;
  const uct_iface_h &        iface_;
};

}  // namespace internal
}  // namespace uc
}  // namespace blazingdb

#endif
