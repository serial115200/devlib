#include <stdio.h>
#include <json-c/json.h>

int main() {
    // 创建一个 JSON 对象
    json_object *jobj = json_object_new_object();
    
    // 添加一些字段
    json_object *jstring = json_object_new_string("Hello, World!");
    json_object *jint = json_object_new_int(42);
    json_object *jbool = json_object_new_boolean(1);
    
    json_object_object_add(jobj, "message", jstring);
    json_object_object_add(jobj, "number", jint);
    json_object_object_add(jobj, "flag", jbool);
    
    // 打印 JSON 字符串
    printf("JSON: %s\n", json_object_to_json_string(jobj));
    
    // 清理
    json_object_put(jobj);
    
    return 0;
}

