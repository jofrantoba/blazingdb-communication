#ifndef BLAZINGDB_COMMUNICATION_MESSAGES_COMPONENTMESSAGE_H
#define BLAZINGDB_COMMUNICATION_MESSAGES_COMPONENTMESSAGE_H

#include "blazingdb/communication/messages/BaseComponentMessage.h"
#include <iostream>
#include <chrono>

namespace blazingdb {
namespace communication {
namespace messages {

    template <typename RalColumn, typename CudfColumn, typename GpuFunctions>
    class GpuComponentMessage : public BaseComponentMessage {
    protected:
        GpuComponentMessage(std::unique_ptr<MessageToken>&& message_token,
                            std::shared_ptr<ContextToken>&& context_token,
                            const Node& sender_node)
        : BaseComponentMessage(std::move(message_token), std::move(context_token), sender_node)
        { }

    protected:
        GpuComponentMessage(const ContextToken& context_token, const MessageToken::TokenType& message_token)
        : BaseComponentMessage(context_token, message_token)
        { }

        GpuComponentMessage(std::shared_ptr<ContextToken>&& context_token, const MessageToken::TokenType& message_token)
        : BaseComponentMessage(std::move(context_token), message_token)
        { }

    protected:
        static void serializeCudfColumn(BaseComponentMessage::Writer& writer, CudfColumn* column) {
            writer.StartObject();
            {
                writer.Key("size");
                writer.Uint64(column->size);

                writer.Key("dtype");
                writer.Uint(column->dtype);

                writer.Key("null_count");
                writer.Uint64(column->null_count);

                writer.Key("dtype_info-time_unit");
                writer.Uint(column->dtype_info.time_unit);
            }
            writer.EndObject();
        }

        static void serializeRalColumn(BaseComponentMessage::Writer& writer, RalColumn& column) {
            writer.StartObject();
            {
                writer.Key("is_ipc");
                writer.Bool(column.is_ipc());

                writer.Key("column_token");
                writer.Uint64(column.get_column_token());

                writer.Key("column_name");
                auto column_name = column.name();
                writer.String(column_name.c_str(), column_name.length());

                writer.Key("has_valids");
                writer.Bool(column.null_count() > 0 || (column.null_count() == 0 && column.valid() != nullptr));

                writer.Key("cudf_column");
                serializeCudfColumn(writer, column.get_gdf_column());
            }
            writer.EndObject();
        }

        static std::string serializeToBinary(std::vector<RalColumn>& columns) {
            std::string result;

            std::size_t capacity = 0;
            for (const auto& column : columns) {
              if (!GpuFunctions::isGdfString(*column.get_gdf_column())) {
                capacity +=
                    GpuFunctions::getDataCapacity(column.get_gdf_column());
                capacity +=
                    GpuFunctions::getValidCapacity(column.get_gdf_column());
              }
              // WARNING!!! Here we are only getting the size for non-string columns. The size we need for string columns is determined inside the copyGpuToCpu where it is resized again. 
              // THIS is a bad performance issue. This needs to be addressed
              // TODO!!
            }

            const typename GpuFunctions::StringsInfo* stringsInfo =
                GpuFunctions::createStringsInfo(columns);

            capacity += GpuFunctions::getStringsCapacity(stringsInfo);
            result.resize(capacity);

            std::size_t binary_pointer = 0;
            for (const auto& column : columns) {
              GpuFunctions::copyGpuToCpu(binary_pointer, result,
                                         const_cast<RalColumn&>(column),
                                         stringsInfo);
            }

            GpuFunctions::destroyStringsInfo(stringsInfo);

            return result;
        }

        static CudfColumn deserializeCudfColumn(rapidjson::Value::ConstObject&& object) {
            CudfColumn column;

            column.size = object["size"].GetUint64();

            column.dtype = (typename GpuFunctions::DType)object["dtype"].GetUint();

            column.null_count = object["null_count"].GetUint64();

            column.dtype_info = (typename GpuFunctions::DTypeInfo){
                (typename GpuFunctions::TimeUnit)object["dtype_info-time_unit"]
                    .GetUint()};

            return column;
        }

