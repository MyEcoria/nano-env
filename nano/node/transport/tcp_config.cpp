#include <nano/node/transport/tcp_config.hpp>

nano::error nano::transport::tcp_config::serialize (nano::tomlconfig & toml) const
{
	toml.put ("max_inbound_connections", max_inbound_connections, "Maximum number of incoming TCP connections. \ntype:uint64");
	toml.put ("max_outbound_connections", max_outbound_connections, "Maximum number of outgoing TCP connections. \ntype:uint64");
	toml.put ("max_attempts", max_attempts, "Maximum connection attempts. \ntype:uint64");
	toml.put ("max_attempts_per_ip", max_attempts_per_ip, "Maximum connection attempts per IP. \ntype:uint64");

	toml.put ("connect_timeout", connect_timeout.count (), "Timeout for establishing TCP connection in seconds. \ntype:seconds");
	toml.put ("handshake_timeout", handshake_timeout.count (), "Timeout for completing node handshake in seconds. \ntype:seconds");
	toml.put ("io_timeout", io_timeout.count (), "Timeout for TCP I/O operations in seconds. Use 0 to disable timeout. \ntype:seconds");
	toml.put ("silent_timeout", silent_timeout.count (), "Timeout for silent TCP connections in seconds. Use 0 to disable timeout. \ntype:seconds");
	toml.put ("checkup_interval", checkup_interval.count (), "Interval for checking health of TCP connections in seconds. \ntype:seconds");

	return toml.get_error ();
}

nano::error nano::transport::tcp_config::deserialize (nano::tomlconfig & toml)
{
	toml.get ("max_inbound_connections", max_inbound_connections);
	toml.get ("max_outbound_connections", max_outbound_connections);
	toml.get ("max_attempts", max_attempts);
	toml.get ("max_attempts_per_ip", max_attempts_per_ip);

	toml.get_duration ("connect_timeout", connect_timeout);
	toml.get_duration ("handshake_timeout", handshake_timeout);
	toml.get_duration ("io_timeout", io_timeout);
	toml.get_duration ("silent_timeout", silent_timeout);
	toml.get_duration ("checkup_interval", checkup_interval);

	return toml.get_error ();
}