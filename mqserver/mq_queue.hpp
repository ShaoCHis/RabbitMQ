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
    //1.定义队列类
    struct Queue{
        using ptr = std::shared_ptr<Queue>;
        //1.队列名称
        std::string name;
        //2.队列持久化标志
        bool durable;
        //3.是否独占
        bool exclusive;
        //4.是否自动删除
        bool auto_delete;
        //5.其他参数
        std::unordered_map<std::string,std::string> args;
        Queue(std::string qname,bool qdurable,
            bool qexclusive,bool qauto_delete,
            std::unordered_map<std::string,std::string> &qargs)
            :name(qname),durable(qdurable),
            exclusive(qexclusive),auto_delete(qauto_delete),
            args(qargs){}
        Queue(){}
        
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

    //2.定义队列数据持久化类 ----- 存储在sqlite数据库中
    class QueueMapper{
        public:
            QueueMapper(const std::string &dbfile):_sql_helper(dbfile){
                std::string parentPath = FileHelper::parentDirectory(dbfile);
                FileHelper::createDirectory(parentPath);
                assert(_sql_helper.open());
                createTable();
            }

            void createTable(){
                #define CREATE_TABLE "create table if not exists queue_table(\
                    name varchar(32) primary key,\
                    durable int,\
                    exclusive int,\
                    auto_delete int,\
                    args varchar(128));"
                bool ret = _sql_helper.exec(CREATE_TABLE,nullptr,nullptr);
                if(ret == false){
                    abort();//直接异常退出程序
                }
            }

            void removeTable(){
                #define DROP_TABLE "drop table if exists queue_table;"
                bool ret = _sql_helper.exec(DROP_TABLE,nullptr,nullptr);
                if(ret == false){
                    abort();
                }
            }

            void insert(Queue::ptr &queue){
                #define INSERT_SQL "insert into queue_table values('%s',%d,%d,%d,'%s');"
                char sql_str[4096] = {0};
                sprintf(sql_str,INSERT_SQL,
                    queue->name.c_str(),queue->durable,queue->exclusive,queue->auto_delete,queue->getArgs().c_str());
                _sql_helper.exec(sql_str,nullptr,nullptr);
            }

            void remove(const std::string &name){
                #define DELETE_SQL "delete from queue_table where name = '%s';"
                char sql_str[4096] = {0};
                sprintf(sql_str,DELETE_SQL,name.c_str());
                _sql_helper.exec(sql_str,nullptr,nullptr);
            }

            //恢复，读取所有的数据，进行返回
            using QueueMap = std::unordered_map<std::string,Queue::ptr>;
            std::unordered_map<std::string,Queue::ptr> recovery(){
                #define GETALL_SQL "select name,durable,exclusive,auto_delete,args from queue_table;"
                QueueMap result;
                _sql_helper.exec(GETALL_SQL,selectCallBack,&result);
                return result;
            }
        private:
            //类成员函数参数是有一个自己的指针，静态函数没有
            static int selectCallBack(void* args,int numcol,char** row,char** fields){
                QueueMap *result = reinterpret_cast<QueueMap*>(args);
                auto exp = std::make_shared<Queue>();
                exp->name = row[0];
                exp->durable = (bool)row[1];
                exp->exclusive = (bool)std::stoi(row[2]);
                exp->auto_delete = (bool)row[3];
                if(row[4]) exp->setArgs(row[4]);
                result->insert(std::make_pair(exp->name,exp));
                return 0;
            }
        private:
            SqliteHelper _sql_helper;
    };

    //3.定义队列数据内存管理类
    class QueueManager{
        public:
            using ptr = std::shared_ptr<QueueManager>;
            QueueManager(const std::string &dbfile):_mapper(dbfile){
                _queues = _mapper.recovery();
            }
            //声明队列
            void declareQueue(const std::string &name,
                bool durable,bool exclusive,bool auto_delete,
                std::unordered_map<std::string,std::string> &args){
                std::lock_guard<std::mutex> lock(_mtx);
                auto it = _queues.find(name);
                if(it!=_queues.end()){
                    DLOG("队列 %s 已经存在！！",name.c_str());
                    return;
                }
                auto queue = std::make_shared<Queue>(name,durable,exclusive,auto_delete,args);
                if(durable){
                    _mapper.insert(queue);
                }
                _queues.insert(std::make_pair(name,queue));
            }
            //删除队列
            void deleteQueue(const std::string &name){
                std::lock_guard<std::mutex> lock(_mtx);
                auto it = _queues.find(name);
                if(it==_queues.end()){
                    DLOG("队列 %s 不存在！！",name.c_str());
                }
                if(it->second->durable) _mapper.remove(name);
                _queues.erase(name);
            }

            //获取指定队列对象
            Queue::ptr selectQueue(const std::string &name){
                std::lock_guard<std::mutex> lock(_mtx);
                auto it = _queues.find(name);
                if(it==_queues.end()){
                    DLOG("队列 %s 不存在！！",name.c_str());
                    return Queue::ptr();
                }
                return it->second;
            }

            //判断队列是否存在
            bool exists(const std::string &name){
                std::lock_guard<std::mutex> lock(_mtx);
                auto it = _queues.find(name);
                if(it==_queues.end()){
                    DLOG("队列 %s 不存在！！",name.c_str());
                    return false;
                }
                return true;
            }

            size_t size(){
                std::lock_guard<std::mutex> lock(_mtx);
                return _queues.size();
            }

            //清理所有队列数据
            void clear(){
                std::lock_guard<std::mutex> lock(_mtx);
                _mapper.removeTable();
                _queues.clear();
            }
        private:
            std::mutex _mtx;
            QueueMapper _mapper;
            std::unordered_map<std::string,Queue::ptr> _queues;
    };
};


