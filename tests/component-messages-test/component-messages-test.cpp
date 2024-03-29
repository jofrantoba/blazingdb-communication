#include <gtest/gtest.h>
#include "blazingdb/communication/messages/DataPivot.h"
#include "blazingdb/communication/messages/DataScatterMessage.h"
#include "blazingdb/communication/messages/PartitionPivotsMessage.h"
#include "blazingdb/communication/messages/SampleToNodeMasterMessage.h"
#include "blazingdb/communication/messages/ColumnDataMessage.h"
#include "blazingdb/communication/messages/NodeDataMessage.h"
#include "tests/utils/gdf_column.h"
#include "tests/utils/gdf_column_cpp.h"

struct ComponentMessagesTest : public testing::Test {
    ComponentMessagesTest() {
    }

    ~ComponentMessagesTest() {
    }

    void SetUp() override {
    }

    void TearDown() override {
    }
};

struct GpuFunctions {
    using DType = blazingdb::test::gdf_dtype;
    using DTypeInfo = blazingdb::test::gdf_dtype_extra_info;
    using TimeUnit = blazingdb::test::gdf_time_unit;

    using DataTypePointer = void*;
    using ValidTypePointer = blazingdb::test::gdf_valid_type*;
    
    using NvStrings = NVStrings;
    using NvCategory = NVCategory;


    static void copyGpuToCpu(std::size_t& binary_pointer, std::string& result, blazingdb::test::gdf_column_cpp& column) {
        std::size_t data_size = getDataCapacity(column.get_gdf_column());
        std::memcpy(&result[binary_pointer], column.get_gdf_column()->data, data_size);
        binary_pointer += data_size;

        std::size_t valid_size = getValidCapacity(column.get_gdf_column());
        std::memcpy(&result[binary_pointer], column.get_gdf_column()->valid, valid_size);
        binary_pointer += valid_size;
    }

    static std::size_t getDataCapacity(blazingdb::test::gdf_column* column) {
        return column->size;
    }

    static std::size_t getValidCapacity(blazingdb::test::gdf_column* column) {
        return column->size * getDTypeSize(column->dtype);
    }

    static std::size_t getDTypeSize(blazingdb::test::gdf_dtype type) {
        return blazingdb::test::get_dtype_width(type);
    }

    static std::size_t isGdfString(blazingdb::test::gdf_column& column) {
      return false;
    }

    static NvStrings* CreateNvStrings(const void * stringsPointer,
                                    const void *      offsetsPointer,
                                    const std::size_t keysLength){
        return nullptr;
    }
    
    static void log(std::string message) {
    
    }
};


TEST_F(ComponentMessagesTest, ColumnDataMessage) {
    // Make alias
    using ContextToken = blazingdb::communication::ContextToken;
    using MessageToken = blazingdb::communication::messages::MessageToken;
    using ColumnDataMessage = blazingdb::communication::messages::ColumnDataMessage<blazingdb::test::gdf_column_cpp,
                                                                                    blazingdb::test::gdf_column,
                                                                                    GpuFunctions>;

    // Test data - create gdf_column_cpp
    auto gdf_column_1 = blazingdb::test::build(8,
                                               blazingdb::test::GDF_INT16,
                                               4,
                                               blazingdb::test::TIME_UNIT_ms,
                                               "column name 1");

    auto gdf_column_2 = blazingdb::test::build(16,
                                               blazingdb::test::GDF_INT32,
                                               8,
                                               blazingdb::test::TIME_UNIT_s,
                                               "column name 2");

    auto gdf_column_cpp_1 = blazingdb::test::build(gdf_column_1.get(),
                                                   gdf_column_1->size,
                                                   gdf_column_1->size * blazingdb::test::get_dtype_width(gdf_column_1->dtype),
                                                   "column name 1",
                                                   false,
                                                   123);

    auto gdf_column_cpp_2 = blazingdb::test::build(gdf_column_2.get(),
                                                   gdf_column_2->size,
                                                   gdf_column_2->size * blazingdb::test::get_dtype_width(gdf_column_2->dtype),
                                                   "column name 2",
                                                   false,
                                                   3567);

    std::vector<blazingdb::test::gdf_column_cpp> columns;
    columns.emplace_back(gdf_column_cpp_1);
    columns.emplace_back(gdf_column_cpp_2);

    // Test data - create node
    using Address = blazingdb::communication::Address;
    blazingdb::communication::Node node(Address::Make("1.2.3.4", 1234, 5678));

    // Test data - create context token
    const ContextToken::TokenType context_token_value = 2437;

    // Serialize data
    std::string json_data;
    std::string binary_data;

    // Serialize message
    {
        std::unique_ptr<MessageToken> message_token = MessageToken::Make(ColumnDataMessage::getMessageID());
        std::shared_ptr<ContextToken> context_token = ContextToken::Make(context_token_value);
        ColumnDataMessage message(std::move(message_token), std::move(context_token), node, columns);

        json_data = message.serializeToJson();
        binary_data = message.serializeToBinary();
    }

    // Deserialize message & test
    {
        std::shared_ptr<ColumnDataMessage> message = std::static_pointer_cast<ColumnDataMessage>
                (ColumnDataMessage::Make(json_data, binary_data));

        // Test context token
        ASSERT_EQ(context_token_value, message->getContextTokenValue());

        // Test message token
        ASSERT_EQ(ColumnDataMessage::getMessageID(), message->getMessageTokenValue());

        // Test message node
        ASSERT_EQ(node, message->getSenderNode());

        // Test columns
        const auto message_columns = message->getColumns();
        ASSERT_EQ(message_columns.size(), columns.size());
        for (std::size_t k = 0; k < columns.size(); ++k) {
            ASSERT_TRUE(message_columns[k] == columns[k]);
        }
    }
}


