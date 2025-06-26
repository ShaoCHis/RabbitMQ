/**
 * Promise
 * A future object referring to the same shared state as the promise
 * promise中得到的future对象与promise共享值状态，也就是future.get()会等待promise.set()
 */

#include <iostream>
#include <functional>   //std::ref传值使用
#include <thread>
#include <future>
#include <chrono>

void print_int(std::future<int>& fut){
    std::cout << "func print_int start\n";
    int x = fut.get();
    std::cout << "value:" << x << std::endl;
}

int main(){
    std::promise<int> prom;                         //create promise
    std::future<int> fut = prom.get_future();       //engagement with future
    std::cout << "engagement with future\n";
    std::thread thrd(print_int,std::ref(fut));      //send future to new thread
    thrd.detach();
    std::cout << "create thread and sleep for 2s\n";
    std::this_thread::sleep_for(std::chrono::seconds(2));
    prom.set_value(10);                             //fulfill promise
    std::cout << "promise set value\n";
    //thrd.join();                                    //(synchronizes with getting the future)
    getchar();
    return 0;
}
