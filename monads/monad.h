#pragma once

#include "functor.h"
#include <type_traits>

/*
* NOTE:
* std::result_of_t<F(A)> is deprecated
* use
* std::invoke_result_t<F, A> instead
*/

namespace Monad {
	/*
	* Haskell definition
	* class (Functor m) => Monad m where
	*	pure :: a -> m a
	*	bind :: m a -> (a -> m b) -> m b
	*/
	template <template <typename> class M, typename Enable = void>
	struct Monad;

	namespace detail {
		// the default case is that any given class template is not a Monad
		template <template <typename> class M, typename Enable = void>
		struct IsMonadT : std::false_type {};

		// the general case
		template <template <typename> class M>
		struct IsMonadT <M, std::void_t<Monad<M>, std::enable_if_t<Functor::IsFunctor<M>>>>
			: std::true_type {};

		// pure :: (Monad m) => a -> m a
		template <template <typename> class M,
				  typename A,
				  typename = std::enable_if_t<IsMonad<M>>>
		M<A> pure(const A& x) {
			return Monad<M>::pure(x);
		}

		// bind :: (Monad m) => m a -> (a -> m b) -> m b
		template <typename F,
				  template <typename> class M,
				  typename A,
				  typename = std::enable_if_t<IsMonad<M>>>
		std::invoke_result_t<F, A> bind(const M<A>& m, F&& f) {
			return Monad<M>::bind(m, std::forward<F>(f));
		}
	}

	template <template <typename> class M>
	constexpr bool IsMonad = detail::IsMonadT<M>::value;

	// In Haskell the infix (>>=) operator is used as a synonym for bind
	// It allows us to write m >>= [](){} instead of bind(m, [](){})
	template <template <typename> class M,
			  typename A,
			  typename F,
		      typename = std::enable_if_t<IsMonad<M>>>
	auto operator>>=(const M<A>& m, F&& f) {
		return detail::bind(m, std::forward<F>(f));
	}

	// The infix (>>) operator is also used in Haskell.
	// It just throws away the result of evaluating the first argument
	// and returns the second argument instead.
	template <template <typename> class M,
			  typename A,
			  typename B,
			  typename = std::enable_if_t<IsMonad<M>>>
	auto operator>>=(const M<A>& m, const M<B>& v) {
		return detail::bind(m, [=](auto&&) { return v; });
	}
}

