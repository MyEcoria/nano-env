#pragma once

#include <nano/lib/diagnosticsconfig.hpp>
#include <nano/lib/lmdbconfig.hpp>
#include <nano/lib/logging.hpp>
#include <nano/lib/rocksdbconfig.hpp>
#include <nano/node/nodeconfig.hpp>

#include <chrono>

namespace nano
{
class ledger_constants;
class lmdb_config;
class rocksdb_config;
class txn_tracking_config;
}

namespace nano::store
{
class component;
}

namespace nano
{
std::unique_ptr<nano::store::component> make_store (nano::logger &, std::filesystem::path const & path, nano::ledger_constants & constants, bool read_only = false, bool add_db_postfix = true, nano::node_config node_config = nano::node_config{});
}
