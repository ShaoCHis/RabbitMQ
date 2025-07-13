#include <iostream>
#include "../mqcommon/mq_msg.pb.h"
#include "../mqcommon/mq_helper.hpp"
#include "../mqcommon/mq_logger.hpp"
#include <iostream>
#include <unordered_map>
#include <mutex>
#include <memory>

namespace RabbitMQ{
    struct Binding{
        using ptr = std::shared_ptr<Binding>;
        std::string exchange_name;
        std::string msgqueue_name;
        std::string binding_key;

        Binding(){}
        Binding(const std::string &ename,const std::string &qname,const std::string &key):
            exchange_name(ename),msgqueue_name(qname),binding_key(key){}
    };
    
    //队列跟绑定信息是一一对应的，一个队列只能绑定在一个交换机上 <queueName,bindingInfo>
    using MsgQueueBindingMap = std::unordered_map<std::string,Binding::ptr>;
    //一个交换机上可能有多个队列的绑定信息 <exchangeName,MsgQueues>
    using BindingMap = std::unordered_map<std::string,MsgQueueBindingMap>;

    //std::unordered_map<std::string,Binding::ptr>;队列与绑定
    //std::unordered_map<std::string,Binding::ptr>;交换机与绑定
    //采用上边两个结构，则删除交换机相关绑定信息的时候，不仅要删除交换机映射，还要删除对应队列中的映射，否则对象得不到释放
    class BindingMapper{
        public:
            BindingMapper(const std::string &dbfile):_sql_helper(dbfile){
                std::string path = FileHelper::parentDirectory(dbfile);
                FileHelper::createDirectory(path);
                _sql_helper.open();
                createTable();
            }
            void createTable(){
                #define CREATE_TABLE "create table if not exists binding_table\
                            (exchange_name varchar(32),msgqueue_name varchar(32),binding_key varchar(128));"
                assert(_sql_helper.exec(CREATE_TABLE,nullptr,nullptr));
            }
            void removeTable(){
                #define DROP_TABLE "drop table if exists binding_table;"
                assert(_sql_helper.exec(DROP_TABLE,nullptr,nullptr));
            }
            bool insert(Binding::ptr &binding){
                #define INSERT_BINDING "insert into binding_table values('%s','%s','%s');"
                char sql[4096] = {0};
                sprintf(sql,INSERT_BINDING,binding->exchange_name.c_str(),binding->msgqueue_name.c_str(),binding->binding_key.c_str());
                return _sql_helper.exec(sql,nullptr,nullptr);
            }
            void remove(const std::string &ename,const std::string &qname){
                #define REMOVE_BINDING "delete from binding_table where exchange_name='%s' and msgqueue_name='%s';"
                char sql[4096] = {0};
                sprintf(sql,REMOVE_BINDING,ename.c_str(),qname.c_str());
                _sql_helper.exec(sql,nullptr,nullptr);
            }
            void removeExchangeBindings(const std::string &ename){
                #define REMOVE_EXCHANGE "delete from binding_table where exchange_name='%s';"
                char sql[4096] = {0};
                sprintf(sql,REMOVE_EXCHANGE,ename.c_str());
                _sql_helper.exec(sql,nullptr,nullptr);
            }
            void removeMsgQueueBindings(const std::string &qname){
                #define REMOVE_QUEUE "delete from binding_table where msgqueue_name='%s';"
                char sql[4096] = {0};
                sprintf(sql,REMOVE_QUEUE,qname.c_str());
                _sql_helper.exec(sql,nullptr,nullptr);
            }
            BindingMap recovery(){
                #define RECOVERY "select exchange_name,msgqueue_name,binding_key from binding_table;"
                BindingMap result;
                _sql_helper.exec(RECOVERY,selectCallback,&result);
                return result;
            }
        private:
            static int selectCallback(void *arg,int numcol,char** row,char** fields){
                BindingMap* result = reinterpret_cast<BindingMap*>(arg);
                Binding::ptr bp = std::make_shared<Binding>(row[0],row[1],row[2]);
                //本身不存在会自动创建,同时防止交换机相关的绑定信息已经存在，因此不能直接创建队列映射进行添加，这样会覆盖历史数据
                //因此先获取交换机对应的映射对象，往里面添加数据
                //但是，若这时候没有交换机对应的映射信息，因此这里的获取要使用引用（会保证不存在则自动创建）
                MsgQueueBindingMap &qmap = (*result)[bp->exchange_name];
                qmap.insert(std::make_pair(bp->msgqueue_name,bp));
            }
        private:
            SqliteHelper _sql_helper;
    };

