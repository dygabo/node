#include "async_context_frame.h"  // NOLINT(build/include_inline)

#include "env-inl.h"
#include "node_errors.h"
#include "node_external_reference.h"
#include "tracing/traced_value.h"
#include "util-inl.h"

#include "debug_utils-inl.h"

#include "v8.h"

using v8::Context;
using v8::FunctionCallbackInfo;
using v8::FunctionTemplate;
using v8::Isolate;
using v8::Local;
using v8::Object;
using v8::Value;

#if defined(NODE_USE_NATIVE_ALS) && NODE_USE_NATIVE_ALS

namespace node {

//
// Scope helper
//
AsyncContextFrame::Scope::Scope(Isolate* isolate, Local<Value> object)
    : isolate_(isolate) {
  auto prior = AsyncContextFrame::exchange(isolate, object);
  prior_.Reset(isolate, prior);
}

AsyncContextFrame::Scope::Scope(Isolate* isolate, AsyncContextFrame* frame)
    : Scope(isolate, frame->object()) {}

AsyncContextFrame::Scope::~Scope() {
  auto value = prior_.Get(isolate_);
  AsyncContextFrame::exchange(isolate_, value);
}

//
// Constructor
//
AsyncContextFrame::AsyncContextFrame(Environment* env,
                                     Local<Object> obj,
                                     Local<Object> current,
                                     Local<Value> key,
                                     Local<Value> value)
    : BaseObject(env, obj),
      parent_(env->isolate(), current),
      key_(env->isolate(), key),
      value_(env->isolate(), value),
      enabled_(true) {
  key_.SetWeak();
}

Local<Value> AsyncContextFrame::current(Isolate* isolate) {
  return isolate
      ->GetEnteredOrMicrotaskContext()
      ->GetContinuationPreservedEmbedderData();
}

// NOTE: It's generally recommended to use AsyncContextFrame::Scope
// but sometimes (such as enterWith) a direct exchange is needed.
Local<Value> AsyncContextFrame::exchange(Isolate* isolate,
                                                Local<Value> value) {
  auto prior = current(isolate);
  isolate
      ->GetEnteredOrMicrotaskContext()
      ->SetContinuationPreservedEmbedderData(value);
  return prior;
}

Local<Value> AsyncContextFrame::disable(Isolate* isolate, Local<Value> key) {
  if (key_ == key) {
    enabled_ = false;
    return v8::True(isolate);
  }

  auto parent = parent_.Get(isolate);
  Environment* env = Environment::GetCurrent(isolate);
  if (parent.IsEmpty() || !AsyncContextFrame::HasInstance(env, parent)) {
    return v8::False(isolate);
  }

  return Unwrap<AsyncContextFrame>(parent)->disable(isolate, key);
}

Local<Value> AsyncContextFrame::get(Isolate* isolate, Local<Value> key) {
  if (key_ == key && enabled_) {
    return value_.Get(isolate);
  }

  auto parent = parent_.Get(isolate);
  if (parent.IsEmpty()) {
    return v8::Undefined(isolate);
  }

  Environment* env = Environment::GetCurrent(isolate);
  if (!AsyncContextFrame::HasInstance(env, parent)) {
    return v8::Undefined(isolate);
  }

  return Unwrap<AsyncContextFrame>(parent)->get(isolate, key);
}

//
// JS Static Methods
//
void AsyncContextFrame::New(const FunctionCallbackInfo<Value>& info) {
  CHECK(info.IsConstructCall());
  CHECK_GE(info.Length(), 2);

  Environment* env = Environment::GetCurrent(info);

  auto key = info[0];
  auto value = info[1];

  // Parent frame is optional. Defaults to current frame.
  auto parent = AsyncContextFrame::HasInstance(env, info[2])
                    ? Unwrap<AsyncContextFrame>(info[2])->object()
                    : current(info.GetIsolate()).As<Object>();

  new AsyncContextFrame(env, info.This(), parent, key, value);
}

void AsyncContextFrame::Current(const FunctionCallbackInfo<Value>& info) {
  info.GetReturnValue().Set(AsyncContextFrame::current(info.GetIsolate()));
}

void AsyncContextFrame::Get(const FunctionCallbackInfo<Value>& info) {
  Environment* env = Environment::GetCurrent(info);
  Isolate* isolate = env->isolate();

  auto maybeFrame = current(isolate);
  if (AsyncContextFrame::HasInstance(env, maybeFrame)) {
    AsyncContextFrame* acf = Unwrap<AsyncContextFrame>(maybeFrame);
    info.GetReturnValue().Set(acf->get(isolate, info[0]));
  }
}

void AsyncContextFrame::Disable(const FunctionCallbackInfo<Value>& info) {
  Isolate* isolate = info.GetIsolate();
  AsyncContextFrame* acf = Unwrap<AsyncContextFrame>(current(isolate));
  info.GetReturnValue().Set(acf->disable(isolate, info[0]));
}

void AsyncContextFrame::Exchange(const FunctionCallbackInfo<Value>& info) {
  info.GetReturnValue().Set(
      AsyncContextFrame::exchange(info.GetIsolate(), info[0]));
}

void AsyncContextFrame::Descend(const FunctionCallbackInfo<Value>& info) {
  CHECK_GE(info.Length(), 2);

  Environment* env = Environment::GetCurrent(info);
  Isolate* isolate = info.GetIsolate();

  auto key = info[0];
  auto value = info[1];

  // Parent frame is optional. Defaults to current frame.
  auto parent = AsyncContextFrame::HasInstance(env, info[2])
                    ? Unwrap<AsyncContextFrame>(info[2])->object()
                    : current(isolate).As<Object>();

  auto acf = AsyncContextFrame::Create(env, key, value, parent);

  info.GetReturnValue().Set(
      AsyncContextFrame::exchange(isolate, acf->object()));
}

//
// Class construction infra
//
Local<FunctionTemplate> AsyncContextFrame::GetConstructorTemplate(
    Environment* env) {
  return GetConstructorTemplate(env->isolate_data());
}

Local<FunctionTemplate> AsyncContextFrame::GetConstructorTemplate(
    IsolateData* isolate_data) {
  Local<FunctionTemplate> tmpl =
      isolate_data->async_context_frame_ctor_template();
  if (tmpl.IsEmpty()) {
    Isolate* isolate = isolate_data->isolate();
    tmpl = NewFunctionTemplate(isolate, New);
    tmpl->InstanceTemplate()->SetInternalFieldCount(
        BaseObject::kInternalFieldCount);
    tmpl->SetClassName(FIXED_ONE_BYTE_STRING(isolate, "AsyncContextFrame"));
    SetMethodNoSideEffect(isolate, tmpl, "get", Get);
    SetMethodNoSideEffect(isolate, tmpl, "current", Current);
    SetMethod(isolate, tmpl, "disable", Disable);
    SetMethod(isolate, tmpl, "exchange", Exchange);
    SetMethod(isolate, tmpl, "descend", Descend);
    isolate_data->set_async_context_frame_ctor_template(tmpl);
  }
  return tmpl;
}

bool AsyncContextFrame::HasInstance(Environment* env,
                                    v8::Local<v8::Value> object) {
  return GetConstructorTemplate(env->isolate_data())->HasInstance(object);
}

BaseObjectPtr<AsyncContextFrame> AsyncContextFrame::Create(
    Environment* env,
    Local<Value> key,
    Local<Value> value,
    Local<Object> current) {
  Local<Object> obj;

  if (UNLIKELY(!GetConstructorTemplate(env)
                    ->InstanceTemplate()
                    ->NewInstance(env->context())
                    .ToLocal(&obj))) {
    return BaseObjectPtr<AsyncContextFrame>();
  }

  return MakeBaseObject<AsyncContextFrame>(env, obj, current, key, value);
}

void AsyncContextFrame::RegisterExternalReferences(
    ExternalReferenceRegistry* registry) {
  registry->Register(New);
  registry->Register(Get);
  registry->Register(Disable);
  registry->Register(Current);
  registry->Register(Exchange);
  registry->Register(Descend);
}

void AsyncContextFrame::CreatePerContextProperties(Local<Object> target,
                                                   Local<Value> unused,
                                                   Local<Context> context,
                                                   void* priv) {
  Environment* env = Environment::GetCurrent(context);

  auto t = AsyncContextFrame::GetConstructorTemplate(env);
  SetConstructorFunction(context, target, "AsyncContextFrame", t);
}

void AsyncContextFrame::MemoryInfo(MemoryTracker* tracker) const {
  tracker->TrackField("parent", parent_);
  tracker->TrackField("key", key_);
  tracker->TrackField("value", value_);
}

}  // namespace node

NODE_BINDING_CONTEXT_AWARE_INTERNAL(
    async_context_frame, node::AsyncContextFrame::CreatePerContextProperties)
NODE_BINDING_EXTERNAL_REFERENCE(
    async_context_frame, node::AsyncContextFrame::RegisterExternalReferences)

#else
namespace node {
void EmptyProperties(Local<Object> target,
                     Local<Value> unused,
                     Local<Context> context,
                     void* priv) {}

void EmptyExternals(ExternalReferenceRegistry* registry) {}
}  // namespace node

NODE_BINDING_CONTEXT_AWARE_INTERNAL(async_context_frame, node::EmptyProperties)

NODE_BINDING_EXTERNAL_REFERENCE(async_context_frame, node::EmptyExternals)
#endif  // defined(NODE_USE_NATIVE_ALS) && NODE_USE_NATIVE_ALS
