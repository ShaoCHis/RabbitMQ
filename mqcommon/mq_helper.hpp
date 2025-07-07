#ifndef _MQ_HELPER_H
#define _MQ_HELPER_H

#include "mq_logger.hpp"

#include <iostream>
#include <string>
#include <vector>
#include <sqlite3.h>
#include <random>
#include <sstream>
#include <iomanip>
#include <atomic>


namespace RabbitMQ{
//Sqlite工具类
class SqliteHelper{
public:
    //定义回调函数类型
    typedef int(*SqliteCallBack)(void*,int,char**,char**);
    SqliteHelper(const std::string &dbfile):_dbfile(dbfile),_handler(nullptr){}
    ~SqliteHelper() {
        close();
        delete _handler;
    }

    bool open(int safe_level=SQLITE_OPEN_FULLMUTEX){
        //int sqlite3_open_v2(const char* filename,sqlite3 **ppDb,int flags,const char *zVfs);
        int ret = sqlite3_open_v2(_dbfile.c_str(),&_handler,SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE|safe_level,nullptr);
        if(ret!=SQLITE_OK){
            ELOG("创建/打开sqlite数据库失败:%s",sqlite3_errmsg(_handler));
            return false;
        }
        return true;
    }

    bool exec(const std::string &sql,SqliteCallBack cb,void *arg){
        //int sqlite3_exec(sqlite3*,char *sql,int (*callback)(void*,int,char**,char**),void *arg,char *err);
        int ret = sqlite3_exec(_handler,sql.c_str(),cb,arg,nullptr);
        if(ret!=SQLITE_OK){
            ELOG("%s \n执行语句失败,Err Msg:%s",sql.c_str(),sqlite3_errmsg(_handler));
            return false;
        }
        return true;
    }

    void close(){
        if(_handler) sqlite3_close_v2(_handler);
    }
private:
    std::string _dbfile;
    sqlite3* _handler;
};

class StrHelper{
public:
    static size_t split(const std::string &str,const std::string &sep,std::vector<std::string> &result){
        size_t pos,idx=0;
        while(idx<str.size()){
            pos = str.find(sep,idx);
            if(pos==std::string::npos){
                result.push_back(str.substr(idx));
                return result.size();
            }
            if(pos==idx) {
                idx = pos + sep.size();
                continue;
            }
            result.push_back(str.substr(idx,pos-idx));
            idx=pos+sep.size();
        }
        return result.size();
    }
};

class UUID{
public:
    static std::string uuid(){
        std::random_device rd;
        std::mt19937_64 generator(rd());
        std::uniform_int_distribution<int> distribution(0,255);
        std::stringstream ss;
        for(int i = 0;i<8;i++){
            ss<< std::setw(2) << std::setfill('0') << std::hex << distribution(generator);
            if(i==3||i==5||i==7)
                ss<<"-";
        }
        static std::atomic<size_t> seq(1);
        size_t num = seq.fetch_add(1);
        for(int i = 7;i>=0;i--){
            ss<< std::setw(2) << std::setfill('0') << std::hex << ((num>>(i*8))&0xff);
            if(i==6) ss<<"-";
        }
        return ss.str();
    }
};

class FileHelper{
public:
    FileHelper(const std::string &filename):_filename(filename) {}

    bool exists(){

    }

    size_t size();

    bool read(std::string &body);
    bool read(std::string &body,size_t offset,size_t len);
    //直接写入
    bool write(const std::string &body);
    //将body的数据，写入到offset偏移位置
    bool write(const std::string &body,size_t offset);

    bool createFile();
    bool removeFile();
    bool createDirectory();
    bool removeDirectory();

    //获取文件的父级目录
    static std::string parentDirectory(const std::string &filename);

private:
    std::string _filename;
};

}
#endif