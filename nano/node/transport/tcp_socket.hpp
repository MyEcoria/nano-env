#pragma once

#include <nano/lib/asio.hpp>
#include <nano/lib/async.hpp>
#include <nano/lib/logging.hpp>
#include <nano/node/fwd.hpp>
#include <nano/node/transport/common.hpp>
#include <nano/node/transport/fwd.hpp>

#include <atomic>
#include <chrono>
#include <memory>

namespace nano::transport
{
class tcp_socket final : public std::enable_shared_from_this<tcp_socket>
{
public:
	/** Construct a new (unconnected) socket */
	explicit tcp_socket (nano::node &, nano::transport::socket_endpoint = socket_endpoint::client);

	/** Construct from an existing (connected) socket */
	tcp_socket (nano::node &, asio::ip::tcp::socket, nano::transport::socket_endpoint = socket_endpoint::server);

	~tcp_socket ();

	void close ();
	void close_async (); // Safe to call from io context

	nano::endpoint get_remote_endpoint () const;
	nano::endpoint get_local_endpoint () const;
	nano::transport::socket_endpoint get_endpoint_type () const;

	bool alive () const;

	bool has_connected () const;
	bool has_timed_out () const;

	std::chrono::steady_clock::time_point get_time_created () const;
	std::chrono::steady_clock::time_point get_time_connected () const;

public:
	asio::awaitable<std::tuple<boost::system::error_code>> co_connect (nano::endpoint endpoint);
	asio::awaitable<std::tuple<boost::system::error_code, size_t>> co_read (nano::shared_buffer, size_t size);
	asio::awaitable<std::tuple<boost::system::error_code, size_t>> co_write (nano::shared_buffer, size_t size);

	// Adapters for callback style code
	void async_connect (nano::endpoint endpoint, std::function<void (boost::system::error_code const &)> callback);
	void async_read (nano::shared_buffer, size_t size, std::function<void (boost::system::error_code const &, size_t)> callback = nullptr);
	void async_write (nano::shared_buffer, std::function<void (boost::system::error_code const &, size_t)> callback = nullptr);

	// Adapters for sync style code
	boost::system::error_code blocking_connect (nano::endpoint endpoint);
	std::tuple<boost::system::error_code, size_t> blocking_read (nano::shared_buffer, size_t size);
	std::tuple<boost::system::error_code, size_t> blocking_write (nano::shared_buffer, size_t size);

private:
	asio::awaitable<std::tuple<boost::system::error_code>> co_connect_impl (nano::endpoint endpoint);
	asio::awaitable<std::tuple<boost::system::error_code, size_t>> co_read_impl (nano::shared_buffer, size_t size);
	asio::awaitable<std::tuple<boost::system::error_code, size_t>> co_write_impl (nano::shared_buffer, size_t size);

public: // TODO: Remove these
	nano::transport::socket_type type () const
	{
		return type_m;
	};
	void type_set (nano::transport::socket_type type)
	{
		type_m = type;
	}

private:
	void start ();
	void stop ();

	void close_impl ();

	asio::awaitable<void> ongoing_checkup ();
	bool checkup ();

private:
	nano::node & node;

	nano::async::strand strand;
	nano::async::task task;
	asio::ip::tcp::socket raw_socket;

	nano::endpoint remote_endpoint;
	nano::endpoint local_endpoint;
	nano::transport::socket_endpoint const endpoint_type;

	std::atomic<bool> connected{ false };
	std::atomic<bool> closed{ false };
	std::atomic<bool> error{ false };
	std::atomic<bool> timed_out{ false };

	std::chrono::steady_clock::time_point const time_created{ std::chrono::steady_clock::now () };
	std::atomic<std::chrono::steady_clock::time_point> time_connected{};

	// Guard against conflicting concurrent async operations
	std::atomic<bool> connect_in_progress{ false };
	std::atomic<bool> read_in_progress{ false };
	std::atomic<bool> write_in_progress{ false };

	std::atomic<nano::transport::socket_type> type_m{ nano::transport::socket_type::undefined };

private: // Accessed only from strand
	// Using a low-resolution clock to track timeouts to avoid system clock overhead
	uint64_t timestamp{ 1 };
	uint64_t read_timestamp{ 0 };
	uint64_t write_timestamp{ 0 };
	uint64_t last_send{ 0 };
	uint64_t last_receive{ 0 };

public: // Logging
	void operator() (nano::object_stream &) const;
};
}
