#include "../mqserver/mq_exchange.hpp"
#include <gtest/gtest.h>

RabbitMQ::ExchangeManager::ptr emp;

class ExchangeTest : public testing::Environment{
    public:
        virtual void SetUp() override{
            emp = std::make_shared<RabbitMQ::ExchangeManager>("./data/meta.db");
        }

        virtual void TearDown() override{
            //emp->clear();
            std::cout << "单元测试执行完毕后的环境清理！！" << std::endl;
        }
};

TEST(exchange_test,insert_test){
    //std::unordered_map<std::string,std::string> map = {{"k1","v1"},{"k2","v2"}};
    std::unordered_map<std::string,std::string> map;
    emp->declareExchange("exchange1",RabbitMQ::ExchangeType::DIRECT,true,false,map);
    emp->declareExchange("exchange2",RabbitMQ::ExchangeType::DIRECT,true,false,map);
    emp->declareExchange("exchange3",RabbitMQ::ExchangeType::DIRECT,true,false,map);
    emp->declareExchange("exchange4",RabbitMQ::ExchangeType::DIRECT,true,false,map);
    ASSERT_EQ(emp->size(),4);
}


TEST(exchange_test,select_test){
    RabbitMQ::Exchange::ptr exp = emp->selectExchange("exchange3");
    ASSERT_NE(emp->exists("exchange3"),false);
    ASSERT_EQ(exp->name,"exchange3");
    ASSERT_EQ(exp->durable,true);
    ASSERT_EQ(exp->type,RabbitMQ::ExchangeType::DIRECT);
    ASSERT_EQ(exp->auto_delete,false);
    ASSERT_EQ(exp->getArgs(),std::string("k2=v2&k1=v1"));
}

TEST(exchange_test,remove_test){
    emp->deleteExchange("exchange3");
    RabbitMQ::Exchange::ptr exp = emp->selectExchange("exchange3");
    ASSERT_EQ(exp.get(),nullptr);
    ASSERT_EQ(emp->exists("exchange3"),false);
}

TEST(exchange_test,recovery_test){
    ASSERT_EQ(emp->exists("exchange1"),true);
    ASSERT_EQ(emp->exists("exchange2"),false);
    ASSERT_EQ(emp->exists("exchange3"),false);
    ASSERT_EQ(emp->exists("exchange4"),true);
    RabbitMQ::Exchange::ptr exp = emp->selectExchange("exchange1");
    ASSERT_NE(exp.get(),nullptr);
    ASSERT_EQ(exp->name,"exchange1");
    ASSERT_EQ(exp->durable,true);
    ASSERT_EQ(exp->type,RabbitMQ::ExchangeType::DIRECT);
    ASSERT_EQ(exp->auto_delete,false);
    ASSERT_EQ(exp->getArgs(),std::string("k2=v2&k1=v1"));
}

int main(int argc,char *argv[]){
    testing::InitGoogleTest(&argc,argv);
    testing::AddGlobalTestEnvironment(new ExchangeTest);
    RUN_ALL_TESTS();
    return 0;
}