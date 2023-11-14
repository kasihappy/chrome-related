//
// Created by lby on 23-11-12.
//
// ToDo:
// 创建一个C++模板类

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "include/libplatform/libplatform.h"
#include "include/v8-array-buffer.h"
#include "include/v8-context.h"
#include "include/v8-exception.h"
#include "include/v8-external.h"
#include "include/v8-function.h"
#include "include/v8-initialization.h"
#include "include/v8-isolate.h"
#include "include/v8-local-handle.h"
#include "include/v8-object.h"
#include "include/v8-persistent-handle.h"
#include "include/v8-primitive.h"
#include "include/v8-script.h"
#include "include/v8-snapshot.h"
#include "include/v8-template.h"
#include "include/v8-value.h"

class document{
public:
    document(){cookie = 0; child = nullptr;}
    document(int val, document* chi, v8::Local<v8::Context> CurrentContext): cookie(val), child(chi), context(CurrentContext){}
    document* child;
    v8::Local<v8::Context> context;
    int cookie;
};

void GetCookie(v8::Local<v8::String> property,const v8::PropertyCallbackInfo<v8::Value>& info) {
    v8::Isolate* isolate = info.GetIsolate();
    v8::Local<v8::Object> self = info.Holder();
    v8::Local<v8::Context> cur = isolate->GetCurrentContext();
    document* doc = static_cast<document*>(self->GetAlignedPointerFromInternalField(0));
    int cookie = doc->cookie;
    v8::Local<v8::Integer> val = v8::Integer::New(isolate, cookie);
    if (doc->context != cur) {
        val = v8::Integer::New(isolate, 0);
        printf("trying to access cookie over context!!\n");
    }
    info.GetReturnValue().Set(val);
}

void GetChild(v8::Local<v8::String> property,const v8::PropertyCallbackInfo<v8::Value>& info) {
    v8::Isolate* isolate = info.GetIsolate();
    v8::Local<v8::Object> self = info.Holder();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    document* doc = static_cast<document*>(self->GetAlignedPointerFromInternalField(0));
    v8::Local<v8::FunctionTemplate> templ = v8::FunctionTemplate::New(isolate, nullptr);
    templ->InstanceTemplate()->SetInternalFieldCount(1);
    templ->InstanceTemplate()->SetAccessor(v8::String::NewFromUtf8(isolate, "cookie").ToLocalChecked(), GetCookie);
    document* child = doc->child;
    v8::Local<v8::Object> obj = templ->GetFunction(context).ToLocalChecked()->NewInstance(context).ToLocalChecked();
    obj->SetAlignedPointerInInternalField(0, child);
    info.GetReturnValue().Set(obj);
}



/*void SetCookie(v8::Local<v8::String> property, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info) {
    v8::Isolate *isolate = info.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    v8::Local<v8::Object> self = info.Holder();
    v8::Local<v8::External> wrap = v8::Local<v8::External>::Cast(self->GetInternalField(0));
    void* ptr = wrap->Value();
    static_cast<document*>(ptr)->cookie = value->Int32Value(context).ToChecked();
}*/

/*static void LogCallback(const v8::FunctionCallbackInfo <v8::Value> &info) {
    if (info.Length() < 1) return;
    v8::Isolate *isolate = info.GetIsolate();
    v8::HandleScope scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    v8::Local<v8::Value> arg = info[0];
    v8::String::Utf8Value value(isolate, arg);
    printf("%s\n", *value);
}*/

