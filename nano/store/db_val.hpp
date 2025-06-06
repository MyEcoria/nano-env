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

	db_val (nano::uint128_union const & val_a) :
		span_view (val_a.bytes.data (), sizeof (val_a))
	{
	}

	db_val (nano::uint256_union const & val_a) :
		span_view (val_a.bytes.data (), sizeof (val_a))
	{
	}

	db_val (nano::uint512_union const & val_a) :
		span_view (val_a.bytes.data (), sizeof (val_a))
	{
	}

	db_val (nano::qualified_root const & val_a) :
		span_view (reinterpret_cast<uint8_t const *> (&val_a), sizeof (val_a))
	{
	}

	db_val (nano::account_info const & val_a);

	db_val (nano::account_info_v22 const & val_a);

	db_val (nano::pending_info const & val_a);

	db_val (nano::pending_key const & val_a);

	db_val (nano::confirmation_height_info const & val_a) :
		buffer (std::make_shared<std::vector<uint8_t>> ())
	{
		{
			nano::vectorstream stream (*buffer);
			val_a.serialize (stream);
		}
		convert_buffer_to_value ();
	}

	db_val (nano::block_info const & val_a) :
		span_view (reinterpret_cast<uint8_t const *> (&val_a), sizeof (val_a))
	{
		static_assert (std::is_standard_layout<nano::block_info>::value, "Standard layout is required");
	}

	db_val (nano::endpoint_key const & val_a) :
		span_view (reinterpret_cast<uint8_t const *> (&val_a), sizeof (val_a))
	{
		static_assert (std::is_standard_layout<nano::endpoint_key>::value, "Standard layout is required");
	}

	db_val (std::shared_ptr<nano::block> const & val_a);

	db_val (uint64_t val_a) :
		buffer (std::make_shared<std::vector<uint8_t>> ())
	{
		{
			boost::endian::native_to_big_inplace (val_a);
			nano::vectorstream stream (*buffer);
			nano::write (stream, val_a);
		}
		convert_buffer_to_value ();
	}

	explicit operator nano::account_info () const;
	explicit operator nano::account_info_v22 () const;

	explicit operator block_info () const
	{
		nano::block_info result;
		debug_assert (size () == sizeof (result));
		static_assert (sizeof (nano::block_info::account) + sizeof (nano::block_info::balance) == sizeof (result), "Packed class");
		std::copy (span_view.begin (), span_view.end (), reinterpret_cast<uint8_t *> (&result));
		return result;
	}

	explicit operator nano::pending_info () const;

	explicit operator nano::pending_key () const;

	explicit operator nano::confirmation_height_info () const
	{
		nano::bufferstream stream (span_view.data (), span_view.size ());
		nano::confirmation_height_info result;
		bool error (result.deserialize (stream));
		(void)error;
		debug_assert (!error);
		return result;
	}

	explicit operator nano::uint128_union () const
	{
		return convert<nano::uint128_union> ();
	}

	explicit operator nano::amount () const
	{
		return convert<nano::amount> ();
	}

	explicit operator nano::block_hash () const
	{
		return convert<nano::block_hash> ();
	}

	explicit operator nano::public_key () const
	{
		return convert<nano::public_key> ();
	}

	explicit operator nano::qualified_root () const
	{
		return convert<nano::qualified_root> ();
	}

	explicit operator nano::uint256_union () const
	{
		return convert<nano::uint256_union> ();
	}

	explicit operator nano::uint512_union () const
	{
		return convert<nano::uint512_union> ();
	}

	explicit operator std::array<char, 64> () const
	{
		nano::bufferstream stream (span_view.data (), span_view.size ());
		std::array<char, 64> result;
		auto error = nano::try_read (stream, result);
		(void)error;
		debug_assert (!error);
		return result;
	}

	explicit operator nano::endpoint_key () const
	{
		nano::endpoint_key result;
		debug_assert (span_view.size () == sizeof (result));
		std::copy (span_view.begin (), span_view.end (), reinterpret_cast<uint8_t *> (&result));
		return result;
	}

	explicit operator block_w_sideband () const;

	explicit operator std::nullptr_t () const
	{
		return nullptr;
	}

	explicit operator nano::no_value () const
	{
		return no_value::dummy;
	}

	explicit operator std::shared_ptr<nano::block> () const;

	template <typename Block>
	std::shared_ptr<Block> convert_to_block () const;

	explicit operator std::shared_ptr<nano::send_block> () const;
	explicit operator std::shared_ptr<nano::receive_block> () const;
	explicit operator std::shared_ptr<nano::open_block> () const;
	explicit operator std::shared_ptr<nano::change_block> () const;
	explicit operator std::shared_ptr<nano::state_block> () const;

	explicit operator std::shared_ptr<nano::vote> () const;

	explicit operator uint64_t () const
	{
		uint64_t result;
		nano::bufferstream stream (span_view.data (), span_view.size ());
		auto error (nano::try_read (stream, result));
		(void)error;
		debug_assert (!error);
		boost::endian::big_to_native_inplace (result);
		return result;
	}

	void * data () const
	{
		return const_cast<void *> (static_cast<void const *> (span_view.data ()));
	}

	size_t size () const
	{
		return span_view.size ();
	}

	void convert_buffer_to_value ()
	{
		if (buffer)
		{
			span_view = std::span<uint8_t const> (buffer->data (), buffer->size ());
		}
	}

	std::span<uint8_t const> span_view;
	std::shared_ptr<std::vector<uint8_t>> buffer;

private:
	template <typename T>
	T convert () const
	{
		T result;
		debug_assert (span_view.size () == sizeof (result));
		std::copy (span_view.begin (), span_view.end (), result.bytes.data ());
		return result;
	}
};
}
