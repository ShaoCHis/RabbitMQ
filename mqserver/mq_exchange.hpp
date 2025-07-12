#include <iostream>
#include "../mqcommon/mq_msg.pb.h"
#include "../mqcommon/mq_helper.hpp"
#include "../mqcommon/mq_logger.hpp"
#include <iostream>
#include <unordered_map>
#include <mutex>
#include <memory>

using namespace RabbitMQ;

namespace RabbitMQ{
    //1.定义交换机类
    struct Exchange{
        using ptr = std::shared_ptr<Exchange>;
        //1.交换机名称
        std::string name;
        //2.交换机类型
        ExchangeType type;
        //3.交换机持久化标志
        bool durable;
        //4.是否自动删除标志
        bool auto_delete;
        //5.其他参数
        std::unordered_map<std::string,std::string> args;
        Exchange(std::string ename,ExchangeType etype,
            bool edurable,bool eauto_delete,
            std::unordered_map<std::string,std::string> &eargs)
            :name(ename),type(etype),
            durable(edurable),auto_delete(eauto_delete),
            args(eargs){}
        Exchange(){}
        
        //将map组织成字符串，key=value&key=value进行持久化存储
        void setArgs(const std::string &args_str){ //内部解析 args_str字符串，存储到对象中
            std::vector<std::string> sub_args;
            StrHelper::split(args_str,"&",sub_args);
            for(const std::string &arg:sub_args){
                size_t pos = arg.find("=");
                std::string key = arg.substr(0,pos);
                std::string val = arg.substr(pos+1);
                args.insert(std::make_pair(key,val));
            }
        }

        //将args中的内容序列化，返回一个字符串
        std::string getArgs(){
            std::string result;
            for(auto start = args.begin();start!=args.end();start++){
                result += start->first + "=" + start->second + "&";
            }
            result.pop_back();
            return result;
        }   
    };

    //2.定义交换机数据持久化类 ----- 存储在sqlite数据库中
    class ExchangeMapper{
        public:
            ExchangeMapper(const std::string &dbfile):_sql_helper(dbfile){
                std::string parentPath = FileHelper::parentDirectory(dbfile);
                FileHelper::createDirectory(parentPath);
                assert(_sql_helper.open());
                createTable();
            }

            void createTable(){
                #define CREATE_TABLE "create table if not exists exchange_table(\
                    name varchar(32) primary key,\
                    type int,\
                    durable int,\
                    auto_delete int,\
                    args varchar(128));"
                bool ret = _sql_helper.exec(CREATE_TABLE,nullptr,nullptr);
                if(ret == false){
                    abort();//直接异常退出程序
                }
            }

            void removeTable(){
                #define DROP_TABLE "drop table if exists exchange_table;"
                bool ret = _sql_helper.exec(DROP_TABLE,nullptr,nullptr);
                if(ret == false){
                    abort();
                }
            }

            void insert(Exchange::ptr &exchange){
                #define INSERT_SQL "insert into exchange_table values('%s',%d,%d,%d,'%s');"
                char sql_str[4096] = {0};
                sprintf(sql_str,INSERT_SQL,
                    exchange->name.c_str(),exchange->type,exchange->durable,exchange->auto_delete,exchange->getArgs().c_str());
                _sql_helper.exec(sql_str,nullptr,nullptr);
            }

            void remove(const std::string &name){
                #define DELETE_SQL "delete from exchange_table where name = '%s';"
                char sql_str[4096] = {0};
                sprintf(sql_str,DELETE_SQL,name.c_str());
                _sql_helper.exec(sql_str,nullptr,nullptr);
            }

            //恢复，读取所有的数据，进行返回
            using ExchangeMap = std::unordered_map<std::string,Exchange::ptr>;
            std::unordered_map<std::string,Exchange::ptr> recovery(){
                #define GETALL_SQL "select name,type,durable,auto_delete,args from exchange_table;"
                ExchangeMap result;
                _sql_helper.exec(GETALL_SQL,selectCallBack,&result);
                return result;
            }
        private:
            //类成员函数参数是有一个自己的指针，静态函数没有
            static int selectCallBack(void* args,int numcol,char** row,char** fields){
                ExchangeMap *result = reinterpret_cast<ExchangeMap*>(args);
                auto exp = std::make_shared<Exchange>();
                exp->name = row[0];
                exp->type = (ExchangeType)std::stoi(row[1]);
                exp->durable = (bool)row[2];
                exp->auto_delete = (bool)row[3];
                if(row[4]) exp->setArgs(row[4]);
                result->insert(std::make_pair(exp->name,exp));
                return 0;
            }
        private:
            SqliteHelper _sql_helper;
    };

    //3.定义交换机数据内存管理类
    class ExchangeManager{
        public:
            using ptr = std::shared_ptr<ExchangeManager>;
            ExchangeManager(const std::string &dbfile):_mapper(dbfile){
                _exchanges = _mapper.recovery();
            }
            //声明交换机
            void declareExchange(const std::string &name,
                ExchangeType type,bool durable,bool auto_delete,
                std::unordered_map<std::string,std::string> &args){
                std::lock_guard<std::mutex> lock(_mtx);
                auto it = _exchanges.find(name);
                if(it!=_exchanges.end()){
                    DLOG("交换机 %s 已经存在！！",name.c_str());
                    return;
                }
                auto exchange = std::make_shared<Exchange>(name,type,durable,auto_delete,args);
                if(durable){
                    _mapper.insert(exchange);
                }
                _exchanges.insert(std::make_pair(name,exchange));
            }
            //删除交换机
            void deleteExchange(const std::string &name){
                std::lock_guard<std::mutex> lock(_mtx);
                auto it = _exchanges.find(name);
                if(it==_exchanges.end()){
                    DLOG("交换机 %s 不存在！！",name.c_str());
                }
                if(it->second->durable) _mapper.remove(name);
                _exchanges.erase(name);
            }

            //获取指定交换机对象
            Exchange::ptr selectExchange(const std::string &name){
                std::lock_guard<std::mutex> lock(_mtx);
                auto it = _exchanges.find(name);
                if(it==_exchanges.end()){
                    DLOG("交换机 %s 不存在！！",name.c_str());
                    return Exchange::ptr();
                }
                return it->second;
            }

            //判断交换机是否存在
            bool exists(const std::string &name){
                std::lock_guard<std::mutex> lock(_mtx);
                auto it = _exchanges.find(name);
                if(it==_exchanges.end()){
                    DLOG("交换机 %s 不存在！！",name.c_str());
                    return false;
                }
                return true;
            }

            size_t size(){
                std::lock_guard<std::mutex> lock(_mtx);
                return _exchanges.size();
            }

            //清理所有交换机数据
            void clear(){
                std::lock_guard<std::mutex> lock(_mtx);
                _mapper.removeTable();
                _exchanges.clear();
            }
        private:
            std::mutex _mtx;
            ExchangeMapper _mapper;
            std::unordered_map<std::string,Exchange::ptr> _exchanges;
    };
};