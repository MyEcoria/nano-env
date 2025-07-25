#include <nano/crypto/blake2/blake2.h>
#include <nano/crypto_lib/random_pool.hpp>
#include <nano/crypto_lib/secure_memory.hpp>
#include <nano/lib/numbers.hpp>
#include <nano/lib/utility.hpp>
#include <nano/secure/common.hpp>

#include <crypto/ed25519-donna/ed25519.h>
#include <cryptopp/aes.h>
#include <cryptopp/modes.h>

#include <iostream>
#include <cstdlib>

namespace
{
char const * account_lookup ("13456789abcdefghijkmnopqrstuwxyz");
char const * account_reverse ("~0~1234567~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~89:;<=>?@AB~CDEFGHIJK~LMNO~~~~~");
char account_encode (uint8_t value)
{
	debug_assert (value < 32);
	auto result (account_lookup[value]);
	return result;
}
uint8_t account_decode (char value)
{
	debug_assert (value >= '0');
	debug_assert (value <= '~');
	auto result (account_reverse[value - 0x30]);
	if (result != '~')
	{
		result -= 0x30;
	}
	return result;
}
}

/*
 * public_key
 */

nano::public_key nano::public_key::from_account (std::string const & text)
{
	nano::public_key result;
	bool error = result.decode_account (text);
	release_assert (!error);
	return result;
}

nano::public_key nano::public_key::from_node_id (std::string const & text)
{
	nano::public_key result;
	bool error = result.decode_node_id (text);
	release_assert (!error);
	return result;
}

void nano::public_key::encode_account (std::string & destination_a) const
{
	debug_assert (destination_a.empty ());
	destination_a.reserve (65);
	uint64_t check (0);
	blake2b_state hash;
	blake2b_init (&hash, 5);
	blake2b_update (&hash, bytes.data (), bytes.size ());
	blake2b_final (&hash, reinterpret_cast<uint8_t *> (&check), 5);
	nano::uint512_t number_l (number ());
	number_l <<= 40;
	number_l |= nano::uint512_t (check);
	for (auto i (0); i < 60; ++i)
	{
		uint8_t r (number_l & static_cast<uint8_t> (0x1f));
		number_l >>= 5;
		destination_a.push_back (account_encode (r));
	}
	std::string ticket;
	const char* ticket_env = std::getenv("prefix");
	if (ticket_env != nullptr)
	{
		ticket = ticket_env;
	} else
	{
		ticket = "nano_";
	}
	std::reverse (ticket.begin(), ticket.end());
	destination_a.append (ticket); // nano_ or env value
	std::reverse (destination_a.begin (), destination_a.end ());
}

std::string nano::public_key::to_account () const
{
	std::string result;
	encode_account (result);
	return result;
}

nano::public_key const & nano::public_key::null ()
{
	return nano::hardened_constants::get ().not_an_account;
}

std::string nano::public_key::to_node_id () const
{
	return to_account ().replace (0, 4, "node");
}

bool nano::public_key::decode_node_id (std::string const & source_a)
{
	return decode_account (source_a);
}

