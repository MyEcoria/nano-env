#pragma once

#include <nano/lib/common.hpp>
#include <nano/lib/numbers.hpp>

#include <fmt/ostream.h>

template <>
struct fmt::formatter<nano::endpoint> : fmt::ostream_formatter
{
};
template <>
struct fmt::formatter<nano::ip_address> : fmt::ostream_formatter
{
};

template <>
struct fmt::formatter<nano::uint128_t> : fmt::ostream_formatter
{
};
template <>
struct fmt::formatter<nano::uint256_t> : fmt::ostream_formatter
{
};
template <>
struct fmt::formatter<nano::uint512_t> : fmt::ostream_formatter
{
};

template <>
struct fmt::formatter<nano::uint128_union> : fmt::ostream_formatter
{
};
template <>
struct fmt::formatter<nano::uint256_union> : fmt::ostream_formatter
{
};
template <>
struct fmt::formatter<nano::uint512_union> : fmt::ostream_formatter
{
};
template <>
struct fmt::formatter<nano::hash_or_account> : fmt::ostream_formatter
{
};
template <>
struct fmt::formatter<nano::block_hash> : fmt::formatter<nano::uint256_union>
{
};
template <>
struct fmt::formatter<nano::public_key> : fmt::formatter<nano::uint256_union>
{
};
template <>
struct fmt::formatter<nano::qualified_root> : fmt::formatter<nano::uint512_union>
{
};
template <>
struct fmt::formatter<nano::root> : fmt::formatter<nano::hash_or_account>
{
};
