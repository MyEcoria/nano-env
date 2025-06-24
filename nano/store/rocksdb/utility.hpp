#pragma once

#include <nano/store/rocksdb/db_val.hpp>

#include <variant>

#include <rocksdb/slice.h>
#include <rocksdb/utilities/transaction_db.h>

namespace nano::store
{
class transaction;
}

namespace nano::store::rocksdb
{
auto tx (store::transaction const & transaction_a) -> std::variant<::rocksdb::Transaction *, ::rocksdb::ReadOptions *>;

/**
 * Converts a db_val to rocksdb::Slice for RocksDB operations
 */
inline ::rocksdb::Slice to_slice (nano::store::db_val const & val)
{
	return ::rocksdb::Slice{ reinterpret_cast<char const *> (val.data ()), val.size () };
}

/**
 * Creates a db_val from rocksdb::Slice for read operations
 */
inline nano::store::db_val from_slice (::rocksdb::Slice const & slice)
{
	auto buffer = std::make_shared<std::vector<uint8_t>> (
	reinterpret_cast<uint8_t const *> (slice.data ()),
	reinterpret_cast<uint8_t const *> (slice.data ()) + slice.size ());
	return nano::store::db_val (buffer);
}
}
