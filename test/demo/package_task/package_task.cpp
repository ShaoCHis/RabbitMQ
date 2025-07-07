/***
 * packaged_task::get_future
 * std::future与Package_task合并使用，实例化的对象可以对一个函数进行二次封装
 * 可以通过get_future获取一个future对象，来获取封装的这个函数的异步执行结果
 *
 */

#include <iostream>
#include <utility>
#include <future>
#include <thread>
#include <chrono>

int triple(int x) {
    std::cout << "triple function start!\n";
    std::this_thread::sleep_for(std::chrono::seconds(3));
    return x*3; 
}

int main(){
    std::packaged_task<int(int)> task(triple);  //packaged_task
    
    //task(33)
    // * packaged_task可以当作一个可调用对象来调用执行任务
    //但是它又不能完全的当作一个函数来使用,以下两行会报错
    //std::async(std::launch::async,task,33);
    //std::thread(task,33);
    //但是我们可以把task定义成为一个指针，传到线程中，进行解引用执行
    //但是如果单纯指针指向一个对象，存在生命周期问题，很有可能出现风险，思想就是在堆上new对象，用智能指针管理生命周期
    // std::shared_ptr<std::packaged_task<int(int)>> ptask = std::make_shared<std::packaged_task<int(int)>>(triple);
    // std::future<int> fut = ptask->get_future();
    // std::thread thr([ptask](){
    //     (*ptask)(33);
    // });
    // int sum = fut.get();

    std::future<int> fut = task.get_future();
    std::cout << "create thrd\n";
    std::thread(std::move(task),33).detach();

    int value = fut.get();
    std::cout << "The triple of 33 is " << value << std::endl;
    return 0;
}