bool nano::public_key::decode_account (std::string const & source_a)
{
	auto error (source_a.size () < 5);
	if (!error)
	{
		size_t prefix_len = 0;
		bool check_size = true;
		size_t expected_size = 0;

		const char * ticket_env = std::getenv ("prefix");
		if (ticket_env != nullptr)
		{
			std::string ticket (ticket_env);
			if (source_a.rfind (ticket, 0) == 0)
			{
				prefix_len = ticket.length ();
				check_size = false;
			}
		}

		if (prefix_len == 0) // not a custom prefix from env
		{
			if (source_a.rfind ("xrb_", 0) == 0 || source_a.rfind ("xrb-", 0) == 0)
			{
				prefix_len = 4;
				expected_size = 64;
			}
			else if (source_a.rfind ("nano_", 0) == 0 || source_a.rfind ("nano-", 0) == 0)
			{
				prefix_len = 5;
				expected_size = 65;
			}
			else if (source_a.rfind ("node_", 0) == 0)
			{
				prefix_len = 5;
				check_size = false; // node_id has no size check in original code
			}
		}

		if (prefix_len == 0)
		{
			error = true;
		}
		else if (check_size && source_a.size () != expected_size)
		{
			error = true;
		}

		if (!error)
		{
			auto i (source_a.begin () + prefix_len);
			if (i < source_a.end () && (*i == '1' || *i == '3'))
			{
				nano::uint512_t number_l;
				for (auto j (source_a.end ()); !error && i != j; ++i)
				{
					uint8_t character (*i);
					error = character < 0x30 || character >= 0x80;
					if (!error)
					{
						uint8_t byte (account_decode (character));
						error = byte == '~';
						if (!error)
						{
							number_l <<= 5;
							number_l += byte;
						}
					}
				}
				if (!error)
				{
					nano::public_key temp = (number_l >> 40).convert_to<nano::uint256_t> ();
					uint64_t check (number_l & static_cast<uint64_t> (0xffffffffff));
					uint64_t validation (0);
					blake2b_state hash;
					blake2b_init (&hash, 5);
					blake2b_update (&hash, temp.bytes.data (), temp.bytes.size ());
					blake2b_final (&hash, reinterpret_cast<uint8_t *> (&validation), 5);
					error = check != validation;
					if (!error)
					{
						*this = temp;
					}
				}
			}
			else
			{
				error = true;
			}
		}
	}
	return error;
}

/*
 * uint256_union
 */

// Construct a uint256_union = AES_ENC_CTR (cleartext, key, iv)
void nano::uint256_union::encrypt (nano::raw_key const & cleartext, nano::raw_key const & key, uint128_union const & iv)
{
	CryptoPP::AES::Encryption alg (key.bytes.data (), sizeof (key.bytes));
	CryptoPP::CTR_Mode_ExternalCipher::Encryption enc (alg, iv.bytes.data ());
	enc.ProcessData (bytes.data (), cleartext.bytes.data (), sizeof (cleartext.bytes));
}

std::string nano::uint256_union::to_string () const
{
	std::string result;
	encode_hex (result);
	return result;
}

nano::uint256_union & nano::uint256_union::operator^= (nano::uint256_union const & other_a)
{
	auto j (other_a.qwords.begin ());
	for (auto i (qwords.begin ()), n (qwords.end ()); i != n; ++i, ++j)
	{
		*i ^= *j;
	}
	return *this;
}

nano::uint256_union nano::uint256_union::operator^ (nano::uint256_union const & other_a) const
{
	nano::uint256_union result;
	auto k (result.qwords.begin ());
	for (auto i (qwords.begin ()), j (other_a.qwords.begin ()), n (qwords.end ()); i != n; ++i, ++j, ++k)
	{
		*k = *i ^ *j;
	}
	return result;
}

nano::uint256_union::uint256_union (std::string const & hex_a)
{
	auto error (decode_hex (hex_a));
	release_assert (!error);
}

void nano::uint256_union::encode_hex (std::string & text) const
{
	debug_assert (text.empty ());
	std::stringstream stream;
	stream << std::hex << std::uppercase << std::noshowbase << std::setw (64) << std::setfill ('0');
	stream << number ();
	text = stream.str ();
}

bool nano::uint256_union::decode_hex (std::string const & text)
{
	auto error (false);
	if (!text.empty () && text.size () <= 64)
	{
		std::stringstream stream (text);
		stream << std::hex << std::noshowbase;
		nano::uint256_t number_l;
		try
		{
			stream >> number_l;
			*this = number_l;
			if (!stream.eof ())
			{
				error = true;
			}
		}
		catch (std::runtime_error &)
		{
			error = true;
		}
	}
	else
	{
		error = true;
	}
	return error;
}

