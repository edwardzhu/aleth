// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2015-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

/// @file
/// Class for importing snapshot from directory on disk
#pragma once

#include <boost/filesystem.hpp>
#include <memory>
#include <libdevcore/Common.h>
#include <libdevcore/Exceptions.h>
#include <libdevcore/FixedHash.h>
#include <libdevcore/Log.h>
#include <libdevcore/DBFactory.h>

namespace dev
{

namespace eth
{

class Client;
class BlockChainImporterFace;
class SnapshotStorageFace;
class StateImporterFace;

class SnapshotImporter
{
public:
    SnapshotImporter(StateImporterFace& _stateImporter, BlockChainImporterFace& _bcImporter): 
        m_stateImporter(_stateImporter), 
        m_blockChainImporter(_bcImporter)
    {
        m_database = db::DBFactory::create(db::DatabaseKind::MongoDB, boost::filesystem::path("configure"));
    }

    ~SnapshotImporter()
    {
        m_database.reset();
    }

    void import(SnapshotStorageFace const& _snapshotStorage, h256 const& _genesisHash);

private:
    void importStateChunks(SnapshotStorageFace const& _snapshotStorage, h256s const& _stateChunkHashes, h256 const& _stateRoot);
    void importBlockChunks(SnapshotStorageFace const& _snapshotStorage, h256s const& _blockChunkHashes);

    StateImporterFace& m_stateImporter;
    BlockChainImporterFace& m_blockChainImporter;
    std::unique_ptr<db::DatabaseFace> m_database;

    Logger m_logger{createLogger(VerbosityInfo, "snap")};
};

}
}
