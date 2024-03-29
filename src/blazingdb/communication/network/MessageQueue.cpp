#include "blazingdb/communication/network/MessageQueue.h"
#include <iostream>
#include <algorithm>

namespace blazingdb {
namespace communication {
namespace network {

std::shared_ptr<Message> MessageQueue::getMessage(const MessageTokenType& messageToken) {
    
    
    
    std::unique_lock<std::mutex> lock(mutex_);
    
    std::cout << "TF1: " << messageToken << std::endl;
    std::cout << "TF1: " << this->message_queue_.size() << std::endl;

    condition_variable_.wait(lock, 
                            [&, this]{ return std::any_of(this->message_queue_.cbegin(),
                                                        this->message_queue_.cend(), 
                                                        [&](const auto& e){ 
            std::cout << "TF2: " << e->getMessageTokenValue() << std::endl;
            return e->getMessageTokenValue() == messageToken; }); 
    });
    return getMessageQueue(messageToken);
}

void MessageQueue::putMessage(std::shared_ptr<Message>& message) {
    std::unique_lock<std::mutex> lock(mutex_);
    putMessageQueue(message);
    condition_variable_.notify_one();
}

std::shared_ptr<Message> MessageQueue::getMessageQueue(const MessageTokenType& messageToken) {
    auto it = std::partition(message_queue_.begin(), message_queue_.end(),
                            [&messageToken](const auto& e){ return e->getMessageTokenValue() != messageToken; });

    std::shared_ptr<Message> message;
    if (it != message_queue_.end()) {
        // This should always execute because there will be always at least one message
        // with the messageToken requested due to the conditional variable.
        message = *it;
        message_queue_.erase(it, it + 1);
    }
    
    return message;
}

void MessageQueue::putMessageQueue(std::shared_ptr<Message>& message) {
    message_queue_.push_back(message);
}

}  // namespace network
}  // namespace communication
}  // namespace blazingdb