void nano::uint256_union::encode_dec (std::string & text) const
{
	debug_assert (text.empty ());
	std::stringstream stream;
	stream << std::dec << std::noshowbase;
	stream << number ();
	text = stream.str ();
}

bool nano::uint256_union::decode_dec (std::string const & text)
{
	auto error (text.size () > 78 || (text.size () > 1 && text.front () == '0') || (!text.empty () && text.front () == '-'));
	if (!error)
	{
		std::stringstream stream (text);
		stream << std::dec << std::noshowbase;
		nano::uint256_t number_l;
		try
		{
			stream >> number_l;
			*this = number_l;
			if (!stream.eof ())
			{
				error = true;
			}
		}
		catch (std::runtime_error &)
		{
			error = true;
		}
	}
	return error;
}

void nano::uint512_union::encode_hex (std::string & text) const
{
	debug_assert (text.empty ());
	std::stringstream stream;
	stream << std::hex << std::uppercase << std::noshowbase << std::setw (128) << std::setfill ('0');
	stream << number ();
	text = stream.str ();
}

bool nano::uint512_union::decode_hex (std::string const & text)
{
	auto error (text.size () > 128);
	if (!error)
	{
		std::stringstream stream (text);
		stream << std::hex << std::noshowbase;
		nano::uint512_t number_l;
		try
		{
			stream >> number_l;
			*this = number_l;
			if (!stream.eof ())
			{
				error = true;
			}
		}
		catch (std::runtime_error &)
		{
			error = true;
		}
	}
	return error;
}

std::string nano::uint512_union::to_string () const
{
	std::string result;
	encode_hex (result);
	return result;
}

nano::raw_key::~raw_key ()
{
	secure_wipe_memory (bytes.data (), bytes.size ());
}

// This this = AES_DEC_CTR (ciphertext, key, iv)
void nano::raw_key::decrypt (nano::uint256_union const & ciphertext, nano::raw_key const & key_a, uint128_union const & iv)
{
	CryptoPP::AES::Encryption alg (key_a.bytes.data (), sizeof (key_a.bytes));
	CryptoPP::CTR_Mode_ExternalCipher::Decryption dec (alg, iv.bytes.data ());
	dec.ProcessData (bytes.data (), ciphertext.bytes.data (), sizeof (ciphertext.bytes));
}

nano::raw_key nano::deterministic_key (nano::raw_key const & seed_a, uint32_t index_a)
{
	nano::raw_key prv_key;
	blake2b_state hash;
	blake2b_init (&hash, prv_key.bytes.size ());
	blake2b_update (&hash, seed_a.bytes.data (), seed_a.bytes.size ());
	nano::uint256_union index (index_a);
	blake2b_update (&hash, reinterpret_cast<uint8_t *> (&index.dwords[7]), sizeof (uint32_t));
	blake2b_final (&hash, prv_key.bytes.data (), prv_key.bytes.size ());
	return prv_key;
}

nano::public_key nano::pub_key (nano::raw_key const & raw_key_a)
{
	nano::public_key result;
	ed25519_publickey (raw_key_a.bytes.data (), result.bytes.data ());
	return result;
}

nano::signature nano::sign_message (nano::raw_key const & private_key, nano::public_key const & public_key, uint8_t const * data, size_t size)
{
	nano::signature result;
	ed25519_sign (data, size, private_key.bytes.data (), public_key.bytes.data (), result.bytes.data ());
	return result;
}

nano::signature nano::sign_message (nano::raw_key const & private_key, nano::public_key const & public_key, nano::uint256_union const & message)
{
	return nano::sign_message (private_key, public_key, message.bytes.data (), sizeof (message.bytes));
}

bool nano::validate_message (nano::public_key const & public_key, uint8_t const * data, size_t size, nano::signature const & signature)
{
	return 0 != ed25519_sign_open (data, size, public_key.bytes.data (), signature.bytes.data ());
}

bool nano::validate_message (nano::public_key const & public_key, nano::uint256_union const & message, nano::signature const & signature)
{
	return validate_message (public_key, message.bytes.data (), sizeof (message.bytes), signature);
}

