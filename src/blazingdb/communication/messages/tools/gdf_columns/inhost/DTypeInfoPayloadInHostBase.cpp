#include "DTypeInfoPayloadInHostBase.hpp"
#include "CategoryPayloadInHostBase.hpp"

#include "../buffers/StringBuffer.hpp"

#include "InHostGdfColumnIOHelpers.hpp"

namespace blazingdb {
namespace communication {
namespace messages {
namespace tools {
namespace gdf_columns {

DTypeInfoPayloadInHostBase::DTypeInfoPayloadInHostBase(const Buffer& buffer)
    : buffer_{buffer} {
  inhost_iohelpers::StreamBuffer streamBuffer(buffer);
  std::istream                   istream{&streamBuffer};
  istream.sync();

  std::istream::pos_type begin = std::istream::pos_type(
      reinterpret_cast<std::istream::streamoff>(buffer_.Data()));

  inhost_iohelpers::Read(istream, begin, &timeUnit_);

  std::unique_ptr<Buffer> categoryBuffer;
  inhost_iohelpers::Read(istream, begin, &categoryBuffer);

  auto specialized = CategorySpecialized::MakeInHost(*categoryBuffer);
  auto resultPayload = specialized->Apply();
  categoryPayload_ = static_cast<CategoryPayload *>(resultPayload.get());
}

std::int_fast32_t
DTypeInfoPayloadInHostBase::TimeUnit() const noexcept {
  return *timeUnit_;
}

const CategoryPayload&
DTypeInfoPayloadInHostBase::Category() const noexcept {
  return *categoryPayload_;
}

const Buffer&
DTypeInfoPayloadInHostBase::Deliver() const noexcept {
  return buffer_;
}

}  // namespace gdf_columns
}  // namespace tools
}  // namespace messages
}  // namespace communication
}  // namespace blazingdb