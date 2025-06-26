/***
 * packaged_task::get_future
 * std::future与Package_task合并使用，实例化的对象可以对一个函数进行二次封装
 * 可以通过get_future获取一个future对象，来获取封装的这个函数的异步执行结果
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
    
    std::future<int> fut = task.get_future();
    std::cout << "create thrd\n";
    std::thread(std::move(task),33).detach();

    int value = fut.get();
    std::cout << "The triple of 33 is " << value << std::endl;
    return 0;
}
