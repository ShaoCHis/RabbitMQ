#include <iostream>
#include <string>
#include <vector>

size_t split(const std::string &str,const std::string &sep,std::vector<std::string> &result){
    //news....music.#.pop
    //分割的思想：
    /**
     * 1.从0位置开始查找指定字符的位置，找到之后进行分割
     * 2.从上次查找的位置继续向后查找指定字符
     */
    size_t pos,idx=0;
    while(idx<str.size()){
        pos = str.find(sep,idx);
        if(pos==std::string::npos){
            //没有找到，则从查找位置截取到末尾
            result.push_back(str.substr(idx));
            return result.size();
        }
        if(pos==idx) {
            idx = pos + sep.size();
            continue;  // 代表两个分隔符之间没有数据，或者说查找起始位置就是分隔符
        }
        //idx到pos的字符段进行截取
        result.push_back(str.substr(idx,pos-idx));
        idx=pos+sep.size();
    }
    return result.size();
}

int main(){
    std::string str = "news....musice.#.pop";
    std::vector<std::string> array;
    int n = split(str,".",array);
    std::cout << "n:" << n << std::endl;
    for(auto& item:array){
        std::cout << item << std::endl;
    }
    return 0;
}