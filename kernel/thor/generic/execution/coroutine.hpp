#pragma once

#include <frigg/debug.hpp>
#include "basics.hpp"

template<typename T>
struct coroutine_continuation {
	virtual void set_done(T value) = 0;
};

// Specialization for coroutines without results.
template<>
struct coroutine_continuation<void> {
	virtual void set_done() = 0;
};

template<typename T, typename R>
struct coroutine_operation;

template<typename T>
struct coroutine;

// Specialization for coroutines without results.
template<>
struct coroutine<void> {
	template<typename T_, typename R>
	friend struct coroutine_operation;

	struct promise_type {
		template<typename T_, typename R>
		friend struct coroutine_operation;

		void *operator new(size_t size) {
			return thor::kernelAlloc->allocate(size);
		}

		void operator delete(void *p, size_t size) {
			return thor::kernelAlloc->deallocate(p, size);
		}

		coroutine get_return_object() {
			return {std::experimental::coroutine_handle<promise_type>::from_promise(*this)};
		}

		void unhandled_exception() {
			frigg::panicLogger() << "thor: Unhandled exception in coroutine<T>" << frigg::endLog;
		}

		void return_void() {
			// Do nothing.
		}

		auto initial_suspend() {
			struct awaiter {
				awaiter(promise_type *promise)
				: promise_{promise} { }

				bool await_ready() {
					return false;
				}

				void await_suspend(std::experimental::coroutine_handle<void>) {
					// Do nothing.
				}

				void await_resume() {
					assert(promise_->cont_);
				}

			private:
				promise_type *promise_;
			};
			return awaiter{this};
		}

		auto final_suspend() {
			struct awaiter {
				awaiter(promise_type *promise)
				: promise_{promise} { }

				bool await_ready() {
					return false;
				}

				void await_suspend(std::experimental::coroutine_handle<void>) {
					promise_->cont_->set_done();
				}

				void await_resume() {
					__builtin_trap();
				}

			private:
				promise_type *promise_;
			};
			return awaiter{this};
		}

	private:
		coroutine_continuation<void> *cont_ = nullptr;
	};

	coroutine()
	: h_{} { }

	coroutine(std::experimental::coroutine_handle<promise_type> h)
	: h_{h} { }

	coroutine(const coroutine &) = delete;

	coroutine(coroutine &&other)
	: coroutine{} {
		std::swap(h_, other.h_);
	}

	~coroutine() {
		if(h_)
			h_.destroy();
	}

	coroutine &operator= (coroutine other) {
		std::swap(h_, other.h_);
		return *this;
	}

private:
	std::experimental::coroutine_handle<promise_type> h_;
};

template<typename T, typename R>
struct coroutine_operation;

// Specialization for coroutines without results.
template<typename R>
struct coroutine_operation<void, R> : private coroutine_continuation<void> {
	coroutine_operation(coroutine<void> s, R receiver)
	: s_{std::move(s)}, receiver_{std::move(receiver)} { }

	coroutine_operation(const coroutine_operation &) = delete;

	coroutine_operation &operator= (const coroutine_operation &) = delete;

	void start() {
		auto promise = &s_.h_.promise();
		promise->cont_ = this;
		s_.h_.resume();
	}

private:
	void set_done() override {
		receiver_.set_done();
	}

private:
	coroutine<void> s_;
	R receiver_;
};

template<typename T, typename R>
coroutine_operation<T, R> connect(coroutine<T> s, R receiver) {
	return {std::move(s), std::move(receiver)};
};

template<typename T>
execution::sender_awaiter<coroutine<T>, T> operator co_await(coroutine<T> s) {
	return {std::move(s)};
}