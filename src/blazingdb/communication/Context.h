#ifndef BLAZINGDB_COMMUNICATION_CONTEXT_H_
#define BLAZINGDB_COMMUNICATION_CONTEXT_H_

#include <vector>
#include <blazingdb/communication/ContextToken.h>
#include <blazingdb/communication/Node.h>

namespace blazingdb {
namespace communication {

class Context {
public:
  explicit Context(const std::vector<Node> taskNodes, const Node& masterNode,
                   const std::string logicalPlan,
                   const std::vector<std::string> sourceDataFiles);
  std::vector<Node> getAllNodes() const;
  std::vector<Node> getSiblingsNodes() const;
  const Node& getMasterNode() const;
  std::string getLogicalPlan() const;
  std::vector<std::string> getDataFiles() const;

private:
  const ContextToken token_;
  const std::vector<Node> taskNodes_;
  const Node* masterNode_;
  const std::string logicalPlan_;
  const std::vector<std::string> sourceDataFiles_;
};

}  // namespace communication
}  // namespace blazingdb

#endif