        static RalColumn deserializeRalColumn(std::size_t& binary_pointer, const std::string& binary_data, rapidjson::Value::ConstObject&& object) {
	        auto	start = std::chrono::high_resolution_clock::now();

            const auto& column_name_data = object["column_name"];
            std::string column_name(column_name_data.GetString(), column_name_data.GetStringLength());

            bool is_ipc = object["is_ipc"].GetBool();

            std::uint64_t column_token = object["column_token"].GetUint64();

            bool has_valids = object["has_valids"].GetBool();

            auto cudf_column = deserializeCudfColumn(object["cudf_column"].GetObject());

            RalColumn ral_column;

            std::size_t dtype_size = GpuFunctions::getDTypeSize(cudf_column.dtype);

            if (GpuFunctions::isGdfString(cudf_column)) {
              if (!cudf_column.size) {
                typename GpuFunctions::NvCategory* nvCategory =
                    GpuFunctions::NvCategory::create_from_array(nullptr, 0);
                ral_column.create_gdf_column(nvCategory, 0, column_name);
                return ral_column;
              }

              const std::size_t stringsSize =
                  *reinterpret_cast<const std::size_t*>(
                      &binary_data[binary_pointer]);
              const std::size_t offsetsSize =
                  *reinterpret_cast<const std::size_t*>(
                      &binary_data[binary_pointer + sizeof(const std::size_t)]);

              const std::size_t stringsIndex =
                  binary_pointer + 3 * sizeof(const std::size_t);
              const std::size_t offsetsIndex = stringsIndex + stringsSize;

              const void* stringsPointer =
                  reinterpret_cast<const typename GpuFunctions::NvStrings*>(
                      &binary_data[stringsIndex]);
              const void* offsetsPointer =
                  reinterpret_cast<const typename GpuFunctions::NvStrings*>(
                      &binary_data[offsetsIndex]);

              const std::size_t keysLength =
                  *reinterpret_cast<const std::size_t*>(
                      &binary_data[binary_pointer +
                                   2 * sizeof(const std::size_t)]);

              typename GpuFunctions::NvStrings* nvStrings =
                  GpuFunctions::CreateNvStrings(stringsPointer, offsetsPointer,
                                                keysLength);

              typename GpuFunctions::NvCategory* nvCategory =
                  GpuFunctions::NvCategory::create_from_strings(*nvStrings);

              binary_pointer +=
                  stringsSize + offsetsSize + 3 * sizeof(const std::size_t);

              ral_column.create_gdf_column(nvCategory, keysLength, column_name);
            } else {  // gdf is not string
              // Calculate pointers and update binary_pointer
              std::size_t data_pointer = binary_pointer;
              std::size_t valid_pointer =
                  data_pointer + GpuFunctions::getDataCapacity(&cudf_column);
              binary_pointer =
                  valid_pointer + GpuFunctions::getValidCapacity(&cudf_column);

              if (!is_ipc) {
                if(cudf_column.null_count > 0){
                    ral_column.create_gdf_column(cudf_column.dtype,
                                                 cudf_column.size,
                                                 (typename GpuFunctions::DataTypePointer)&binary_data[data_pointer],
                                                 (typename GpuFunctions::ValidTypePointer)&binary_data[valid_pointer],
                                                 dtype_size,
                                                 column_name);
            	} else if(has_valids) {
                    ral_column.create_gdf_column(cudf_column.dtype,
                                                 cudf_column.size,
                                                 (typename GpuFunctions::DataTypePointer)&binary_data[data_pointer],
                                                 dtype_size,
                                                 column_name);
                } else {
                    ral_column.create_gdf_column(cudf_column.dtype,
                                                 cudf_column.size,
                                                 (typename GpuFunctions::DataTypePointer)&binary_data[data_pointer],
                                                 (typename GpuFunctions::ValidTypePointer)nullptr,
                                                 dtype_size,
                                                 column_name);
            	}
              } else {
                ral_column.create_gdf_column_for_ipc(
                    cudf_column.dtype,
                    (typename GpuFunctions::DataTypePointer) &
                        binary_data[data_pointer],
                    (typename GpuFunctions::ValidTypePointer) &
                        binary_data[valid_pointer],
                    cudf_column.size, cudf_column.null_count, column_name);
              }

              ral_column.set_column_token(column_token);
              ral_column.get_gdf_column()->null_count = cudf_column.null_count;
              ral_column.get_gdf_column()->dtype_info = cudf_column.dtype_info;
            }
            std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration<double, std::milli>(end-start).count();

            GpuFunctions::log("-> deserializeRalColumn " + std::to_string(duration) + " ms" );
            return ral_column;
        }
    };

} // namespace messages
} // namespace communication
} // namespace blazingdb

#endif //BLAZINGDB_COMMUNICATION_MESSAGES_COMPONENTMESSAGE_H
