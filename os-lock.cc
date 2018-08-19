#include <node.h>
#include <uv.h>

#include "lock.h"

namespace OSLock {
using node::AsyncResource;
using node::UVException;
using v8::Exception;
using v8::Function;
using v8::FunctionCallbackInfo;
using v8::HandleScope;
using v8::Isolate;
using v8::Local;
using v8::Object;
using v8::Persistent;
using v8::String;
using v8::Value;

struct LockWork {
  uv_work_t req;
  int res;
  Persistent<Function> cb;

  int fd;
  long long start;
  long long length;
  bool exclusive;
  bool immediate;
};

void Lock(uv_work_t* req) {
  LockWork* work = static_cast<LockWork*>(req->data);
  work->res = uv_translate_sys_error(lock(
    work->fd, work->start, work->length, work->exclusive, work->immediate
  ));
}

void Unlock(uv_work_t* req) {
  LockWork* work = static_cast<LockWork*>(req->data);
  work->res = uv_translate_sys_error(unlock(
    work->fd, work->start, work->length
  ));
}

static void LockFinish(uv_work_t* req, int status) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope handleScope(isolate);

  LockWork* work = static_cast<LockWork*>(req->data);

  Local<Value> argv[] = {
    work->res ? UVException(isolate, work->res, "fcntl") :
      Null(isolate).As<Value>()
  };

  AsyncResource(
    isolate, isolate->GetCurrentContext()->Global(), "os-lock"
  ).MakeCallback(
    Local<Function>::New(isolate, work->cb), 1, argv
  );

  work->cb.Reset();

  delete work;
}

void LockAsync(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();

  if (args.Length() != 6) {
    isolate->ThrowException(Exception::TypeError(
      String::NewFromUtf8(isolate, "Wrong number of arguments")
    ));
    return;
  }

  LockWork* work = new LockWork();

  work->fd = args[0]->Int32Value();
  work->start = args[1]->IntegerValue();
  work->length = args[2]->IntegerValue();
  work->exclusive = args[3]->BooleanValue();
  work->immediate = args[4]->BooleanValue();

  Local<Function> callback = args[5].As<Function>();
  work->cb.Reset(isolate, callback);

  work->req.data = work;

  uv_queue_work(uv_default_loop(), &work->req, Lock, LockFinish);
}

void UnlockAsync(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();

  if (args.Length() != 4) {
    isolate->ThrowException(Exception::TypeError(
      String::NewFromUtf8(isolate, "Wrong number of arguments")
    ));
    return;
  }

  LockWork* work = new LockWork();

  work->fd = args[0]->Int32Value();
  work->start = args[1]->IntegerValue();
  work->length = args[2]->IntegerValue();

  Local<Function> callback = args[3].As<Function>();
  work->cb.Reset(isolate, callback);

  work->req.data = work;

  uv_queue_work(uv_default_loop(), &work->req, Unlock, LockFinish);
}

void Initialize(Local<Object> exports) {
  NODE_SET_METHOD(exports, "lock", LockAsync);
  NODE_SET_METHOD(exports, "unlock", UnlockAsync);
}

NODE_MODULE(NODE_GYP_MODULE_NAME, Initialize)
}