    class BindingManager{
        public:
            using ptr = std::shared_ptr<BindingManager>;
            BindingManager(const std::string &dbfile):_mapper(dbfile){
                _bindings = _mapper.recovery();
            }
            //durable 减少耦合性
            bool bind(const std::string &ename,const std::string &qname,const std::string &key,bool durable){
                //加锁，构造一个队列的绑定信息对象，添加映射关系
                std::lock_guard<std::mutex> lock(_mtx);
                auto it = _bindings.find(ename);
                if(it!=_bindings.end()&&it->second.find(qname)!=it->second.end()){
                    ELOG("该绑定信息 %s 已经存在交换机 %s 队列 %s 上",key.c_str(),ename.c_str(),qname.c_str());
                    return false;
                }
                //绑定信息是否需要持久化，取决于什么？交换机数据是持久化的以及队列数据也是持久化的
                Binding::ptr bp = std::make_shared<Binding>(ename,qname,key);
                if(durable){
                    bool ret = _mapper.insert(bp);
                    if(ret==false)  return false;
                }
                auto &qbmap = _bindings[ename];
                qbmap.insert(std::make_pair(qname,bp));
                return true;
            }
            void unBind(const std::string &ename,const std::string &qname){
                //加锁，构造一个队列的绑定信息对象，添加映射关系
                std::lock_guard<std::mutex> lock(_mtx);
                auto exchangeMap = _bindings.find(ename);
                if(exchangeMap==_bindings.end()){
                    ELOG("该交换机 %s 不存在",ename.c_str());
                    return;
                }
                auto queueMap = exchangeMap->second.find(qname);
                if(queueMap==exchangeMap->second.end()){
                    ELOG("该队列 %s 不存在交换机 %s 下",qname.c_str(),ename.c_str());
                    return;
                }
                _mapper.remove(ename,qname);
                exchangeMap->second.erase(qname);
                return;
            }
            void removeExchangeBindings(const std::string &ename){
                std::lock_guard<std::mutex> lock(_mtx);
                _mapper.removeExchangeBindings(ename);
                _bindings.erase(ename);
            }
            void removeQueueBindings(const std::string &qname){
                std::lock_guard<std::mutex> lock(_mtx);
                _mapper.removeMsgQueueBindings(qname);
                //遍历所有的交换机，删除相关队列的信息
                for(auto start = _bindings.begin();start!=_bindings.end();start++){
                    start->second.erase(qname);
                }
            }
            MsgQueueBindingMap getExchangeBindings(const std::string &ename){
                std::lock_guard<std::mutex> lock(_mtx);
                auto it = _bindings.find(ename);
                if(it==_bindings.end())
                    return MsgQueueBindingMap();
                return it->second;
            }

            Binding::ptr getBinding(const std::string &ename,const std::string &qname){
                std::lock_guard<std::mutex> lock(_mtx);
                auto eit = _bindings.find(ename);
                if(eit==_bindings.end())
                    return Binding::ptr();
                auto qit = eit->second.find(qname);
                if(qit==eit->second.end())
                    return Binding::ptr();
                return qit->second;
            }

            bool exists(const std::string &ename,const std::string &qname){
                std::lock_guard<std::mutex> lock(_mtx);
                auto eit = _bindings.find(ename);
                if(eit==_bindings.end())
                    return false;
                auto qit = eit->second.find(qname);
                if(qit==eit->second.end())
                    return false;
                return true;
            }

            size_t size(){
                size_t total_size = 0;
                std::lock_guard<std::mutex> lock(_mtx);
                for(auto start = _bindings.begin();start!=_bindings.end();start++){
                    total_size+=start->second.size();
                }
                return total_size;
            }

            void clear(){
                std::lock_guard<std::mutex> lock(_mtx);
                _mapper.removeTable();
                _bindings.clear();
            }

        private:
            std::mutex _mtx;
            BindingMapper _mapper;
            BindingMap _bindings;
    };
};