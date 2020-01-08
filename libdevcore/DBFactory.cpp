// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2018-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.
#include "DBFactory.h"
#include "FileSystem.h"
#include "LevelDB.h"
#include "MemoryDB.h"
#include "MongoDB.h"
#include "libethcore/Exceptions.h"

#if ALETH_ROCKSDB
#include "RocksDB.h"
#endif

namespace dev
{
namespace db
{
namespace fs = boost::filesystem;
namespace po = boost::program_options;

auto g_kind = DatabaseKind::LevelDB;
fs::path g_dbPath;
std::string g_dbName;
std::string g_dbUrl;

/// A helper type to build the table of DB implementations.
///
/// More readable than std::tuple.
/// Fields are not initialized to allow usage of construction with initializer lists {}.
struct DBKindTableEntry
{
    DatabaseKind kind;
    char const* name;
};

/// The table of available DB implementations.
///
/// We don't use a map to avoid complex dynamic initialization. This list will never be long,
/// so linear search only to parse command line arguments is not a problem.
DBKindTableEntry dbKindsTable[] = {
    {DatabaseKind::LevelDB, "leveldb"},
#if ALETH_ROCKSDB
    {DatabaseKind::RocksDB, "rocksdb"},
#endif
    {DatabaseKind::MemoryDB, "memorydb"},
    {DatabaseKind::MongoDB, "mongodb"}
};

static std::shared_ptr<MongoDBInstance> s_pMongoInstance;
static std::mutex s_MongoMutex;

void setDBUrl(std::string const& dbUrl)
{
    g_dbUrl = dbUrl;
}

void setDbName(std::string const& dbName)
{
    g_dbName = dbName;
}

void setDatabaseKindByName(std::string const& _name)
{
    for (auto& entry : dbKindsTable)
    {
        if (_name == entry.name)
        {
            g_kind = entry.kind;
            return;
        }
    }

    BOOST_THROW_EXCEPTION(
        eth::InvalidDatabaseKind() << errinfo_comment("invalid database name supplied: " + _name));
}

void setDatabaseKind(DatabaseKind _kind)
{
    g_kind = _kind;
}

void setDatabasePath(std::string const& _path)
{
    g_dbPath = fs::path(_path);
}

bool isDiskDatabase()
{
    switch (g_kind)
    {
        case DatabaseKind::LevelDB:
        case DatabaseKind::RocksDB:
            return true;
        default:
            return false;
    }
}

DatabaseKind databaseKind()
{
    return g_kind;
}

fs::path databasePath()
{
    auto path = g_dbPath.empty() ? getDataDir() : g_dbPath;
    std::cout << "Database Path: " << path.string() << std::endl;
    return path;
}

po::options_description databaseProgramOptions(unsigned _lineLength)
{
    // It must be a static object because boost expects const char*.
    static std::string const description = [] {
        std::string names;
        for (auto const& entry : dbKindsTable)
        {
            if (!names.empty())
                names += ", ";
            names += entry.name;
        }

        return "Select database implementation. Available options are: " + names + ".";
    }();

    po::options_description opts("DATABASE OPTIONS", _lineLength);
    auto add = opts.add_options();

    add("db",
        po::value<std::string>()->value_name("<name>")->default_value("leveldb")->notifier(
            setDatabaseKindByName),
        description.data());

    add("db-path",
        po::value<std::string>()
            ->value_name("<path>")
            ->default_value(getDataDir().string())
            ->notifier(setDatabasePath),
        "Database path (for non-memory database options)\n");
    add("db-url", po::value<std::string>()->value_name("<path>")->default_value("mongodb://127.0.0.1:27017")->notifier(setDBUrl), "The mongoDB url.");
    add("db-name", po::value<std::string>()->value_name("<db-name>")->default_value("ethereum")->notifier(setDbName), "The MongoDB name.");

    return opts;
}

std::unique_ptr<DatabaseFace> DBFactory::create()
{
    return create(databasePath());
}

std::unique_ptr<DatabaseFace> DBFactory::create(fs::path const& _path)
{
    return create(g_kind, _path);
}

std::unique_ptr<DatabaseFace> DBFactory::create(DatabaseKind _kind)
{
    return create(_kind, databasePath());
}

std::unique_ptr<DatabaseFace> DBFactory::create(DatabaseKind _kind, fs::path const& _path)
{
    switch (_kind)
    {
    case DatabaseKind::LevelDB:
        return std::unique_ptr<DatabaseFace>(new LevelDB(_path));
        break;
#if ALETH_ROCKSDB
    case DatabaseKind::RocksDB:
        return std::unique_ptr<DatabaseFace>(new RocksDB(_path));
        break;
#endif
    case DatabaseKind::MemoryDB:
        // Silently ignore path since the concept of a db path doesn't make sense
        // when using an in-memory database
        return std::unique_ptr<DatabaseFace>(new MemoryDB());
        break;
    case DatabaseKind::MongoDB:
        initMongoDB(g_dbUrl);
        return std::make_unique<MongoDB>(_path, s_pMongoInstance, g_dbName);
        break;
    default:
        assert(false);
        return {};
    }
}

void DBFactory::initMongoDB(std::string const& _uri)
{
    std::cout << "Initialize MongoDB: " << _uri << std::endl;
    std::lock_guard<std::mutex> lock(s_MongoMutex);
    if (!s_pMongoInstance)
    {
        s_pMongoInstance = std::make_shared<MongoDBInstance>(_uri);
    }
}

}  // namespace db
}  // namespace dev