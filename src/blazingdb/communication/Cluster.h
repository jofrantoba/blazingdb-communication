#ifndef BLAZINGDB_COMMUNICATION_CLUSTER_H_
#define BLAZINGDB_COMMUNICATION_CLUSTER_H_

#include <vector>
#include <memory>
#include <mutex>
#include <blazingdb/communication/Node.h>

namespace blazingdb {
namespace communication {

class Cluster {
public:
  explicit Cluster() = default;
  void addNode(const Node& node);
  size_t getTotalNodes() const;
  std::vector<Node*> getAvailableNodes() const;

private:
  std::vector<std::unique_ptr<Node>> nodes_;
  std::mutex condition_mutex;
};

}  // namespace communication
}  // namespace blazingdb

#endif
