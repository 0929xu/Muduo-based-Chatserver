#include "json.hpp"
using json = nlohmann::json;

#include <iostream>
#include <vector>
#include <string>
using namespace std;


// json 序列化示例1
string func1() {
    // json对象实例化
    json js;
    js["msg_sype"] = 2;
    js["from"] = "zhang san";
    js["to"] = "Li si";
    js["msg"] = "hahahahahaha!";
    
    // cout<<js<<endl; // {"from":"zhang san","msg":"hahahahahaha!","msg_sype":2,"to":"Li si"}

    // 怎么将json通过网络发送出去呢？ 使用dump方法转成字符串
    string sendBuf = js.dump(); // dump() 序列化
    // cout<<sendBuf.c_str()<<endl;  // 网络上传送的是char*，所以通过c_str()转换成char*类型
                                  // {"from":"zhang san","msg":"hahahahahaha!","msg_sype":2,"to":"Li si"}
    return sendBuf;
}

// json 序列化示例2
string func2() {
    json js;
    // 添加数组
    js["id"] = {1,2,3,4,5};
    // 添加key-value
    js["name"] = "zhang san";
    // 添加对象
    js["msg"]["zhang san"] = "hello wochong!";
    js["msg"]["liu liu"] = "yiyaha!!!!";
    // 上面等同于下面这一句一次性添加数组对象
    js["msg"] = {{"zhang san","hello wochong!"},{"liu liu","yiyaha!!!!"}};
    // cout<<js<<endl; // {"id":[1,2,3,4,5],"msg":{"liu liu":"yiyaha!!!!","zhang san":"hello wochong!"},"name":"zhang san"}
    return js.dump();
}

// json 序列化示例3
string func3() {
    json js;

    // json 可以直接序列化一个vector容器
    vector<int> vec;
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);
    js["list"] = vec;

    // json 可以直接序列化一个map容器
    map<int, string> m;
    m.insert({1, "usaqi"});
    m.insert({2, "chikawa"});
    m.insert({3, "chiiwa"});
    js["path"] = m;
    
    string sendBuf = js.dump(); // json数据对象 -> 序列化json字符串
    // cout<< sendBuf <<endl;
    return js.dump();

}

int main() {
    // 加入现在序列化的json字符串通过网络发送过来，由recvBuf接住
    string recvBuf = func3();
    // 数据反序列化  json字符串 ->反序列化 数据对象（看成容器，方便访问）
    json jsbuf = json::parse(recvBuf);

    // func1
    // cout<< jsbuf["msg_sype"] << endl;    // 2
    // cout<< jsbuf["from"] << endl;        // "zhang san"
    // cout<< jsbuf["to"] << endl;          // "Li si"
    // cout<< jsbuf["msg"] << endl;         // "hahahahahaha!"

    // func2
    // cout<< jsbuf["id"] <<endl;
    // auto arr = jsbuf["id"];  // [1,2,3,4,5]
    // cout << arr[2] << endl;  // 3

    // auto msgjs = jsbuf["msg"];
    // cout<< msgjs["zhang san"] << endl;  // "hello wochong!"
    // cout<< msgjs["liu liu"] << endl;    // "yiyaha!!!!"

    vector<int> vec = jsbuf["list"];
    for(int &v : vec) {
        cout<< v <<" "; 
    }
    cout << endl;             // 1 2 3 

    map<int, string> mymap = jsbuf["path"];
    for(auto &p : mymap) {
        cout<< p.first << " " << p.second << endl;
    }
    // 1 usaqi
    // 2 chikawa
    // 3 chiiwa
    return 0;
}