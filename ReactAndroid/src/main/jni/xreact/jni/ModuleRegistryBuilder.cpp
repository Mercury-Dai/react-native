// Copyright 2004-present Facebook. All Rights Reserved.

#include "ModuleRegistryBuilder.h"

#include <cxxreact/CxxNativeModule.h>
#include <folly/Memory.h>

namespace facebook {
namespace react {

std::string ModuleHolder::getName() const {
  static auto method = getClass()->getMethod<jstring()>("getName");
  return method(self())->toStdString();
}

xplat::module::CxxModule::Provider ModuleHolder::getProvider() const {
  return [self=jni::make_global(self())] {
    static auto method =
      ModuleHolder::javaClassStatic()->getMethod<JNativeModule::javaobject()>(
        "getModule");
    // This is the call which uses the lazy Java Provider to instantiate the
    // Java CxxModuleWrapper which contains the CxxModule.
    auto module = method(self);
    CHECK(module->isInstanceOf(CxxModuleWrapper::javaClassStatic()))
      << "module isn't a C++ module";
    auto cxxModule = jni::static_ref_cast<CxxModuleWrapper::javaobject>(module);
    // Then, we grab the CxxModule from the wrapper, which is no longer needed.
    return cxxModule->cthis()->getModule();
  };
}

std::unique_ptr<ModuleRegistry> buildModuleRegistry(
    std::weak_ptr<Instance> winstance,
    jni::alias_ref<jni::JCollection<JavaModuleWrapper::javaobject>::javaobject> javaModules,
    jni::alias_ref<jni::JCollection<ModuleHolder::javaobject>::javaobject> cxxModules) {
  std::vector<std::unique_ptr<NativeModule>> modules;
  for (const auto& jm : *javaModules) {
    modules.emplace_back(folly::make_unique<JavaNativeModule>(winstance, jm));
  }
  for (const auto& cm : *cxxModules) {
    modules.emplace_back(
      folly::make_unique<CxxNativeModule>(winstance, cm->getName(), cm->getProvider()));
  }
  if (modules.empty()) {
    return nullptr;
  } else {
    return folly::make_unique<ModuleRegistry>(std::move(modules));
  }
}

}}
