#pragma once

#include <nano/lib/blocks.hpp>
#include <nano/secure/account_info.hpp>
#include <nano/secure/pending_info.hpp>
#include <nano/secure/vote.hpp>
#include <nano/store/db_val.hpp>

inline nano::store::db_val::db_val (nano::account_info const & val_a) :
	span_view (reinterpret_cast<uint8_t const *> (&val_a), val_a.db_size ())
{
}

inline nano::store::db_val::db_val (nano::account_info_v22 const & val_a) :
	span_view (reinterpret_cast<uint8_t const *> (&val_a), val_a.db_size ())
{
}

inline nano::store::db_val::db_val (std::shared_ptr<nano::block> const & val_a) :
	buffer (std::make_shared<std::vector<uint8_t>> ())
{
	{
		nano::vectorstream stream (*buffer);
		nano::serialize_block (stream, *val_a);
	}
	convert_buffer_to_value ();
}

inline nano::store::db_val::db_val (nano::pending_info const & val_a) :
	span_view (reinterpret_cast<uint8_t const *> (&val_a), val_a.db_size ())
{
	static_assert (std::is_standard_layout<nano::pending_info>::value, "Standard layout is required");
}

inline nano::store::db_val::db_val (nano::pending_key const & val_a) :
	span_view (reinterpret_cast<uint8_t const *> (&val_a), sizeof (val_a))
{
	static_assert (std::is_standard_layout<nano::pending_key>::value, "Standard layout is required");
}

inline nano::store::db_val::operator nano::account_info () const
{
	nano::account_info result;
	debug_assert (span_view.size () == result.db_size ());
	std::copy (span_view.begin (), span_view.end (), reinterpret_cast<uint8_t *> (&result));
	return result;
}

inline nano::store::db_val::operator nano::account_info_v22 () const
{
	nano::account_info_v22 result;
	debug_assert (span_view.size () == result.db_size ());
	std::copy (span_view.begin (), span_view.end (), reinterpret_cast<uint8_t *> (&result));
	return result;
}

inline nano::store::db_val::operator std::shared_ptr<nano::block> () const
{
	nano::bufferstream stream (span_view.data (), span_view.size ());
	std::shared_ptr<nano::block> result (nano::deserialize_block (stream));
	return result;
}

inline nano::store::db_val::operator nano::store::block_w_sideband () const
{
	nano::bufferstream stream (span_view.data (), span_view.size ());
	nano::store::block_w_sideband block_w_sideband;
	block_w_sideband.block = (nano::deserialize_block (stream));
	auto error = block_w_sideband.sideband.deserialize (stream, block_w_sideband.block->type ());
	release_assert (!error);
	block_w_sideband.block->sideband_set (block_w_sideband.sideband);
	return block_w_sideband;
}

inline nano::store::db_val::operator nano::pending_info () const
{
	nano::pending_info result;
	debug_assert (span_view.size () == result.db_size ());
	std::copy (span_view.begin (), span_view.end (), reinterpret_cast<uint8_t *> (&result));
	return result;
}

inline nano::store::db_val::operator nano::pending_key () const
{
	nano::pending_key result;
	debug_assert (span_view.size () == sizeof (result));
	static_assert (sizeof (nano::pending_key::account) + sizeof (nano::pending_key::hash) == sizeof (result), "Packed class");
	std::copy (span_view.begin (), span_view.end (), reinterpret_cast<uint8_t *> (&result));
	return result;
}

inline nano::store::db_val::operator std::shared_ptr<nano::vote> () const
{
	nano::bufferstream stream (span_view.data (), span_view.size ());
	auto error (false);
	auto result (nano::make_shared<nano::vote> (error, stream));
	debug_assert (!error);
	return result;
}

inline nano::store::db_val::operator std::shared_ptr<nano::send_block> () const
{
	return convert_to_block<nano::send_block> ();
}

inline nano::store::db_val::operator std::shared_ptr<nano::receive_block> () const
{
	return convert_to_block<nano::receive_block> ();
}

inline nano::store::db_val::operator std::shared_ptr<nano::open_block> () const
{
	return convert_to_block<nano::open_block> ();
}

inline nano::store::db_val::operator std::shared_ptr<nano::change_block> () const
{
	return convert_to_block<nano::change_block> ();
}

inline nano::store::db_val::operator std::shared_ptr<nano::state_block> () const
{
	return convert_to_block<nano::state_block> ();
}

template <typename Block>
inline std::shared_ptr<Block> nano::store::db_val::convert_to_block () const
{
	nano::bufferstream stream (span_view.data (), span_view.size ());
	auto error (false);
	auto result (nano::make_shared<Block> (error, stream));
	debug_assert (!error);
	return result;
}
