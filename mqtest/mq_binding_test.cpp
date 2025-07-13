#include "../mqserver/mq_binding.hpp"
#include <gtest/gtest.h>

RabbitMQ::BindingManager::ptr bmp;

class BindingTest : public testing::Environment{
    public:
        virtual void SetUp() override{
            bmp = std::make_shared<RabbitMQ::BindingManager>("./data/meta.db");
        }

        virtual void TearDown() override{
            //emp->clear();
            std::cout << "单元测试执行完毕后的环境清理！！" << std::endl;
        }
};

// TEST(binding_test,insert_test){
//     bmp->bind("exchange1","queue1","news.music.#",true);
//     bmp->bind("exchange1","queue2","news.sport.#",true);
//     bmp->bind("exchange1","queue3","news.food.#",true);
//     bmp->bind("exchange2","queue1","news.music.pop",true);
//     bmp->bind("exchange2","queue2","news.sport.football",true);
//     bmp->bind("exchange2","queue3","news.food.hotpot",true);
//     ASSERT_EQ(bmp->size(),6);
// }


// TEST(binding_test,select_test){
//     ASSERT_EQ(bmp->exists("exchange1","queue1"),true);
//     ASSERT_EQ(bmp->exists("exchange1","queue2"),true);
//     ASSERT_EQ(bmp->exists("exchange1","queue3"),true);
//     ASSERT_EQ(bmp->exists("exchange2","queue1"),true);
//     ASSERT_EQ(bmp->exists("exchange2","queue2"),true);
//     ASSERT_EQ(bmp->exists("exchange2","queue3"),true);

//     RabbitMQ::Binding::ptr bp = bmp->getBinding("exchange1","queue1");
//     ASSERT_NE(bp.get(),nullptr);
//     ASSERT_EQ(bp->exchange_name,std::string("exchange1"));
//     ASSERT_EQ(bp->msgqueue_name,std::string("queue1"));
//     ASSERT_EQ(bp->binding_key,std::string("news.music.#"));
// }

// TEST(binding_test,select_exchange_test){
//     RabbitMQ::MsgQueueBindingMap mqbm = bmp->getExchangeBindings("exchange1");
//     ASSERT_EQ(mqbm.size(),3);
//     ASSERT_NE(mqbm.find("queue1"),mqbm.end());
//     ASSERT_NE(mqbm.find("queue2"),mqbm.end());
// }

// TEST(binding_test,remove_queue_test){
//     bmp->removeQueueBindings("queue1");
//     ASSERT_EQ(bmp->exists("exchange1","queue1"),false);
//     ASSERT_EQ(bmp->exists("exchange2","queue1"),false);
// }

// TEST(binding_test,remove_exchange_test){
//     bmp->removeExchangeBindings("exchange1");
//     ASSERT_EQ(bmp->exists("exchange1","queue1"),false);
//     ASSERT_EQ(bmp->exists("exchange1","queue2"),false);
//     ASSERT_EQ(bmp->exists("exchange1","queue3"),false);
// }

// TEST(binding_test,remove_single_test){
//     ASSERT_EQ(bmp->exists("exchange2","queue2"),true);
//     bmp->unBind("exchange2","queue2");
//     ASSERT_EQ(bmp->exists("exchange2","queue2"),false);
//     ASSERT_EQ(bmp->exists("exchange2","queue3"),true);
// }

TEST(binding_test,recovery_test){
    ASSERT_EQ(bmp->exists("exchange2","queue3"),true);
    ASSERT_EQ(bmp->exists("exchange2","queue2"),true);
    ASSERT_EQ(bmp->exists("exchange2","queue1"),true);
    ASSERT_EQ(bmp->exists("exchange1","queue3"),true);
    ASSERT_EQ(bmp->exists("exchange1","queue2"),true);
    ASSERT_EQ(bmp->exists("exchange1","queue1"),true);
}

int main(int argc,char *argv[]){
    testing::InitGoogleTest(&argc,argv);
    testing::AddGlobalTestEnvironment(new BindingTest);
    RUN_ALL_TESTS();
    return 0;
}