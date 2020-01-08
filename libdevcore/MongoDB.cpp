#include "MongoDB.h"
#include <mongocxx/client.hpp>
#include <thread>
#include <bsoncxx/builder/stream/document.hpp>
#include <vector>

using namespace bsoncxx::builder::stream;
using bsoncxx::builder::basic::kvp;
/**
 * \brief Convert the slice object to std::string.
 * \param slice the slice object.
 */
#define CONVERT_SLICE(slice) std::string((slice).data(), (slice).size())

namespace dev
{
    namespace db
    {
        class MongoDBWriteBatch : public WriteBatchFace
        {
        public:
            virtual void insert(Slice _key, Slice _value) override;
            virtual void kill(Slice _key) override;

            bool isEmpty() const { return m_insertMap.empty() && m_deleteSet.empty(); }
            const std::vector<std::pair<std::string, std::string>>& getInsertMap() const { return m_insertMap; }
            const std::vector<std::string>& getDeleteMap() const { return m_deleteSet; }

        protected:
        private:
            std::vector<std::pair<std::string, std::string>> m_insertMap;
            std::vector<std::string> m_deleteSet;
        };

        void MongoDBWriteBatch::insert(Slice _key, Slice _value)
        {
            m_insertMap.emplace_back(CONVERT_SLICE(_key), CONVERT_SLICE(_value));
        }

        void MongoDBWriteBatch::kill(Slice _key)
        {
            std::string strKey = CONVERT_SLICE(_key);
            auto iter = std::find_if(m_deleteSet.begin(), m_deleteSet.end(), [&strKey](std::string const& key) { return strKey == key; });
            if (iter == m_deleteSet.end())
            {
                m_deleteSet.push_back(strKey);
            }
        }

        MongoDB::MongoDB(boost::filesystem::path const& _path, std::shared_ptr<MongoDBInstance> const& _instance, std::string& _dbName)
        : m_instance(_instance), m_strDBName(_dbName)
        {
            std::cout << "MongoDB Database: " << m_strDBName << std::endl;
            m_strCollectionName = _path.filename().string();
            std::cout << "MongoDB Collection: " << m_strCollectionName << std::endl;
        }

        std::string MongoDB::lookup(Slice _key) const 
        {
            try 
            {
                auto client = m_instance->getPool().acquire();
                bsoncxx::types::b_binary key = { bsoncxx::binary_sub_type::k_binary, (uint32_t)_key.size(), (const unsigned char *)_key.data() };
                const auto str_key = CONVERT_SLICE(_key);
                bsoncxx::stdx::optional<bsoncxx::document::value> maybe_result = getCollection(*client).find_one(
                    bsoncxx::builder::basic::make_document(kvp("_id", key)));
                
                if (maybe_result)
                {
                    bsoncxx::document::element element = (*maybe_result).view()["value"];
                    bsoncxx::types::b_binary btvalue = element.get_binary();
                    return std::string(btvalue.bytes, btvalue.bytes + btvalue.size);
                }
            }
            catch (std::exception const& e)
            {
                BOOST_THROW_EXCEPTION(DatabaseError() << errinfo_comment(e.what()));
            }
            catch (...)
            {
                BOOST_THROW_EXCEPTION(DatabaseError() << errinfo_comment(boost::current_exception_diagnostic_information()));
            }

            return "";
        }

        bool MongoDB::exists(Slice _key) const 
        { 
            try
            {
                auto client = m_instance->getPool().acquire();
                bsoncxx::types::b_binary key = { bsoncxx::binary_sub_type::k_binary, (uint32_t)_key.size(), (const unsigned char *)_key.data() };
                auto str_key = CONVERT_SLICE(_key);
                bsoncxx::stdx::optional<bsoncxx::document::value> maybe_result = getCollection(*client).find_one(
                    bsoncxx::builder::basic::make_document(kvp("_id", key)));

                if (maybe_result)
                {
                    return true;
                }

                return false;
            }
            catch(const std::exception& e)
            {
                BOOST_THROW_EXCEPTION(DatabaseError() << errinfo_comment(e.what()));
            }
            catch (...)
            {
                BOOST_THROW_EXCEPTION(DatabaseError() << errinfo_comment(boost::current_exception_diagnostic_information()));
            }
        }

