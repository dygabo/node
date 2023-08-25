#include "async_context_frame.h"  // NOLINT(build/include_inline)

#include "env-inl.h"
#include "node_errors.h"
#include "node_external_reference.h"
#include "tracing/traced_value.h"
#include "util-inl.h"

#include "debug_utils-inl.h"

#include "v8.h"

using v8::Context;
using v8::Isolate;
using v8::Local;
using v8::Object;
using v8::String;
using v8::Value;

#if defined(NODE_USE_NATIVE_ALS) && NODE_USE_NATIVE_ALS

namespace node {

//
// Scope helper
//
AsyncContextFrame::Scope::Scope(Isolate* isolate, Local<Value> object)
    : isolate_(isolate) {
  auto prior = AsyncContextFrame::current(isolate);

  isolate_->GetEnteredOrMicrotaskContext()
      ->SetContinuationPreservedEmbedderData(object);

  prior_.Reset(isolate, prior);
}

AsyncContextFrame::Scope::~Scope() {
  auto value = prior_.Get(isolate_);
  isolate_->GetEnteredOrMicrotaskContext()
      ->SetContinuationPreservedEmbedderData(value);
}

Local<Value> AsyncContextFrame::current(Isolate* isolate) {
  return isolate->GetEnteredOrMicrotaskContext()
      ->GetContinuationPreservedEmbedderData();
}

// NOTE: It's generally recommended to use AsyncContextFrame::Scope
// but sometimes (such as enterWith) a direct exchange is needed.
Local<Value> AsyncContextFrame::exchange(Isolate* isolate, Local<Value> value) {
  auto prior = current(isolate);
  isolate->GetEnteredOrMicrotaskContext()->SetContinuationPreservedEmbedderData(
      value);
  return prior;
}

namespace async_context_frame {

void CreatePerContextProperties(Local<Object> target,
                                Local<Value> unused,
                                Local<Context> context,
                                void* priv) {
  Environment* env = Environment::GetCurrent(context);

  Local<String> getContinuationPreservedEmbedderData = FIXED_ONE_BYTE_STRING(
      env->isolate(), "GetContinuationPreservedEmbedderData");
  Local<String> setContinuationPreservedEmbedderData = FIXED_ONE_BYTE_STRING(
      env->isolate(), "SetContinuationPreservedEmbedderData");

  // Grab the intrinsics from the binding object and expose those to our
  // binding layer.
  Local<Object> binding = context->GetExtrasBindingObject();
  target
      ->Set(context,
            getContinuationPreservedEmbedderData,
            binding->Get(context, getContinuationPreservedEmbedderData)
                .ToLocalChecked())
      .Check();
  target
      ->Set(context,
            setContinuationPreservedEmbedderData,
            binding->Get(context, setContinuationPreservedEmbedderData)
                .ToLocalChecked())
      .Check();
}

}  // namespace async_context_frame

}  // namespace node

NODE_BINDING_CONTEXT_AWARE_INTERNAL(
    async_context_frame, node::async_context_frame::CreatePerContextProperties)

#else
namespace node {
void EmptyContextProperties(Local<Object> target,
                            Local<Value> unused,
                            Local<Context> context,
                            void* priv) {}
}  // namespace node

NODE_BINDING_CONTEXT_AWARE_INTERNAL(async_context_frame,
                                    node::EmptyContextProperties)
#endif  // defined(NODE_USE_NATIVE_ALS) && NODE_USE_NATIVE_ALS
