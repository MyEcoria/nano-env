#pragma once

#include <nano/lib/blocks.hpp>
#include <nano/lib/memory.hpp>
#include <nano/lib/stream.hpp>
#include <nano/secure/account_info.hpp>
#include <nano/secure/pending_info.hpp>
#include <nano/secure/vote.hpp>
#include <nano/store/db_val.hpp>

namespace nano::store
{
/*
 * Constructors
 */

inline db_val::db_val (uint64_t val_a) :
	buffer (std::make_shared<std::vector<uint8_t>> ())
{
	{
		boost::endian::native_to_big_inplace (val_a);
		nano::vectorstream stream (*buffer);
		nano::write (stream, val_a);
	}
	convert_buffer_to_value ();
}

inline db_val::db_val (nano::uint128_union const & val_a) :
	span_view (val_a.bytes.data (), sizeof (val_a))
{
}

inline db_val::db_val (nano::uint256_union const & val_a) :
	span_view (val_a.bytes.data (), sizeof (val_a))
{
}

inline db_val::db_val (nano::uint512_union const & val_a) :
	span_view (val_a.bytes.data (), sizeof (val_a))
{
}

inline db_val::db_val (nano::qualified_root const & val_a) :
	span_view (reinterpret_cast<uint8_t const *> (&val_a), sizeof (val_a))
{
}

inline db_val::db_val (nano::account_info const & val_a) :
	span_view (reinterpret_cast<uint8_t const *> (&val_a), val_a.db_size ())
{
}

inline db_val::db_val (nano::account_info_v22 const & val_a) :
	span_view (reinterpret_cast<uint8_t const *> (&val_a), val_a.db_size ())
{
}

inline db_val::db_val (nano::pending_info const & val_a) :
	span_view (reinterpret_cast<uint8_t const *> (&val_a), val_a.db_size ())
{
	static_assert (std::is_standard_layout<nano::pending_info>::value, "Standard layout is required");
}

inline db_val::db_val (nano::pending_key const & val_a) :
	span_view (reinterpret_cast<uint8_t const *> (&val_a), sizeof (val_a))
{
	static_assert (std::is_standard_layout<nano::pending_key>::value, "Standard layout is required");
}

inline db_val::db_val (nano::confirmation_height_info const & val_a) :
	buffer (std::make_shared<std::vector<uint8_t>> ())
{
	{
		nano::vectorstream stream (*buffer);
		val_a.serialize (stream);
	}
	convert_buffer_to_value ();
}

inline db_val::db_val (nano::block_info const & val_a) :
	span_view (reinterpret_cast<uint8_t const *> (&val_a), sizeof (val_a))
{
	static_assert (std::is_standard_layout<nano::block_info>::value, "Standard layout is required");
}

inline db_val::db_val (nano::endpoint_key const & val_a) :
	span_view (reinterpret_cast<uint8_t const *> (&val_a), sizeof (val_a))
{
	static_assert (std::is_standard_layout<nano::endpoint_key>::value, "Standard layout is required");
}

inline db_val::db_val (std::shared_ptr<nano::block> const & val_a) :
	buffer (std::make_shared<std::vector<uint8_t>> ())
{
	{
		nano::vectorstream stream (*buffer);
		nano::serialize_block (stream, *val_a);
	}
	convert_buffer_to_value ();
}

/*
 * Conversion operators
 */

inline db_val::operator uint64_t () const
{
	uint64_t result;
	nano::bufferstream stream (span_view.data (), span_view.size ());
	auto error (nano::try_read (stream, result));
	(void)error;
	debug_assert (!error);
	boost::endian::big_to_native_inplace (result);
	return result;
}

inline db_val::operator nano::uint128_union () const
{
	return convert<nano::uint128_union> ();
}

inline db_val::operator nano::uint256_union () const
{
	return convert<nano::uint256_union> ();
}

inline db_val::operator nano::uint512_union () const
{
	return convert<nano::uint512_union> ();
}

inline db_val::operator nano::qualified_root () const
{
	return convert<nano::qualified_root> ();
}

inline db_val::operator nano::account_info () const
{
	nano::account_info result;
	debug_assert (span_view.size () == result.db_size ());
	std::copy (span_view.begin (), span_view.end (), reinterpret_cast<uint8_t *> (&result));
	return result;
}

inline db_val::operator nano::account_info_v22 () const
{
	nano::account_info_v22 result;
	debug_assert (span_view.size () == result.db_size ());
	std::copy (span_view.begin (), span_view.end (), reinterpret_cast<uint8_t *> (&result));
	return result;
}

inline db_val::operator nano::pending_info () const
{
	nano::pending_info result;
	debug_assert (span_view.size () == result.db_size ());
	std::copy (span_view.begin (), span_view.end (), reinterpret_cast<uint8_t *> (&result));
	return result;
}

inline db_val::operator nano::pending_key () const
{
	nano::pending_key result;
	debug_assert (span_view.size () == sizeof (result));
	static_assert (sizeof (nano::pending_key::account) + sizeof (nano::pending_key::hash) == sizeof (result), "Packed class");
	std::copy (span_view.begin (), span_view.end (), reinterpret_cast<uint8_t *> (&result));
	return result;
}

inline db_val::operator nano::confirmation_height_info () const
{
	nano::bufferstream stream (span_view.data (), span_view.size ());
	nano::confirmation_height_info result;
	bool error (result.deserialize (stream));
	(void)error;
	debug_assert (!error);
	return result;
}

inline db_val::operator block_info () const
{
	nano::block_info result;
	debug_assert (size () == sizeof (result));
	static_assert (sizeof (nano::block_info::account) + sizeof (nano::block_info::balance) == sizeof (result), "Packed class");
	std::copy (span_view.begin (), span_view.end (), reinterpret_cast<uint8_t *> (&result));
	return result;
}

inline db_val::operator nano::endpoint_key () const
{
	nano::endpoint_key result;
	debug_assert (span_view.size () == sizeof (result));
	std::copy (span_view.begin (), span_view.end (), reinterpret_cast<uint8_t *> (&result));
	return result;
}

inline db_val::operator std::shared_ptr<nano::block> () const
{
	nano::bufferstream stream (span_view.data (), span_view.size ());
	std::shared_ptr<nano::block> result (nano::deserialize_block (stream));
	return result;
}

inline db_val::operator nano::amount () const
{
	return convert<nano::amount> ();
}

inline db_val::operator nano::block_hash () const
{
	return convert<nano::block_hash> ();
}

inline db_val::operator nano::public_key () const
{
	return convert<nano::public_key> ();
}

inline db_val::operator std::array<char, 64> () const
{
	nano::bufferstream stream (span_view.data (), span_view.size ());
	std::array<char, 64> result;
	auto error = nano::try_read (stream, result);
	(void)error;
	debug_assert (!error);
	return result;
}

inline db_val::operator nano::store::block_w_sideband () const
{
	nano::bufferstream stream (span_view.data (), span_view.size ());
	nano::store::block_w_sideband block_w_sideband;
	block_w_sideband.block = (nano::deserialize_block (stream));
	auto error = block_w_sideband.sideband.deserialize (stream, block_w_sideband.block->type ());
	release_assert (!error);
	block_w_sideband.block->sideband_set (block_w_sideband.sideband);
	return block_w_sideband;
}

inline db_val::operator std::shared_ptr<nano::vote> () const
{
	nano::bufferstream stream (span_view.data (), span_view.size ());
	auto error (false);
	auto result (nano::make_shared<nano::vote> (error, stream));
	debug_assert (!error);
	return result;
}

inline db_val::operator std::nullptr_t () const
{
	return nullptr;
}

inline db_val::operator nano::no_value () const
{
	return no_value::dummy;
}

template <typename Block>
inline std::shared_ptr<Block> db_val::convert_to_block () const
{
	nano::bufferstream stream (span_view.data (), span_view.size ());
	auto error (false);
	auto result (nano::make_shared<Block> (error, stream));
	debug_assert (!error);
	return result;
}

inline db_val::operator std::shared_ptr<nano::send_block> () const
{
	return convert_to_block<nano::send_block> ();
}

inline db_val::operator std::shared_ptr<nano::receive_block> () const
{
	return convert_to_block<nano::receive_block> ();
}

inline db_val::operator std::shared_ptr<nano::open_block> () const
{
	return convert_to_block<nano::open_block> ();
}

inline db_val::operator std::shared_ptr<nano::change_block> () const
{
	return convert_to_block<nano::change_block> ();
}

inline db_val::operator std::shared_ptr<nano::state_block> () const
{
	return convert_to_block<nano::state_block> ();
}

inline void db_val::convert_buffer_to_value ()
{
	if (buffer)
	{
		span_view = std::span<uint8_t const> (buffer->data (), buffer->size ());
	}
}

template <typename T>
inline T db_val::convert () const
{
	T result;
	debug_assert (span_view.size () == sizeof (result));
	std::copy (span_view.begin (), span_view.end (), result.bytes.data ());
	return result;
}
}