        void MongoDB::insert(Slice _key, Slice _value)
        {
            try
            {
                auto client = m_instance->getPool().acquire();
                auto collection = getCollection(*client);
                insertData(collection, CONVERT_SLICE(_key), CONVERT_SLICE(_value));
            }
            catch(const std::exception& e)
            {
                BOOST_THROW_EXCEPTION(DatabaseError() << errinfo_comment(e.what()));
            }
            catch (...)
            {
                BOOST_THROW_EXCEPTION(DatabaseError() << errinfo_comment(boost::current_exception_diagnostic_information()));
            }
        }

        void MongoDB::kill(Slice _key)
        {
            try
            {
                auto client = m_instance->getPool().acquire();
                auto collection = getCollection(*client);
                deleteData(collection, CONVERT_SLICE(_key));
            }
            catch(const std::exception& e)
            {
                BOOST_THROW_EXCEPTION(DatabaseError() << errinfo_comment(e.what()));
            }
            catch (...)
            {
                BOOST_THROW_EXCEPTION(DatabaseError() << errinfo_comment(boost::current_exception_diagnostic_information()));
            }
        }

        std::unique_ptr<WriteBatchFace> MongoDB::createWriteBatch() const
        {
            return std::unique_ptr<WriteBatchFace>(new MongoDBWriteBatch());
        }

        void MongoDB::commit(std::unique_ptr<WriteBatchFace> _batch)
        {
            if (!_batch)
            {
                BOOST_THROW_EXCEPTION(DatabaseError() << errinfo_comment("Cannot commit null batch"));
            }

            auto* batchPtr = dynamic_cast<MongoDBWriteBatch*>(_batch.get());
            if (!batchPtr)
            {
                BOOST_THROW_EXCEPTION(
                    DatabaseError() << errinfo_comment("Invalid batch type passed to MongoDB::commit"));
            }

            if (!batchPtr->isEmpty())
            {
                try
                {
                    auto client = m_instance->getPool().acquire();
                    auto collection = getCollection(*client);
                    auto bulk_write = collection.create_bulk_write();
                    auto const& insertMap = batchPtr->getInsertMap();
                    for (auto const& pair : insertMap)
                    {
                        bsoncxx::types::b_binary new_key = { bsoncxx::binary_sub_type::k_binary, (uint32_t)pair.first.size(), (const unsigned char *)pair.first.data() };
                        bsoncxx::types::b_binary new_value = { bsoncxx::binary_sub_type::k_binary, (uint32_t)pair.second.size(), (const unsigned char *)pair.second.data() };
                        
                        mongocxx::model::update_one update(bsoncxx::builder::basic::make_document(kvp("_id", new_key)),
                            bsoncxx::builder::basic::make_document(kvp("$set", bsoncxx::builder::basic::make_document(kvp("value", new_value)))));
                        update.upsert(true);

                        bulk_write.append(update);
                    }

                    auto const &deleteSet = batchPtr->getDeleteMap();
                    for (auto const &key : deleteSet)
                    {
                        bsoncxx::types::b_binary bt_key = { bsoncxx::binary_sub_type::k_binary, (uint32_t)key.size(), (const unsigned char *)key.data() };
                        
                        mongocxx::model::delete_one del(bsoncxx::builder::basic::make_document(kvp("_id", bt_key)));
                        
                        bulk_write.append(del);
                    }

                    do
                    {
                        try
                        {
                            auto result = bulk_write.execute();
                            break;
                        }
                        catch (...)
                        {
                            BOOST_THROW_EXCEPTION(DatabaseError() << errinfo_comment(boost::current_exception_diagnostic_information()));
                        }
                    } while (true);
                }
                catch(const std::exception& e)
                {
                    BOOST_THROW_EXCEPTION(DatabaseError() << errinfo_comment(e.what()));
                }
                catch (...)
                {
                    BOOST_THROW_EXCEPTION(DatabaseError() << errinfo_comment(boost::current_exception_diagnostic_information()));
                }
            }
        }

