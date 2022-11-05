// Code generator.
// @PENGUINLIONG
#pragma once
#include "ticpp/parse_context.hpp"

namespace ticpp {

extern std::string composite_python_script(
  TiArch arch,
  const ParseResult& itm
);



template<typename TFunc, typename ... TArgs>
std::string run_codegen(TiArch arch, TFunc& fn, TArgs ... args) {
  PARSE_CONTEXT.start();
  fn(expr_conv_t<TArgs>::to_expr(args) ...);
  ParseResult itm = PARSE_CONTEXT.stop();

  std::string out = composite_python_script(arch, itm);
  return out;
}


template<typename TFunc>
struct Kernel {};
template<typename ... TExprs>
struct Kernel<std::function<void(TExprs ...)>> {
  TiArch arch_;
  std::function<void(TExprs ...)> fn_;

  Kernel(TiArch arch, std::function<void(TExprs ...)> fn) :
    arch_(arch), fn_(std::move(fn)) {}

  template<typename ... TArgs>
  std::string compile(const TArgs& ... args) {
    static_assert(sizeof...(TArgs) == sizeof...(TExprs), "");
    return run_codegen(arch_, fn_, args ...);
  }
};



template<typename TFunc>
struct get_func_ty {};
template<typename ... TArgs>
struct get_func_ty<void(TArgs ...)> {
  typedef std::function<void(TArgs ...)> type;
};
template<typename ... TArgs>
struct get_func_ty<void(*)(TArgs ...)> {
  typedef std::function<void(TArgs ...)> type;
};


template<typename TFunc>
auto to_kernel(TFunc f) {
  typename get_func_ty<TFunc>::type func { f };
  return Kernel<typename get_func_ty<TFunc>::type>(TI_ARCH_VULKAN, std::move(func));
}



} // namespace ticpp
