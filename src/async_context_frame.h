#ifndef SRC_ASYNC_CONTEXT_FRAME_H_
#define SRC_ASYNC_CONTEXT_FRAME_H_

#if defined(NODE_WANT_INTERNALS) && NODE_WANT_INTERNALS
#if defined(NODE_USE_NATIVE_ALS) && NODE_USE_NATIVE_ALS

#include "base_object.h"
#include "v8.h"

#include <cstdint>

namespace node {

class ExternalReferenceRegistry;

class AsyncContextFrame final : public BaseObject {
 public:
  AsyncContextFrame(Environment* env,
                    v8::Local<v8::Object> object,
                    v8::Local<v8::Object> current,
                    v8::Local<v8::Value> key,
                    v8::Local<v8::Value> value);

  AsyncContextFrame() = delete;

  class Scope {
   public:
    explicit Scope(v8::Isolate* isolate, v8::Local<v8::Value> object);
    explicit Scope(v8::Isolate* isolate, AsyncContextFrame* frame);
    ~Scope();

   private:
    v8::Isolate* isolate_;
    v8::Global<v8::Value> prior_;
  };

  static v8::Local<v8::Value> current(v8::Isolate* env);
  static v8::Local<v8::Value> exchange(v8::Isolate* env,
                                       v8::Local<v8::Value> value);
  v8::Local<v8::Value> get(v8::Isolate* env, v8::Local<v8::Value> key);
  v8::Local<v8::Value> disable(v8::Isolate* env, v8::Local<v8::Value> key);

  static void New(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void Get(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void Disable(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void Current(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void Exchange(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void Descend(const v8::FunctionCallbackInfo<v8::Value>& args);

  static v8::Local<v8::FunctionTemplate> GetConstructorTemplate(
      IsolateData* isolate_data);
  inline static v8::Local<v8::FunctionTemplate> GetConstructorTemplate(
      Environment* env);
  static bool HasInstance(Environment* env, v8::Local<v8::Value> value);
  static BaseObjectPtr<AsyncContextFrame> Create(
      Environment* env,
      v8::Local<v8::Value> key,
      v8::Local<v8::Value> value,
      v8::Local<v8::Object> current = v8::Local<v8::Object>());

  static void RegisterExternalReferences(ExternalReferenceRegistry* registry);
  static void CreatePerContextProperties(v8::Local<v8::Object> target,
                                         v8::Local<v8::Value> unused,
                                         v8::Local<v8::Context> context,
                                         void* priv);

  // If this needs memory info, swap the next two lines
  void MemoryInfo(MemoryTracker* tracker) const override;
  SET_MEMORY_INFO_NAME(AsyncContextFrame)
  SET_SELF_SIZE(AsyncContextFrame)

 private:
  v8::Global<v8::Object> parent_;
  v8::Global<v8::Value> key_;
  v8::Global<v8::Value> value_;
  bool enabled_;
};

}  // namespace node

#endif  // defined(NODE_USE_NATIVE_ALS) && NODE_USE_NATIVE_ALS
#endif  // defined(NODE_WANT_INTERNALS) && NODE_WANT_INTERNALS

#endif  // SRC_ASYNC_CONTEXT_FRAME_H_