int main(int argc, char* argv[]) {
    // 初始化v8
    v8::V8::InitializeICUDefaultLocation(argv[0]);
    v8::V8::InitializeExternalStartupData(argv[0]);
    std::unique_ptr<v8::Platform> platform = v8::platform::NewDefaultPlatform();
    v8::V8::InitializePlatform(platform.get());
    v8::V8::Initialize();

    // 创建一个isolate并且进入它
    v8::Isolate::CreateParams create_params;
    create_params.array_buffer_allocator = v8::ArrayBuffer::Allocator::NewDefaultAllocator();
    v8::Isolate* isolate = v8::Isolate::New(create_params);
    {
      v8::Isolate::Scope isolate_scope(isolate);

      // 创建handle scope
      v8::HandleScope handleScope(isolate);

      // 创建类模板document，每个context都有其document
      v8::Local<v8::ObjectTemplate> global = v8::ObjectTemplate::New(isolate);
      v8::Local<v8::FunctionTemplate> docTemplate = v8::FunctionTemplate::New(isolate, nullptr);
      docTemplate->InstanceTemplate()->SetInternalFieldCount(1);
      docTemplate->SetClassName(v8::String::NewFromUtf8(isolate, "Document").ToLocalChecked());
      docTemplate->InstanceTemplate()->SetAccessor(v8::String::NewFromUtf8(isolate, "cookie").ToLocalChecked(), GetCookie);
      docTemplate->InstanceTemplate()->SetAccessor(v8::String::NewFromUtf8(isolate, "child").ToLocalChecked(), GetChild);
      global->Set(isolate, "Document", docTemplate);


      // 创建contextA,B
      v8::Local<v8::Context> context_A = v8::Context::New(isolate);
      v8::Local<v8::Context> context_B = v8::Context::New(isolate);
      v8::Local<v8::Object> docA = docTemplate->GetFunction(context_A).ToLocalChecked()->NewInstance(context_A).ToLocalChecked();
      v8::Local<v8::Object> docB = docTemplate->GetFunction(context_B).ToLocalChecked()->NewInstance(context_B).ToLocalChecked();
      document* documentB = new document(456, nullptr, context_B);
      document* documentA = new document(123, documentB, context_A);
      docA->SetAlignedPointerInInternalField(0, documentA);
      docB->SetAlignedPointerInInternalField(0, documentB);
      context_A->Global()->Set(context_A, v8::String::NewFromUtf8(isolate, "documentA").ToLocalChecked(), docA).Check();
      context_B->Global()->Set(context_B, v8::String::NewFromUtf8(isolate, "documentB").ToLocalChecked(), docB).Check();
      // 进入contextA执行一些代码
      v8::Context::Scope context_scopeA(context_A);
      {



        //将JS代码以字符串形式存储
        const char JS_source[] = R"(
            documentA.child.cookie;
        )";

        // 创建一个用以执行的v8字符串
        v8::Local<v8::String> source = v8::String::NewFromUtf8Literal(isolate, JS_source);

        // 以这个字符串为代码编译
        v8::Local<v8::Script> script = v8::Script::Compile(context_A, source).ToLocalChecked();
        // 执行编译好的代码并且获取返回值
        v8::Local<v8::Value> res = script->Run(context_A).ToLocalChecked();
        // 将这个值转化为uint32
        uint32_t number = res->Uint32Value(context_A).ToChecked();
          printf("A.cookie = %u\n", number);
      }

      // 进入contextB执行一些代码
      v8::Context::Scope context_scopeB(context_B);
      {

          //将JS代码以字符串形式存储
          const char JS_source[] = R"(
            documentB.cookie;
          )";

          // 创建一个用以执行的v8字符串
          v8::Local<v8::String> source = v8::String::NewFromUtf8Literal(isolate, JS_source);

          // 以这个字符串为代码编译
          v8::Local<v8::Script> script = v8::Script::Compile(context_B, source).ToLocalChecked();

          // 执行编译好的代码并且获取返回值
          v8::Local<v8::Value> res = script->Run(context_B).ToLocalChecked();

          // 将这个值转化为uint32
          uint32_t number = res->Uint32Value(context_B).ToChecked();
          printf("B.cookie = %u\n", number);
      }

    }
    // 清除isolate并清除v8
    isolate->Dispose();
    v8::V8::Dispose();
    v8::V8::DisposePlatform();
    delete create_params.array_buffer_allocator;
    return 0;

}