TEST_F(ComponentMessagesTest, DataScatterMessage) {
    // Test data - create gdf_column_cpp data
    auto gdf_column_1 = blazingdb::test::build(8,
                                               blazingdb::test::GDF_INT16,
                                               4,
                                               blazingdb::test::TIME_UNIT_ms,
                                               "column name 1");

    auto gdf_column_2 = blazingdb::test::build(16,
                                               blazingdb::test::GDF_INT32,
                                               8,
                                               blazingdb::test::TIME_UNIT_s,
                                               "column name 2");

    auto gdf_column_cpp_1 = blazingdb::test::build(gdf_column_1.get(),
                                                   gdf_column_1->size,
                                                   gdf_column_1->size * blazingdb::test::get_dtype_width(gdf_column_1->dtype),
                                                   "column name 1",
                                                   false,
                                                   123);

    auto gdf_column_cpp_2 = blazingdb::test::build(gdf_column_2.get(),
                                                   gdf_column_2->size,
                                                   gdf_column_2->size * blazingdb::test::get_dtype_width(gdf_column_2->dtype),
                                                   "column name 2",
                                                   false,
                                                   3567);

    std::vector<blazingdb::test::gdf_column_cpp> columns;
    columns.emplace_back(gdf_column_cpp_1);
    columns.emplace_back(gdf_column_cpp_2);

    // Make alias
    using ContextToken = blazingdb::communication::ContextToken;
    using DataScatterMessage = blazingdb::communication::messages::DataScatterMessage<blazingdb::test::gdf_column_cpp,
                                                                                      blazingdb::test::gdf_column,
                                                                                      GpuFunctions>;

    // Create context token
    const ContextToken::TokenType context_token = 2437;

    // Serialize data
    std::string json_data;
    std::string binary_data;

    // Serialize message
    {
        DataScatterMessage message(ContextToken::Make(context_token), columns);

        json_data = message.serializeToJson();
        binary_data = message.serializeToBinary();
    }

    // Deserialize message & test
    {
        std::shared_ptr<DataScatterMessage> message = std::static_pointer_cast<DataScatterMessage>
                (DataScatterMessage::Make(json_data, binary_data));

        // Test context token
        ASSERT_EQ(context_token, message->getContextTokenValue());

        // Test message token
        ASSERT_EQ(DataScatterMessage::getMessageID(), message->getMessageTokenValue());

        // Test columns
        const auto& message_columns = message->getColumns();
        ASSERT_EQ(message_columns.size(), columns.size());
        for (std::size_t k = 0; k < columns.size(); ++k) {
            ASSERT_TRUE(message_columns[k] == columns[k]);
        }
    }
}


