#include <nano/lib/logging.hpp>
#include <nano/node/make_store.hpp>
#include <nano/node/nodeconfig.hpp>
#include <nano/store/lmdb/lmdb.hpp>
#include <nano/store/rocksdb/rocksdb.hpp>

std::unique_ptr<nano::store::component> nano::make_store (nano::logger & logger, std::filesystem::path const & path, nano::ledger_constants & constants, bool read_only, bool add_db_postfix, nano::node_config const & node_config)
{
	if (node_config.database_backend == nano::database_backend::rocksdb)
	{
		return std::make_unique<nano::store::rocksdb::component> (logger, add_db_postfix ? path / "rocksdb" : path, constants, node_config.rocksdb_config, read_only);
	}
	if (node_config.rocksdb_config.enable && node_config.database_backend == nano::database_backend::lmdb)
	{
		// rocksdb.enable is true in config, but database_backend is set to LMDB in config
		logger.critical (nano::log::type::config, "Legacy RocksDb setting detected in config file.");
		logger.info (nano::log::type::config, "Edit node_config.toml and use 'database_backend' in the node section to re-enable RocksDb");
		std::exit (EXIT_FAILURE);
	}

	return std::make_unique<nano::store::lmdb::component> (logger, add_db_postfix ? path / "data.ldb" : path, constants, node_config.diagnostics_config.txn_tracking, node_config.block_processor_batch_max_time, node_config.lmdb_config, node_config.backup_before_upgrade);
}
