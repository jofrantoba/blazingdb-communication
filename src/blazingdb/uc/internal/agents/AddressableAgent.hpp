#ifndef BLAZINGDB_UC_INTERNAL_AGENTS_ADDRESSABLE_AGENT_HPP_
#define BLAZINGDB_UC_INTERNAL_AGENTS_ADDRESSABLE_AGENT_HPP_

#include <blazingdb/uc/Agent.hpp>

#include <uct/api/uct_def.h>

namespace blazingdb {
namespace uc {
class Trader;
namespace internal {

class AddressableAgent : public Agent {
public:
  explicit AddressableAgent(const uct_md_h &     md,
                            const uct_md_attr_t &md_attr,
                            const Trader &       trader);

  std::unique_ptr<Buffer>
  Register(const void *const data, const std::size_t size) const noexcept final;

private:
  const uct_md_h &     md_;
  const uct_md_attr_t &md_attr_;
  const Trader &       trader_;
};

}  // namespace internal
}  // namespace uc
}  // namespace blazingdb

#endif