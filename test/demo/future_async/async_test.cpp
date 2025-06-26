/**
 * std::future表示一个异步操作结果，能够阻塞当前线程直到异步操作完成，从而确保我们在获取结果时不会遇到未完成的操作
 * *** 应用场景
 * ****** 异步任务：需要在后台执行耗时操作时，std::future可以用来表示异步任务的结果，将主线程分离，实现任务并行处理
 * ****** 并发控制：等待某些任务执行完成后才能继续其他操作
 */


/**
 * std::async 异步调用一个函数  policy：launch::async(异步)|launch::deferred(阻塞等待get的调用)
 */

#include <iostream>
#include <thread>
#include <future>
#include <chrono>

int Add(int num1,int num2){
    std::cout << "sum operation start!" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(5));
    std::cout << "sum operation end!" << std::endl;
    return num1+num2;
}

int main(){
    //std::async(func,...)      std::async(policy,func,...)
    std::cout << "------------ 1 ----------------" << std::endl;
    //在执行get获取异步结果时，才会执行异步任务
    std::future<int> res = std::async(std::launch::deferred,Add,10,20);
    //直接异步执行,内部会创建工作线程，异步的完成任务
    //std::future<int> res = std::async(std::launch::async,Add,10,20);
    std::this_thread::sleep_for(std::chrono::seconds(2));
    std::cout << "------------ 2 ----------------" << std::endl;
    std::cout << "result:" << res.get() << std::endl;
    std::cout << "------------ 3 ----------------" << std::endl;
    return 0;
}

