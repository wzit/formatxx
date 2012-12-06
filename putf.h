/*-
 * Copyright (c) 2012, 2013 Zhihao Yuan.  All rights reserved.
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

#ifndef _PUTF_H
#define _PUTF_H 1

#include "__format_base.h"
#include <algorithm>
#include <cassert>
#include <ios>
#include <locale>
#include <sstream>

namespace stdex {

template <typename Iter, typename... T>
using _fmt_put = _format<Iter, T...>;

template <typename CharT, size_t N, typename... T>
inline auto putf(CharT const (&fmt)[N], T const&... t)
	-> _fmt_put<CharT const *, T...> {
	return _fmt_put<CharT const *, T...>(fmt, fmt + N - 1, t...);
}

template <typename CharT, typename Traits, typename... T>
inline auto putf(std::basic_string<CharT, Traits> const& fmt, T const&... t)
	-> _fmt_put<decltype(begin(fmt)), T...> {
	return _fmt_put<decltype(begin(fmt)), T...>(begin(fmt), end(fmt), t...);
}

template <typename CharT, size_t N, typename... T>
inline auto sputf(CharT const (&fmt)[N], T const&... t)
	-> std::basic_string<CharT> {
	std::basic_ostringstream<CharT> out;
	out << _fmt_put<CharT const *, T...>(fmt, fmt + N - 1, t...);
	return out.str();
}

template <typename CharT, typename Traits, typename... T>
inline auto sputf(std::basic_string<CharT, Traits> const& fmt, T const&... t)
	-> std::basic_string<CharT, Traits> {
	std::basic_ostringstream<CharT, Traits> out;
	out << _fmt_put<decltype(begin(fmt)), T...>(begin(fmt), end(fmt), t...);
	return out.str();
}

template <typename T, typename Enable = void>
struct _make_unsigned_fallback {
	typedef T type;
};

template <typename T>
struct _make_unsigned_fallback<T,
	typename std::enable_if<std::is_integral<T>::value>::type> {
	typedef typename std::make_unsigned<T>::type type;
};

template <typename T>
inline auto _to_unsigned(T t)
	-> typename _make_unsigned_fallback<T>::type {
	return static_cast<typename _make_unsigned_fallback<T>::type>(t);
}

template <typename Traits, typename T, typename Enable = void>
struct _make_char_fallback {
	typedef T type;
};

template <typename Traits, typename T>
struct _make_char_fallback<Traits, T,
	typename std::enable_if<std::is_same<T,
	typename Traits::int_type>::value>::type> {
	typedef typename Traits::char_type type;
};

template <typename Traits, typename T>
inline auto _to_char(T t)
	-> typename _make_char_fallback<Traits, T>::type {
	return static_cast<typename _make_char_fallback<Traits, T>::type>(t);
}

template <typename Traits, typename T, typename Enable = void>
struct _make_int_fallback {
	typedef T type;
};

template <typename Traits, typename T>
struct _make_int_fallback<Traits, T,
	typename std::enable_if<std::is_same<T,
	typename Traits::char_type>::value>::type> {
	typedef typename Traits::int_type type;
};

template <typename Traits, typename T>
inline auto _to_int(T t)
	-> typename _make_int_fallback<Traits, T>::type {
	return static_cast<typename _make_int_fallback<Traits, T>::type>(t);
}

template <typename Iter, typename Facet>
int _parse_int(Iter& b, Iter& e, Facet const& fac) {
	int n = 0;
	for (; b != e and fac.is(std::ctype_base::digit, *b); ++b) {
		n *= 10;
		n += fac.narrow(*b, 0) - '0';
	}
	return n;
}

template <typename Stream>
inline auto _flags_for_output(Stream const& out) -> decltype(out.flags()) {
	using os = Stream;
	return out.flags() & os::boolalpha & os::unitbuf;
}

template <typename CharT>
struct _padding {
	template <typename Stream>
	explicit _padding(Stream const& s) :
		precision_(s.precision()), fill_(s.fill()) {}

	std::streamsize	precision_;
	CharT		fill_;
};

template <typename Stream>
struct _padding_guard {
	_padding_guard(Stream& s, _padding<decltype(s.fill())> pad) :
		stream_(s), pad_(s) {
		stream_.precision(pad.precision_);
		stream_.fill(pad.fill_);
	}
	~_padding_guard() {
		stream_.fill(pad_.fill_);
		stream_.precision(pad_.precision_);
	}

	_padding_guard(_padding_guard const&) = delete;
	_padding_guard& operator=(_padding_guard const&) = delete;

private:
	Stream&					stream_;
	_padding<decltype(stream_.fill())>	pad_;
};

template <typename Stream, typename T>
class _outputter;

template <typename Stream, typename T>
_outputter<Stream, T> _output(Stream& out, T const& t) {
	return _outputter<Stream, T>(out, t);
}

template <typename Stream, typename T>
class _outputter {
	typedef typename std::decay<T>::type Tp_;
	typedef typename std::conditional<std::is_pointer<Tp_>::value,
			typename std::add_pointer<
			typename std::remove_cv<
			typename std::remove_pointer<Tp_>::type>::type>::type,
		Tp_>::type RealT;

	Stream& out_;
	T const& t_;

public:
	typedef decltype(out_.flags())		FlagT;
	typedef _padding<decltype(out_.fill())>	PadT;

	_outputter(Stream& out, T const& t) : out_(out), t_(t) {}

	Stream& with(FlagT fl, PadT pad) {
		return _with(fl, pad, identity<RealT>());
	}

	Stream& with_aligned_sign(FlagT fl, PadT pad) {
		using os = std::basic_ostringstream<
			decltype(out_.fill()),
			typename Stream::traits_type>;

		os dummy_out;
		fl |= os::showpos;

		// simulate `out_'
		dummy_out.width(out_.width(0));
		_output(dummy_out, t_).with(fl, pad);
		auto s = dummy_out.str();
		auto i = s.find(out_.widen('+'));
		if (i != decltype(s)::npos)
			s[i] = dummy_out.fill();
		out_.write(s.data(), s.size());
		return out_;
	}

private:
	template <typename _T>
	Stream& _with(FlagT fl, PadT pad, identity<_T>) {
		return _output__(fl, pad, t_);
	}

	template <typename _CharT, typename _Traits>
	Stream& _with(FlagT fl, PadT pad,
	    identity<std::basic_string<_CharT, _Traits>>) {
		return _output__(fl, pad, pad.precision_ < t_.size() ?
		    t_.substr(0, pad.precision_) : t_);
	}

	Stream& _with(FlagT fl, PadT pad, identity<char *>) {
		return _output_chars__(fl, pad, t_);
	}

	Stream& _with(FlagT fl, PadT pad, identity<wchar_t *>) {
		return _output_chars__(fl, pad, t_);
	}

	Stream& _with(FlagT fl, PadT pad, identity<char16_t *>) {
		return _output_chars__(fl, pad, t_);
	}

	Stream& _with(FlagT fl, PadT pad, identity<char32_t *>) {
		return _output_chars__(fl, pad, t_);
	}

	template <typename _CharT>
	Stream& _output_chars__(FlagT fl, PadT pad, _CharT const *t) {
		size_t n = 0;
		auto i = t;
		for (; *i and n < pad.precision_; ++i)
			++n;
		if (*i != 0)
			return _output__(fl, pad,
			    std::basic_string<_CharT>(t, n));
		return _output__(fl, pad, t);
	}

	template <typename _T>
	Stream& _output__(FlagT fl, PadT pad, _T const& t) {
		_padding_guard<Stream> _(out_, pad);
		out_.flags(fl);
		out_ << t;
		return out_;
	}
};

template <size_t I, size_t N>
struct _put_fmt;

template <size_t N>
struct _put_fmt<N, N> {
	template <typename CharT, typename Traits, typename Iter, typename... T>
	static auto apply(std::basic_ostream<CharT, Traits>& out,
	    _fmt_put<Iter, T...>& t) -> decltype(out)
	{
		using std::begin; using std::end;

		auto i = std::find(begin(t), end(t), out.widen('%'));
		if (i == end(t))
			return out.write(&*begin(t), i - begin(t));
		out.write(&*begin(t), i - begin(t));
		auto& b = begin(t);
		b = ++i;
		switch (out.narrow(*b, 0)) {
		case '%':
			++b;
			return _put_fmt<N, N>::apply(out.put(
				    out.widen('%')), t);
		default:
			throw std::invalid_argument("too few arguments");
		}
	}
};

template <size_t I, size_t N>
struct _put_fmt {
	template <typename CharT, typename Traits, typename Iter, typename... T>
	static auto apply(std::basic_ostream<CharT, Traits>& out,
	    _fmt_put<Iter, T...>& t) -> decltype(out)
	{
		using std::begin; using std::end;
		using os = std::basic_ostream<CharT, Traits>;

		auto i = std::find(begin(t), end(t), out.widen('%'));
		if (i == end(t))
			throw std::invalid_argument("too many arguments");
		out.write(&*begin(t), i - begin(t));
		auto& b = begin(t);
		b = ++i;

		auto fl = _flags_for_output(out);
		_padding<CharT> pad(out);
		enum class spec {
			none,
			raw,
			to_unsigned,
			to_char,
			to_int,
		} sp = spec::none;
		bool align_sign = false;

		parse_flags:
		switch (out.narrow(*b, 0)) {
		case '-':
			fl |= os::left;
			break;
		case '+':
			fl |= os::showpos;
			break;
		case ' ':
			if (!(fl & os::showpos))
				align_sign = true;
			break;
		case '#':
			fl |= os::showbase | os::showpoint;
			break;
		case '0':
			if (!(fl & os::left)) {
				fl |= os::internal;
				pad.fill_ = out.widen('0');
			}
			break;
		default:
			goto parse_width;
		}
		if (++b != end(t))
			goto parse_flags;

		parse_width:
		auto& fac = std::use_facet<std::ctype<CharT>>(out.getloc());
		if (isdigit(*b, out.getloc()))
			out.width(_parse_int(b, end(t), fac));

		// precision defaults to zero with a single '.'
		bool no_precision = true;
		if (*b == out.widen('.')) {
			++b;
			pad.precision_ = _parse_int(b, end(t), fac);
			no_precision = false;
		}

		// ignore all length modifiers
		switch (auto c = out.narrow(*b, 0)) {
		case 'h': case 'l':
			c = out.narrow(*++b, 0);
			if (c == 'h' or c == 'l')
				++b;
			break;
		case 'j': case 'z': case 't': case 'L':
			++b;
			break;
		}

		// type-safe conversions are considered
		typedef typename std::remove_reference<
			decltype(get<I>(t))>::type TypeI;

		switch (out.narrow(*b, 0)) {
		case 'p':
			assert(std::is_pointer<TypeI>::value);
		case 'X':
			fl |= os::uppercase;
		case 'x':
			fl |= os::hex;
			sp = spec::to_unsigned;
			break;
		case 'o':
			fl |= os::oct;
			sp = spec::to_unsigned;
			break;
		case 'E':
			fl |= os::uppercase;
		case 'e':
			fl |= os::scientific;
			break;
		case 'F':
			fl |= os::uppercase;
		case 'f':
			fl |= os::fixed;
			break;
		case 'G':
			fl |= os::uppercase;
		case 'g':	/* floatfield == 0 */
			break;
		case 'A':
			fl |= os::uppercase;
		case 'a':
			fl |= os::fixed | os::scientific;
			break;
		case 'u':
			assert(std::is_integral<TypeI>::value);
			sp = spec::to_unsigned;
		case 'd':
			fl |= os::dec;
			sp = spec::to_int;
			break;
		case 'i':	/* basefield == 0 */
			sp = spec::to_int;
			break;
		case 's': case 'S':
			if (no_precision)
				pad.precision_ = -1;
			break;
		case 'c': case 'C':
			sp = spec::to_char;
			break;
		case '%':
			sp = spec::raw;
			if (b == i) break;
		default:
			throw std::invalid_argument("bad format string ");
		}
		++b;

		switch (sp) {
		case spec::raw:
			return _put_fmt<I, N>::apply(out.put(
				    out.widen('%')), t);
		case spec::none: {
			auto v = _output(out, get<I>(t));
			return _put_fmt<I + 1, N>::apply(align_sign ?
			    v.with_aligned_sign(fl, pad) : v.with(fl, pad), t);
		}
		case spec::to_int: {
			auto v = _output(out, _to_int<Traits>(get<I>(t)));
			return _put_fmt<I + 1, N>::apply(align_sign ?
			    v.with_aligned_sign(fl, pad) : v.with(fl, pad), t);
		}
		case spec::to_unsigned:
			return _put_fmt<I + 1, N>::apply(_output(out,
				    _to_unsigned(get<I>(t))).with(fl, pad), t);
		case spec::to_char:
			return _put_fmt<I + 1, N>::apply(_output(out,
				    _to_char<Traits>(get<I>(t))).with(
				    fl, pad), t);
		}
		abort(); /* shut up gcc */
	}
};

template <typename CharT, typename Traits, typename... T>
inline auto operator<<(std::basic_ostream<CharT, Traits>& out,
    _fmt_put<CharT const *, T...> t) -> decltype(out)
{
	_unformatted_guard<decltype(out)> _(out);
	return _put_fmt<0, sizeof...(T)>::apply(out, t);
}

template <typename CharT, typename Traits, typename... T>
inline auto operator<<(std::basic_ostream<CharT, Traits>& out,
    _fmt_put<decltype(begin(std::basic_string<CharT, Traits>())), T...> t)
	-> decltype(out)
{
	_unformatted_guard<decltype(out)> _(out);
	return _put_fmt<0, sizeof...(T)>::apply(out, t);
}

}

#endif