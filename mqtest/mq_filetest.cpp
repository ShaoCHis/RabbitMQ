#include "../mqcommon/mq_helper.hpp"

int main(){
    RabbitMQ::FileHelper helper("../mqcommon/mq_logger.hpp");
    DLOG("是否存在：%d",helper.exists());
    DLOG("文件大小：%ld",helper.size());

    RabbitMQ::FileHelper tmp_helper("./aaa/bbb/ccc/test.cpp");
    if(tmp_helper.exists()==false){
        std::string path = RabbitMQ::FileHelper::parentDirectory(tmp_helper.getName());
        if(RabbitMQ::FileHelper(path).exists()==false){
            RabbitMQ::FileHelper::createDirectory(path);
        }
        RabbitMQ::FileHelper::createFile(tmp_helper.getName());
    }

    // std::string body;
    // helper.read(body);
    // DLOG("%s",body.c_str());
    // tmp_helper.write(body);

    // char str[16] = {0};
    // tmp_helper.read(str,8,11);
    // DLOG("[%s]",str);
    // tmp_helper.write("_MQ_LOGGER_H",8,12);

    //tmp_helper.rename("./aaa/bbb/ccc/test.cpp");
    //RabbitMQ::FileHelper::removeDirectory(tmp_helper.getName());
    //RabbitMQ::FileHelper::removeDirectory("./aaa");
    return 0;
}