TEST_F(ComponentMessagesTest, SampleToNodeMasterMessage) {
    // Make alias
    using ContextToken = blazingdb::communication::ContextToken;
    using MessageToken = blazingdb::communication::messages::MessageToken;
    using SampleToNodeMasterMessage = blazingdb::communication::messages::SampleToNodeMasterMessage<blazingdb::test::gdf_column_cpp,
                                                                                                    blazingdb::test::gdf_column,
                                                                                                    GpuFunctions>;

    // Test data - create samples
    auto gdf_column_1 = blazingdb::test::build(16,
                                               blazingdb::test::GDF_INT64,
                                               12,
                                               blazingdb::test::TIME_UNIT_ms,
                                               "column name 1");

    auto gdf_column_2 = blazingdb::test::build(24,
                                               blazingdb::test::GDF_INT32,
                                               5,
                                               blazingdb::test::TIME_UNIT_s,
                                               "column name 2");

    auto gdf_column_cpp_1 = blazingdb::test::build(gdf_column_1.get(),
                                                   gdf_column_1->size,
                                                   gdf_column_1->size * blazingdb::test::get_dtype_width(gdf_column_1->dtype),
                                                   "column name 1",
                                                   true,
                                                   123);

    auto gdf_column_cpp_2 = blazingdb::test::build(gdf_column_2.get(),
                                                   gdf_column_2->size,
                                                   gdf_column_2->size * blazingdb::test::get_dtype_width(gdf_column_2->dtype),
                                                   "column name 2",
                                                   false,
                                                   3567);

    std::vector<blazingdb::test::gdf_column_cpp> samples;
    samples.emplace_back(gdf_column_cpp_1);
    samples.emplace_back(gdf_column_cpp_2);

    // Test data - create sender node
    using Address = blazingdb::communication::Address;
    blazingdb::communication::Node sender_node(Address::Make("1.2.3.4", 1234, 5678));



    // Create context token
    const ContextToken::TokenType context_token_value = 6574;

    // Total data size
    const std::uint64_t total_row_size = 7659;

    // Serialize data
    std::string json_data;
    std::string binary_data;

    // Serialize message
    {
        std::unique_ptr<MessageToken> message_token = MessageToken::Make(SampleToNodeMasterMessage::getMessageID());
        std::shared_ptr<ContextToken> context_token = ContextToken::Make(context_token_value);
        SampleToNodeMasterMessage message(std::move(message_token), std::move(context_token), sender_node, total_row_size, samples);

        json_data = message.serializeToJson();
        binary_data = message.serializeToBinary();
    }

    // Deserialize message & test
    {
        std::shared_ptr<SampleToNodeMasterMessage> message = std::static_pointer_cast<SampleToNodeMasterMessage>
                (SampleToNodeMasterMessage::Make(json_data, binary_data));

        // Test context token
        ASSERT_EQ(message->getContextTokenValue(), context_token_value);

        // Test message token
        ASSERT_EQ(message->getMessageTokenValue(), SampleToNodeMasterMessage::getMessageID());

        // Test node
        ASSERT_EQ(message->getSenderNode(), sender_node);

        // Test total data size
        ASSERT_EQ(message->getTotalRowSize(), total_row_size);

        // Test samples
        std::vector<blazingdb::test::gdf_column_cpp> message_samples = message->getSamples();
        ASSERT_EQ(message_samples.size(), samples.size());
        for (std::size_t k = 0; k < samples.size(); ++k) {
            ASSERT_TRUE(message_samples[k] == samples[k]);
        }
    }
}


// TEST_F(ComponentMessagesTest, PartitionPivotsMessage) {

//     // Create Data - create nodes
//     using Address = blazingdb::communication::Address;
//     blazingdb::communication::Node node_1(Address::Make("1.2.3.4", 1234));
//     blazingdb::communication::Node node_2(Address::Make("5.6.7.8", 4564));
//     blazingdb::communication::Node node_3(Address::Make("10.11.20.21", 2021));

//     // Create Data - create pivots
//     using blazingdb::communication::messages::DataPivot;

//     std::vector<DataPivot> pivots;
//     pivots.emplace_back(DataPivot(node_1, "1111", "2222"));
//     pivots.emplace_back(DataPivot(node_2, "3333", "4444"));
//     pivots.emplace_back(DataPivot(node_3, "5555", "6666"));

//     // Make alias
//     using ContextToken = blazingdb::communication::ContextToken;
//     using PartitionPivotsMessage = blazingdb::communication::messages::PartitionPivotsMessage;

//     // Create context token
//     const ContextToken::TokenType context_token = 9678;

//     // Serialize data
//     std::string json_data;
//     std::string binary_data;

//     // Serialize message
//     {
//         PartitionPivotsMessage message(ContextToken::Make(context_token), pivots);

//         json_data = message.serializeToJson();
//         binary_data = message.serializeToBinary();
//     }

//     // Deserialize message & test
//     {
//         std::shared_ptr<PartitionPivotsMessage> message = std::dynamic_pointer_cast<PartitionPivotsMessage>
//                 (PartitionPivotsMessage::Make(json_data, binary_data));

//         // Test context token
//         ASSERT_EQ(context_token, message->getContextTokenValue());

//         // Test message token
//         ASSERT_EQ(PartitionPivotsMessage::getMessageID(), message->getMessageTokenValue());

//         // Test pivots
//         ASSERT_EQ(message->getDataPivots().size(), pivots.size());
//         for (std::size_t k = 0; k < pivots.size(); ++k) {
//             const auto& message_pivot = message->getDataPivots()[k];
//             ASSERT_EQ(message_pivot.getMinRange(), pivots[k].getMinRange());
//             ASSERT_EQ(message_pivot.getMaxRange(), pivots[k].getMaxRange());
//             ASSERT_TRUE(message_pivot.getNode() == pivots[k].getNode());
//         }
//     }
// }


TEST_F(ComponentMessagesTest, NodeDataMessage) {
    using Address = blazingdb::communication::Address;

    blazingdb::communication::Node node(Address::Make("1.2.3.4", 1234, 5678));
    // Create message
    using blazingdb::communication::messages::NodeDataMessage;
    NodeDataMessage message(node);

    // Serialize message
    const std::string serialize_message = message.serializeToJson();
    std::cout << serialize_message << std::endl;

    // Deserialize message
    std::shared_ptr<NodeDataMessage> deserialize_message = NodeDataMessage::Make(serialize_message, "");

    // Tests
    // ASSERT_EQ(message.node == deserialize_message->node, true);
}
