#include "../mqserver/mq_queue.hpp"
#include <gtest/gtest.h>

RabbitMQ::QueueManager::ptr qmp;

class QueueTest : public testing::Environment{
    public:
        virtual void SetUp() override{
            qmp = std::make_shared<RabbitMQ::QueueManager>("./data/meta.db");
        }

        virtual void TearDown() override{
            //qmp->clear();
            std::cout << "单元测试执行完毕后的环境清理！！" << std::endl;
        }
};

TEST(queue_test,insert_test){
    std::unordered_map<std::string,std::string> map = {{"k1","v1"},{"k2","v2"}};
    //std::unordered_map<std::string,std::string> map;
    qmp->declareQueue("queue1",true,false,false,map);
    qmp->declareQueue("queue2",true,false,false,map);
    qmp->declareQueue("queue3",true,false,false,map);
    qmp->declareQueue("queue4",true,false,false,map);
    ASSERT_EQ(qmp->size(),4);
}


TEST(queue_test,select_test){
    RabbitMQ::Queue::ptr qup = qmp->selectQueue("queue2");
    ASSERT_NE(qmp->exists("queue2"),false);
    ASSERT_EQ(qup->name,"queue2");
    ASSERT_EQ(qup->durable,true);
    ASSERT_EQ(qup->exclusive,false);
    ASSERT_EQ(qup->auto_delete,false);
    ASSERT_EQ(qup->getArgs(),std::string("k2=v2&k1=v1"));
}

TEST(queue_test,remove_test){
    qmp->deleteQueue("queue2");
    RabbitMQ::Queue::ptr qup = qmp->selectQueue("queue2");
    ASSERT_EQ(qup.get(),nullptr);
    ASSERT_EQ(qmp->exists("queue2"),false);
}

TEST(queue_test,recovery_test){
    ASSERT_EQ(qmp->exists("queue1"),true);
    ASSERT_EQ(qmp->exists("queue2"),false);
    ASSERT_EQ(qmp->exists("queue3"),true);
    ASSERT_EQ(qmp->exists("queue4"),true);
    RabbitMQ::Queue::ptr qup = qmp->selectQueue("queue1");
    ASSERT_NE(qup.get(),nullptr);
    ASSERT_EQ(qup->name,"queue1");
    ASSERT_EQ(qup->durable,true);
    ASSERT_EQ(qup->exclusive,false);
    ASSERT_EQ(qup->auto_delete,false);
    ASSERT_EQ(qup->getArgs(),std::string("k2=v2&k1=v1"));
}

int main(int argc,char *argv[]){
    testing::InitGoogleTest(&argc,argv);
    testing::AddGlobalTestEnvironment(new QueueTest);
    RUN_ALL_TESTS();
    return 0;
}