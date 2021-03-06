/*-
 * Copyright (c) 2012, 2013, 2015 Zhihao Yuan.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef ___FORMAT_BASE_H
#define ___FORMAT_BASE_H 1

#include <cstdlib>
#include <cassert>
#include <stdexcept>
#include <tuple>

#if defined(_MSC_VER)
#include <ciso646>
#endif

namespace stdex {

using std::get;

template <typename T>
using identity = std::common_type<T>;

template <typename T, typename CharT>
struct _accept_narrow : std::false_type {};

template <>
struct _accept_narrow<char, char> : std::false_type {};

template <typename T>
struct _accept_narrow<T, char> : std::true_type {
	typedef char		char_type;
	typedef int		int_type;
};

template <>
struct _accept_narrow<char, signed char> : std::true_type {
	typedef signed char	char_type;
	typedef int		int_type;
};

template <>
struct _accept_narrow<char, unsigned char> : std::true_type {
	typedef unsigned char	char_type;
	typedef int		int_type;
};

template <typename T, typename = void>
struct _param_type {
	using type = T const&;
};

template <typename T>
struct _param_type<T, std::enable_if_t<std::is_scalar<T>::value>> {
	using type = T;
};

template <typename T>
using _param_type_t = typename _param_type<T>::type;

template <typename CharT, typename... T>
struct _fmt_put {
	typedef CharT const*	iterator;

	_fmt_put(iterator it1, iterator it2, T const&... t) :
		iter_(it1, it2), item_(t...) {}

	iterator& begin() {
		return iter_.first;
	}

	iterator& end() {
		return iter_.second;
	}

	template <size_t _I, typename _CharT, typename... _T>
	friend auto _get(_fmt_put<_CharT, _T...> const& o)
		-> decltype(get<_I>(o.item_));

private:
	std::pair<iterator, iterator>	iter_;
	std::tuple<_param_type_t<T>...>	item_;
};

template <typename>
struct _tuple_size;

template <typename CharT, typename... T>
struct _tuple_size<_fmt_put<CharT, T...>> :
	std::integral_constant<int, sizeof...(T)> {};

template <size_t I, typename CharT, typename... T>
inline auto _get(_fmt_put<CharT, T...> const& o)
	-> decltype(get<I>(o.item_)) {
	return get<I>(o.item_);
}

template <int Low, int High, int Mid = (Low + High) / 2, typename = void>
struct _visit1_at;

template <int Low, int High, int Mid>
struct _visit1_at<Low, High, Mid, std::enable_if_t<(Low > High)>>
{
	template <typename... T>
	static decltype(auto) apply(int, T&&...) {}
};

template <int Mid>
struct _visit1_at<Mid, Mid, Mid>
{
	template <typename Tuple, typename F>
	static decltype(auto) apply(int n, F&& f, Tuple&& tp) {
		return std::forward<F>(f)(
		    _get<Mid - 1>(std::forward<Tuple>(tp)));
	}
};

template <int Low, int High, int Mid>
struct _visit1_at<Low, High, Mid, std::enable_if_t<(Low < High)>>
{
	template <typename... T>
	static decltype(auto) apply(int n, T&&... t) {
		if (n < Mid)
			return _visit1_at<Low, Mid - 1>::apply(n,
			    std::forward<T>(t)...);
		else if (n == Mid)
			return _visit1_at<Mid, Mid>::apply(n,
			    std::forward<T>(t)...);
		else
			return _visit1_at<Mid + 1, High>::apply(n,
			    std::forward<T>(t)...);
	}
};

template <typename Tuple, typename F>
inline decltype(auto) visit1_at(int n, F&& f, Tuple&& tp) {
	constexpr int m = _tuple_size<std::decay_t<Tuple>>::value;
	assert(0 < n and n <= m);
	return _visit1_at<1, m>::apply(n, std::forward<F>(f),
	    std::forward<Tuple>(tp));
}

template <size_t... I>
struct _indices {};

template <size_t N, size_t... I>
struct _build_indices : _build_indices<N - 1, N - 1, I...> {};
 
template <size_t... I>
struct _build_indices<0, I...> : _indices<I...> {};
 
template <typename Tuple>
using _tuple_indices = _build_indices<std::tuple_size<Tuple>::value>;

template <int Base, typename Int>
inline int _lexical_width(Int i) {
	if (i == 0)
		return 1;
	int n = 0;
	while (i != 0) {
		i /= Base;
		++n;
	}
	return n;
}

template <typename CharT, typename Facet>
inline char _to_narrow_digit(CharT ch, Facet const& fac) {
	char c = fac.narrow(ch, 0);
	if ('0' <= c and c <= '9')
		return c;
	else
		return 0;
}

}

#endif
