#pragma once

#include <system_error>
#include <new>
#include <string>
#include <utility>
#include <type_traits>


namespace Om {

template<typename T>
class Result {
	static_assert(!std::is_same_v<std::remove_cv_t<std::remove_reference_t<T>>, std::error_code>,
		"Result type can not be std::error_code");

	union {
		T result;
		std::error_code error;
	};
	bool success;

public:
	Result(std::error_code errc) : success(false) {
		new(&error) std::error_code(std::move(errc));
	}

	template<typename... Args>
	Result(Args&&... args) :
		success(false) {

		new(&result) T(std::forward<Args>(args)...);
		success = true;
	}

	template<typename T>
	friend Result<T> make_error_result(std::error_code errc);

	template<typename T, typename... Args>
	friend Result<T> make_result(Args&&... args);


	Result(Result const&) = delete;
	Result& operator=(Result const&) = delete;

	// TODO: enable if std::is_nothrow_move_constructible
	Result(Result&& other) : success(other.success) {
		static_assert(std::is_nothrow_move_constructible_v<T>, "T must be nothrow move constructible.");
		if(success) {
			new(&result) T(std::move(other.result));
		} else {
			new(&error) std::error_code(std::move(other.error));
		}
	}

	Result& operator=(Result&& other) {
		destroy();
		success = false;
		if(other.success) {
			new(&result) T(std::move(other.result));
		} else {
			new(&error) std::error_code(std::move(other.error));
		}
	}

	~Result() {
		destroy();
	}

	explicit operator bool() const noexcept {
		return success;
	}

	T& get() noexcept {
		return result;
	}

	T const& get() const noexcept {
		return result;
	}

	std::error_code const& get_error() const noexcept {
		return error;
	}

private:
	void destroy() {
		if(success) {
			result.~T();
		} else {
			error.~error_code();
		}
	}
};

template<typename T>
Om::Result<T> make_error_result(std::error_code errcode) {
	return Om::Result<T>(errcode);
}

template<typename T, typename... Args>
Om::Result<T> make_result(Args&&... args) {
	return Om::Result<T>(std::forward<Args>(args)...);
}

}