        void MongoDB::forEach(std::function<bool(Slice, Slice)> _f) const
        {
            try
            {
                auto client = m_instance->getPool().acquire();
                auto cursor = getCollection(*client).find({});
                for (auto&& doc : cursor) {
                    bsoncxx::document::element key_element = doc["_id"];
                    bsoncxx::types::b_binary btkey = key_element.get_binary();

                    bsoncxx::document::element value_element = doc["value"];
                    bsoncxx::types::b_binary btvalue = value_element.get_binary();

                    if (!_f(Slice((char const *)btkey.bytes, btkey.size), Slice((char const *)btvalue.bytes, btvalue.size)))
                    {
                        break;
                    }
                }
            }
            catch (const std::exception &e)
            {
                BOOST_THROW_EXCEPTION(DatabaseError() << errinfo_comment(e.what()));
            }
            catch (...)
            {
                BOOST_THROW_EXCEPTION(DatabaseError() << errinfo_comment(boost::current_exception_diagnostic_information()));
            }
        }

        void MongoDB::insert_string(std::string const& _key, std::string const& _value)
        {
            auto client = m_instance->getPool().acquire();
            auto collection = getCollection(*client);

            mongocxx::options::update options;
            options.upsert(true);
            collection.update_one(bsoncxx::builder::stream::document{} << "_id" << _key << finalize,
                bsoncxx::builder::stream::document{} << "$set" << open_document <<
                "value" << _value << close_document << finalize, options);
        }

        std::string MongoDB::lookup_string(std::string const& _key) const
        {
            try
            {
                auto client = m_instance->getPool().acquire();
                bsoncxx::stdx::optional<bsoncxx::document::value> maybe_result = getCollection(*client).find_one(
                { bsoncxx::builder::stream::document{} << "_id" << CONVERT_SLICE(_key) << finalize });

                if (maybe_result)
                {
                    bsoncxx::document::element element = (*maybe_result).view()["value"];
                    std::string value = element.get_utf8().value.to_string();
                    return value;
                }

                return "";
            }
            catch (const std::exception &e)
            {
                BOOST_THROW_EXCEPTION(DatabaseError() << errinfo_comment(e.what()));
            }
            catch (...)
            {
                BOOST_THROW_EXCEPTION(DatabaseError() << errinfo_comment(boost::current_exception_diagnostic_information()));
            }
        }

        mongocxx::v_noabi::collection MongoDB::getCollection(mongocxx::client &_client) const
        {
            return _client[m_strDBName][m_strCollectionName];
        }

        void MongoDB::insertData(mongocxx::v_noabi::collection &_collection, std::string const & _key, std::string const & _value)
        {
            mongocxx::options::update options;
            options.upsert(true);

            bsoncxx::types::b_binary new_key = { bsoncxx::binary_sub_type::k_binary, (uint32_t)_key.size(), (const unsigned char *)_key.data() };
            bsoncxx::types::b_binary new_value = { bsoncxx::binary_sub_type::k_binary, (uint32_t)_value.size(), (const unsigned char *)_value.data() };

            _collection.update_one(bsoncxx::builder::basic::make_document(kvp("_id", new_key)),
                bsoncxx::builder::basic::make_document(kvp("$set", bsoncxx::builder::basic::make_document(kvp("value", new_value)))), options);
        }

        void MongoDB::deleteData(mongocxx::v_noabi::collection &_collection, std::string const & _key)
        {
            bsoncxx::types::b_binary key = { bsoncxx::binary_sub_type::k_binary, (uint32_t)_key.size(), (const unsigned char *)_key.data() };
            _collection.delete_one(bsoncxx::builder::basic::make_document(kvp("_id", key)));
        }
    }
}