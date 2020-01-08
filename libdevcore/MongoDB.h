#pragma once
#include "db.h"
#include <boost/filesystem.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/pool.hpp>
#include <memory>

namespace dev 
{
    namespace db 
    {
        class MongoDBInstance 
        {
        public:
            MongoDBInstance(const std::string& _uri) : m_instance{}, m_pool(_uri.empty() ? mongocxx::uri{} : mongocxx::uri{ _uri }) {}
            mongocxx::pool& getPool() { return m_pool; }
        protected:
        private:
            mongocxx::instance m_instance;
            mongocxx::pool m_pool;
        };

        class MongoDB : public DatabaseFace
        {
        public:
            MongoDB(boost::filesystem::path const& _path, std::shared_ptr<MongoDBInstance> const& _instance, std::string& _dbName);
            virtual std::string lookup(Slice _key) const override;
            virtual bool exists(Slice _key) const override;
            virtual void insert(Slice _key, Slice _value) override;
            virtual void kill(Slice _key) override;
            virtual void insert_string(std::string const& _key, std::string const& _value) override;
            virtual std::string lookup_string(std::string const& _key) const override;

            virtual std::unique_ptr<WriteBatchFace> createWriteBatch() const override;
            virtual void commit(std::unique_ptr<WriteBatchFace> _batch) override;

            // A database must implement the `forEach` method that allows the caller
            // to pass in a function `f`, which will be called with the key and value
            // of each record in the database. If `f` returns false, the `forEach`
            // method must return immediately.
            virtual void forEach(std::function<bool(Slice, Slice)> _f) const override;

        private:
            std::shared_ptr<MongoDBInstance> m_instance;
            std::string m_strCollectionName;
            std::string m_strDBUri;
            std::string m_strDBName;
            mongocxx::v_noabi::collection getCollection(mongocxx::client &_client) const;
            void insertData(mongocxx::v_noabi::collection &_collection, std::string const & _key, std::string const & _value);
            void deleteData(mongocxx::v_noabi::collection &_collection, std::string const & _key);
        };
    }
}