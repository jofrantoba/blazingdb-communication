#ifndef BLAZINGDB_UC_CONTEXT_HPP_
#define BLAZINGDB_UC_CONTEXT_HPP_

#include <memory>
#include <string>
#include <vector>

#include <blazingdb/uc/Agent.hpp>
#include <blazingdb/uc/Trader.hpp>

namespace blazingdb {
namespace uc {

/// \brief Scope for agents and buffers
///
/// A object of this type specify the kind of transport (Copy, IPC, GDR)
/// and the way to share the serialized addresses using a concerete `Trader`
class Context {
public:
  /// \brief Create an agent for own buffers
  virtual std::unique_ptr<Agent>
  OwnAgent() const = 0;

  /// \brief Create an agent for peer buffers
  ///
  /// In the case of IPC and GDR, this is a remote agent descriptor
  virtual std::unique_ptr<Agent>
  PeerAgent() const = 0;

  // ----------------------------------------------------------------------
  // Builders
  using Builder = std::unique_ptr<Context>(const Trader &);

  static std::unique_ptr<Context>
  Copy(const Trader &trader);

  static std::unique_ptr<Context>
  IPC(const Trader &trader);

  static std::unique_ptr<Context>
  GDR(const Trader &trader);

  static std::unique_ptr<Context>
  IPC();

  // ----------------------------------------------------------------------
  // List machine info about UCX valid interfaces

  class Capability {
  public:
    const std::string memoryModel;
    const std::string transportLayer;  // should be a collection
    const std::string deviceName;      // should be a collection
  };

  static std::vector<Capability>
  LookupCapabilities() noexcept;

  UC_INTERFACE(Context);
};

}  // namespace uc
}  // namespace blazingdb

#endif
