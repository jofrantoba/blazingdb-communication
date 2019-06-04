#include "Node.h"

using namespace blazingdb::communication;

Node::Node() : address_{}, unixSocketId_{0}, isAvailable_{false} {}

Node::Node(const std::shared_ptr<Address>& address)
    : address_{address}, unixSocketId_{0}, isAvailable_{true} {}

Node::Node(int unixSocketId, const std::shared_ptr<Address>& address)
    : address_{address}, unixSocketId_{unixSocketId}, isAvailable_{true} {}

bool
Node::operator==(const Node& rhs) const {
  return address_->SameValueAs(*rhs.address_);
}

const Address*
Node::address() const noexcept {
  return address_.get();
}

bool
Node::isAvailable() const {
  return isAvailable_;
}

void
Node::setAvailable(bool available) {
  isAvailable_ = available;
}

int
Node::unixSocketId() const {
  return unixSocketId_;
}

void
Node::serializeToJson(JsonSerializable::Writer& writer) const {
  writer.Key("node");
  writer.StartObject();
  {
    writer.Key("unixSocketId");
    writer.Int(unixSocketId_);
    address_->serializeToJson(writer);
  }
  writer.EndObject();
}

Node
Node::make(const rapidjson::Value::Object& object) {
  return Node{object["unixSocketId"].GetInt(), Address::Make(object)};
}

std::unique_ptr<Node>
Node::makeUnique(const rapidjson::Value::Object& object) {
  return std::make_unique<Node>(object["unixSocketId"].GetInt(),
                                Address::Make(object));
}

#include "Address-Internal.h"

#include <algorithm>

namespace blazingdb {
namespace communication {

namespace {
class NodeBuffer : public Buffer {
public:
  explicit NodeBuffer(const std::string& nodeAsString)
      :  nodeAsString_{nodeAsString}, Buffer{nodeAsString_.data(), nodeAsString_.size()} {
        // data_ = const_cast<char *>(nodeAsString_.data());
        // size_ = nodeAsString_.size();
  }

  const char*
  data() const noexcept {
    return nodeAsString_.data();
  }
  std::size_t
  size() const noexcept {
    return nodeAsString_.size();
  }

private:
  const std::string nodeAsString_;
};

class ConcreteNode : public Node {
public:
  explicit ConcreteNode(const std::shared_ptr<Address>& address)
      : Node{address} {}

  explicit ConcreteNode(const Buffer& buffer)
      : Node{ConcreteAddressFrom(buffer)} {}

  explicit ConcreteNode(int                             unixSocketId,
                        const std::shared_ptr<Address>& address)
      : Node{unixSocketId, address} {}

  const std::shared_ptr<Buffer>
  ToBuffer() const noexcept {
    const internal::ConcreteAddress& concreteAddress =
        *static_cast<const internal::ConcreteAddress*>(address());

    const std::string nodeAsString =
        concreteAddress.ip() + "," + std::to_string(concreteAddress.port());

    return std::make_shared<NodeBuffer>(std::move(nodeAsString));
  }

private:
  static std::shared_ptr<Address>
  ConcreteAddressFrom(const Buffer& buffer) {
    // TODO: change to istream or avoid string
    const char* it =
        std::find(buffer.data(), buffer.data() + buffer.size(), ',');

    if (it == buffer.data() + buffer.size()) {
      throw std::runtime_error("Bad buffer");
    }

    const std::string   ip{buffer.data(), it};
    const std::uint16_t port = std::atoi(++it);

    return std::make_shared<internal::ConcreteAddress>(ip, port);
  }
};
}  // namespace

std::shared_ptr<Node>
Node::Make(const std::shared_ptr<Address>& address) {
  return std::make_shared<ConcreteNode>(address);
}

std::shared_ptr<Node>
Node::Make(const Buffer& buffer) {
  return std::make_shared<ConcreteNode>(buffer);
}

bool
operator!=(const Node& lhs, const Node& rhs) {
  return !(lhs.address()->SameValueAs(*rhs.address()));
}

std::shared_ptr<Node>
Node::makeShared(int unixSocketId, std::string&& ip, int16_t port) {
  return std::make_shared<ConcreteNode>(unixSocketId, Address::Make(ip, port));
}

std::shared_ptr<Node>
Node::makeShared(int unixSocketId, const std::string& ip, int16_t port) {
  return std::make_shared<ConcreteNode>(unixSocketId, Address::Make(ip, port));
}

std::shared_ptr<Node>
Node::makeShared(const Node& node) {
  const internal::ConcreteAddress& concreteAddress =
      *static_cast<const internal::ConcreteAddress*>(node.address());
  return std::make_shared<ConcreteNode>(
      node.unixSocketId(),
      Address::Make(concreteAddress.ip(), concreteAddress.port()));
}

std::unique_ptr<Node>
Node::make(int unixSocketId, const std::string& ip, int16_t port) {
  return std::unique_ptr<Node>(new Node(unixSocketId, Address::Make(ip, port)));
}

}  // namespace communication
}  // namespace blazingdb
