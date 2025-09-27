#include "serial.h"

#include <iostream>
#include <memory>

#include "asio.hpp"

#include "logger.h"

namespace serial
{

static const std::string kPortLogTag("SERIAL PORT");

Port::Port(void) : serial_port_(io_context_)
{
}

Port::Port(const std::string& device) : device_(device), serial_port_(io_context_, device_)
{
}

void Port::Open(const uint32_t baud_rate)
{
    // TODO add ability to set options
    serial_port_.set_option(asio::serial_port_base::baud_rate(baud_rate));
    serial_port_.set_option(asio::serial_port_base::character_size(8));
    serial_port_.set_option(asio::serial_port_base::parity(asio::serial_port_base::parity::none));
    serial_port_.set_option(asio::serial_port_base::stop_bits(asio::serial_port_base::stop_bits::one));
    serial_port_.set_option(asio::serial_port_base::flow_control(asio::serial_port_base::flow_control::none));

    AsyncRead();

    LOGGER_LOG_DEBUG(std::cout, kPortLogTag, "New serial port on {} with baud rate {}", device_, baud_rate);
}

void Port::Open(const std::string& device, const uint32_t baud_rate)
{
    if (!serial_port_.is_open())
    {
        device_ = device;
        serial_port_.open(device_);
    }

    Open(baud_rate);
}

std::size_t Port::Read(std::uint8_t *const buffer, const std::size_t size)
{
    io_context_.run_for(std::chrono::milliseconds(1));  // TODO move to separate method or thread?
    return read_buffer_.Read(buffer, size);
}

std::size_t Port::Write(const std::uint8_t *const buffer, const std::size_t size)
{
    asio::async_write(serial_port_,
                      asio::buffer(buffer, size),
                      std::bind(&Port::HandleWrite,
                                shared_from_this(),
                                asio::placeholders::error,
                                asio::placeholders::bytes_transferred));

    return size;     // All data is always sent
}

size_t Port::CheckReadComplete(const asio::error_code& error, std::size_t bytes_transferred)
{
    size_t bytes_to_read = kPortAsyncReadSize;

    if (error || (bytes_transferred > 0))
    {
        bytes_to_read = 0;
    }

    return bytes_to_read;
}

void Port::AsyncRead(void)
{
    asio::async_read(serial_port_,
                     asio::buffer(async_read_buffer_, async_read_buffer_.max_size()),
                     CheckReadComplete,
                     std::bind(&Port::HandleRead,
                               shared_from_this(),
                               asio::placeholders::error,
                               asio::placeholders::bytes_transferred));
}

void Port::HandleWrite(const std::error_code &error, const std::size_t bytes_transferred) const
{
    if (0 != error.value())
    {
        LOGGER_LOG_ERROR(std::cerr, kPortLogTag, "Error writing data: {}", error.message());
    }
    else
    {
        LOGGER_LOG_VERBOSE(std::cout, kPortLogTag, "{} bytes written successfully", bytes_transferred);
        (void)bytes_transferred;
    }
}

void Port::HandleRead(const std::error_code &error, const std::size_t bytes_transferred)
{
    if (0 != error.value())
    {
        LOGGER_LOG_ERROR(std::cerr, kPortLogTag, "Error receiving data: {}", error.message());
    }
    else if ((read_buffer_.Capacity() - read_buffer_.Size()) < bytes_transferred)
    {
        LOGGER_LOG_WARNING(std::cout, kPortLogTag, "Not enough space to receive data with size {}", bytes_transferred);
        LOGGER_LOG_DEBUG(std::cout, kPortLogTag, "Buffer size: {}, buffer capacity: {}", read_buffer_.Size(), read_buffer_.Capacity());
    }
    else
    {
        for (size_t i = 0; i < bytes_transferred; i++)
        {
            read_buffer_.Write(&async_read_buffer_[i], 1); // Remaining buffer size previously checked, write should be successful
        }

        LOGGER_LOG_VERBOSE(std::cout, kPortLogTag, "{} bytes received successfully", bytes_transferred);
    }

    AsyncRead();
}

} // namespace serial