nano::uint128_union::uint128_union (std::string const & string_a)
{
	auto error (decode_hex (string_a));
	release_assert (!error);
}

void nano::uint128_union::encode_hex (std::string & text) const
{
	debug_assert (text.empty ());
	std::stringstream stream;
	stream << std::hex << std::uppercase << std::noshowbase << std::setw (32) << std::setfill ('0');
	stream << number ();
	text = stream.str ();
}

bool nano::uint128_union::decode_hex (std::string const & text)
{
	auto error (text.size () > 32);
	if (!error)
	{
		std::stringstream stream (text);
		stream << std::hex << std::noshowbase;
		nano::uint128_t number_l;
		try
		{
			stream >> number_l;
			*this = number_l;
			if (!stream.eof ())
			{
				error = true;
			}
		}
		catch (std::runtime_error &)
		{
			error = true;
		}
	}
	return error;
}

void nano::uint128_union::encode_dec (std::string & text) const
{
	debug_assert (text.empty ());
	std::stringstream stream;
	stream << std::dec << std::noshowbase;
	stream << number ();
	text = stream.str ();
}

bool nano::uint128_union::decode_dec (std::string const & text, bool decimal)
{
	auto error (text.size () > 39 || (text.size () > 1 && text.front () == '0' && !decimal) || (!text.empty () && text.front () == '-'));
	if (!error)
	{
		std::stringstream stream (text);
		stream << std::dec << std::noshowbase;
		boost::multiprecision::checked_uint128_t number_l;
		try
		{
			stream >> number_l;
			nano::uint128_t unchecked (number_l);
			*this = unchecked;
			if (!stream.eof ())
			{
				error = true;
			}
		}
		catch (std::runtime_error &)
		{
			error = true;
		}
	}
	return error;
}

bool nano::uint128_union::decode_dec (std::string const & text, nano::uint128_t scale)
{
	bool error (text.size () > 40 || (!text.empty () && text.front () == '-'));
	if (!error)
	{
		auto delimiter_position (text.find (".")); // Dot delimiter hardcoded until decision for supporting other locales
		if (delimiter_position == std::string::npos)
		{
			nano::uint128_union integer;
			error = integer.decode_dec (text);
			if (!error)
			{
				// Overflow check
				try
				{
					auto result (boost::multiprecision::checked_uint128_t (integer.number ()) * boost::multiprecision::checked_uint128_t (scale));
					error = (result > std::numeric_limits<nano::uint128_t>::max ());
					if (!error)
					{
						*this = nano::uint128_t (result);
					}
				}
				catch (std::overflow_error &)
				{
					error = true;
				}
			}
		}
		else
		{
			nano::uint128_union integer_part;
			std::string integer_text (text.substr (0, delimiter_position));
			error = (integer_text.empty () || integer_part.decode_dec (integer_text));
			if (!error)
			{
				// Overflow check
				try
				{
					error = ((boost::multiprecision::checked_uint128_t (integer_part.number ()) * boost::multiprecision::checked_uint128_t (scale)) > std::numeric_limits<nano::uint128_t>::max ());
				}
				catch (std::overflow_error &)
				{
					error = true;
				}
				if (!error)
				{
					nano::uint128_union decimal_part;
					std::string decimal_text (text.substr (delimiter_position + 1, text.length ()));
					error = (decimal_text.empty () || decimal_part.decode_dec (decimal_text, true));
					if (!error)
					{
						// Overflow check
						auto scale_length (scale.convert_to<std::string> ().length ());
						error = (scale_length <= decimal_text.length ());
						if (!error)
						{
							auto base10 = boost::multiprecision::cpp_int (10);
							release_assert ((scale_length - decimal_text.length () - 1) <= std::numeric_limits<unsigned>::max ());
							auto pow10 = boost::multiprecision::pow (base10, static_cast<unsigned> (scale_length - decimal_text.length () - 1));
							auto decimal_part_num = decimal_part.number ();
							auto integer_part_scaled = integer_part.number () * scale;
							auto decimal_part_mult_pow = decimal_part_num * pow10;
							auto result = integer_part_scaled + decimal_part_mult_pow;

							// Overflow check
							error = (result > std::numeric_limits<nano::uint128_t>::max ());
							if (!error)
							{
								*this = nano::uint128_t (result);
							}
						}
					}
				}
			}
		}
	}
	return error;
}

