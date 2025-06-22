#pragma once

#include <nano/lib/numbers.hpp>
#include <nano/lib/stream.hpp>
#include <nano/secure/common.hpp>

#include <cstddef>
#include <span>

namespace nano
{
class account_info;
class account_info_v22;
class block;
class pending_info;
class pending_key;
class vote;
}

namespace nano::store
{
/**
 * Encapsulates database values using std::span for type safety and backend independence
 */
class db_val
{
public:
	db_val () = default;

	db_val (std::span<uint8_t const> span) :
		span_view (span)
	{
	}

	db_val (size_t size, void const * data) :
		span_view (static_cast<uint8_t const *> (data), size)
	{
	}

	db_val (std::nullptr_t) :
		span_view ()
	{
	}

	db_val (std::shared_ptr<std::vector<uint8_t>> buffer_a) :
		buffer (buffer_a)
	{
		convert_buffer_to_value ();
	}

	db_val (uint64_t);
	db_val (nano::uint128_union const &);
	db_val (nano::uint256_union const &);
	db_val (nano::uint512_union const &);
	db_val (nano::qualified_root const &);
	db_val (nano::account_info const &);
	db_val (nano::account_info_v22 const &);
	db_val (nano::pending_info const &);
	db_val (nano::pending_key const &);
	db_val (nano::confirmation_height_info const &);
	db_val (nano::block_info const &);
	db_val (nano::endpoint_key const &);
	db_val (std::shared_ptr<nano::block> const &);

	explicit operator uint64_t () const;
	explicit operator nano::uint128_union () const;
	explicit operator nano::uint256_union () const;
	explicit operator nano::uint512_union () const;
	explicit operator nano::qualified_root () const;
	explicit operator nano::account_info () const;
	explicit operator nano::account_info_v22 () const;
	explicit operator nano::pending_info () const;
	explicit operator nano::pending_key () const;
	explicit operator nano::confirmation_height_info () const;
	explicit operator nano::block_info () const;
	explicit operator nano::endpoint_key () const;
	explicit operator std::shared_ptr<nano::block> () const;
	explicit operator nano::amount () const;
	explicit operator nano::block_hash () const;
	explicit operator nano::public_key () const;
	explicit operator std::array<char, 64> () const;
	explicit operator block_w_sideband () const;
	explicit operator std::shared_ptr<nano::vote> () const;
	explicit operator std::nullptr_t () const;
	explicit operator nano::no_value () const;

	template <typename Block>
	std::shared_ptr<Block> convert_to_block () const;

	explicit operator std::shared_ptr<nano::send_block> () const;
	explicit operator std::shared_ptr<nano::receive_block> () const;
	explicit operator std::shared_ptr<nano::open_block> () const;
	explicit operator std::shared_ptr<nano::change_block> () const;
	explicit operator std::shared_ptr<nano::state_block> () const;

	void * data () const
	{
		return const_cast<void *> (static_cast<void const *> (span_view.data ()));
	}
	size_t size () const
	{
		return span_view.size ();
	}

	void convert_buffer_to_value ();

	std::span<uint8_t const> span_view;
	std::shared_ptr<std::vector<uint8_t>> buffer;

private:
	template <typename T>
	T convert () const;
};
}
