#ifndef BLAZINGDB_COMMUNICATION_NETWORK_CLIENT_H_
#define BLAZINGDB_COMMUNICATION_NETWORK_CLIENT_H_

#include <exception>
#include <memory>

#include <blazingdb/communication/messages/Message.h>
#include <blazingdb/communication/Node.h>
#include <blazingdb/communication/network/Status.h>

namespace blazingdb {
namespace communication {
namespace network {

class Client {
public:
  class SendError;

  virtual std::shared_ptr<Status> Send(const Node &node,
                                       const std::string &endpoint,
                                       const std::string &data,
                                       const std::string &buffer) /*const*/
      = 0;

  virtual std::shared_ptr<Status> Send(const Node &node,
                                       const std::string &endpoint,
                                       const messages::Message &message) = 0;

    virtual std::shared_ptr<Status> send(const Node& node,
                                         std::shared_ptr<messages::Message>& message) = 0;

  virtual std::shared_ptr<Status> SendNodeData(const std::string &ip,
                                               const std::uint16_t port,
                                               const messages::Message &message) = 0;


  static std::unique_ptr<Client> Make();
};

}  // namespace network
}  // namespace communication
}  // namespace blazingdb

#endif