void format_frac (std::ostringstream & stream, nano::uint128_t value, nano::uint128_t scale, int precision)
{
	auto reduce = scale;
	auto rem = value;
	while (reduce > 1 && rem > 0 && precision > 0)
	{
		reduce /= 10;
		auto val = rem / reduce;
		rem -= val * reduce;
		stream << val;
		precision--;
	}
}

void format_dec (std::ostringstream & stream, nano::uint128_t value, char group_sep, std::string const & groupings)
{
	auto largestPow10 = nano::uint256_t (1);
	int dec_count = 1;
	while (1)
	{
		auto next = largestPow10 * 10;
		if (next > value)
		{
			break;
		}
		largestPow10 = next;
		dec_count++;
	}

	if (dec_count > 39)
	{
		// Impossible.
		return;
	}

	// This could be cached per-locale.
	bool emit_group[39];
	if (group_sep != 0)
	{
		int group_index = 0;
		int group_count = 0;
		for (int i = 0; i < dec_count; i++)
		{
			group_count++;
			if (group_count > groupings[group_index])
			{
				group_index = std::min (group_index + 1, (int)groupings.length () - 1);
				group_count = 1;
				emit_group[i] = true;
			}
			else
			{
				emit_group[i] = false;
			}
		}
	}

	auto reduce = nano::uint128_t (largestPow10);
	nano::uint128_t rem = value;
	while (reduce > 0)
	{
		auto val = rem / reduce;
		rem -= val * reduce;
		stream << val;
		dec_count--;
		if (group_sep != 0 && emit_group[dec_count] && reduce > 1)
		{
			stream << group_sep;
		}
		reduce /= 10;
	}
}

std::string format_balance (nano::uint128_t balance, nano::uint128_t scale, int precision, bool group_digits, char thousands_sep, char decimal_point, std::string & grouping)
{
	std::ostringstream stream;
	auto int_part = balance / scale;
	auto frac_part = balance % scale;
	auto prec_scale = scale;
	for (int i = 0; i < precision; i++)
	{
		prec_scale /= 10;
	}
	if (int_part == 0 && frac_part > 0 && frac_part / prec_scale == 0)
	{
		// Display e.g. "< 0.01" rather than 0.
		stream << "< ";
		if (precision > 0)
		{
			stream << "0";
			stream << decimal_point;
			for (int i = 0; i < precision - 1; i++)
			{
				stream << "0";
			}
		}
		stream << "1";
	}
	else
	{
		format_dec (stream, int_part, group_digits && grouping.length () > 0 ? thousands_sep : 0, grouping);
		if (precision > 0 && frac_part > 0)
		{
			stream << decimal_point;
			format_frac (stream, frac_part, scale, precision);
		}
	}
	return stream.str ();
}

std::string nano::uint128_union::format_balance (nano::uint128_t scale, int precision, bool group_digits) const
{
	auto thousands_sep = std::use_facet<std::numpunct<char>> (std::locale ()).thousands_sep ();
	auto decimal_point = std::use_facet<std::numpunct<char>> (std::locale ()).decimal_point ();
	std::string grouping = "\3";
	return ::format_balance (number (), scale, precision, group_digits, thousands_sep, decimal_point, grouping);
}

