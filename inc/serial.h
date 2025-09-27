#ifndef SERIAL_H
#define SERIAL_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string.h>

#include "asio.hpp"

#include "ring_buffer.h"

namespace serial
{

const std::size_t kPortAsyncReadSize = 1024U;
const std::size_t kPortBufferSize    = 2048U;  // TODO templatize Port

// TODO handle port close
class Port : public std::enable_shared_from_this<Port>
{
    public:
        Port(const std::string& device, const std::uint32_t baud_rate);
        Port(Port &event_handler)                  = delete;
        Port(Port &&event_handler)                 = delete;
        Port &operator=(const Port &event_handler) = delete;
        Port &operator=(Port &&event_handler)      = delete;
        std::size_t Read(std::uint8_t *const buffer, const std::size_t size);
        std::size_t Write(std::uint8_t *const buffer, const std::size_t size);

    private:
        void AsyncRead(void);
        static size_t CheckReadComplete(const asio::error_code& error, std::size_t bytes_transferred);
        void HandleWrite(const std::error_code &error, const std::size_t bytes_transferred) const;
        void HandleRead(const std::error_code &error, const std::size_t bytes_transferred);

        asio::io_context io_context_ = asio::io_context();
        asio::serial_port serial_port_;
        std::array<std::uint8_t, kPortAsyncReadSize> async_read_buffer_;
        RingBuffer<std::uint8_t, kPortBufferSize> read_buffer_;
};

} // namespace serial

#endif // SERIAL_H