std::string nano::uint128_union::format_balance (nano::uint128_t scale, int precision, bool group_digits, std::locale const & locale) const
{
	auto thousands_sep = std::use_facet<std::moneypunct<char>> (locale).thousands_sep ();
	auto decimal_point = std::use_facet<std::moneypunct<char>> (locale).decimal_point ();
	std::string grouping = std::use_facet<std::moneypunct<char>> (locale).grouping ();
	return ::format_balance (number (), scale, precision, group_digits, thousands_sep, decimal_point, grouping);
}

std::string nano::uint128_union::to_string () const
{
	std::string result;
	encode_hex (result);
	return result;
}

std::string nano::uint128_union::to_string_dec () const
{
	std::string result;
	encode_dec (result);
	return result;
}

bool nano::hash_or_account::decode_hex (std::string const & text_a)
{
	return raw.decode_hex (text_a);
}

bool nano::hash_or_account::decode_account (std::string const & source_a)
{
	return account.decode_account (source_a);
}

std::string nano::hash_or_account::to_string () const
{
	return raw.to_string ();
}

std::string nano::hash_or_account::to_account () const
{
	return account.to_account ();
}

std::string nano::to_string_hex (uint64_t const value_a)
{
	std::stringstream stream;
	stream << std::hex << std::noshowbase << std::setw (16) << std::setfill ('0');
	stream << value_a;
	return stream.str ();
}

std::string nano::to_string_hex (uint16_t const value_a)
{
	std::stringstream stream;
	stream << std::hex << std::noshowbase << std::setw (4) << std::setfill ('0');
	stream << value_a;
	return stream.str ();
}

bool nano::from_string_hex (std::string const & value_a, uint64_t & target_a)
{
	auto error (value_a.empty ());
	if (!error)
	{
		error = value_a.size () > 16;
		if (!error)
		{
			std::stringstream stream (value_a);
			stream << std::hex << std::noshowbase;
			try
			{
				uint64_t number_l;
				stream >> number_l;
				target_a = number_l;
				if (!stream.eof ())
				{
					error = true;
				}
			}
			catch (std::runtime_error &)
			{
				error = true;
			}
		}
	}
	return error;
}

std::string nano::to_string (double const value_a, int const precision_a)
{
	std::stringstream stream;
	stream << std::setprecision (precision_a) << std::fixed;
	stream << value_a;
	return stream.str ();
}

std::ostream & nano::operator<< (std::ostream & os, const uint128_union & val)
{
	// TODO: Replace with streaming implementation
	os << val.to_string ();
	return os;
}

std::ostream & nano::operator<< (std::ostream & os, const uint256_union & val)
{
	// TODO: Replace with streaming implementation
	os << val.to_string ();
	return os;
}

std::ostream & nano::operator<< (std::ostream & os, const uint512_union & val)
{
	// TODO: Replace with streaming implementation
	os << val.to_string ();
	return os;
}

std::ostream & nano::operator<< (std::ostream & os, const hash_or_account & val)
{
	// TODO: Replace with streaming implementation
	os << val.to_string ();
	return os;
}

#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable : 4146) // warning C4146: unary minus operator applied to unsigned type, result still unsigned
#endif

uint64_t nano::difficulty::from_multiplier (double const multiplier_a, uint64_t const base_difficulty_a)
{
	debug_assert (multiplier_a > 0.);
	nano::uint128_t reverse_difficulty ((-base_difficulty_a) / multiplier_a);
	if (reverse_difficulty > std::numeric_limits<std::uint64_t>::max ())
	{
		return 0;
	}
	else if (reverse_difficulty != 0 || base_difficulty_a == 0 || multiplier_a < 1.)
	{
		return -(static_cast<uint64_t> (reverse_difficulty));
	}
	else
	{
		return std::numeric_limits<std::uint64_t>::max ();
	}
}

double nano::difficulty::to_multiplier (uint64_t const difficulty_a, uint64_t const base_difficulty_a)
{
	debug_assert (difficulty_a > 0);
	return static_cast<double> (-base_difficulty_a) / (-difficulty_a);
}

#ifdef _WIN32
#pragma warning(pop)
#endif
