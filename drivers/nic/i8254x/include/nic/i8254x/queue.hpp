#pragma once

#include <stddef.h>

struct QueueIndex {
	QueueIndex(size_t value, size_t mod) : _index(value), _mod(mod) { }

	operator size_t() {
		return _index;
	}

	size_t operator()() {
		return _index;
	}

	QueueIndex operator+(int v) const {
		auto tmp{*this};
		tmp._index = (tmp._index + (v % _mod)) % _mod;
		return tmp;
	}

	QueueIndex& operator++() {
		_index = (_index + 1) % _mod;
		return *this;
	}

	QueueIndex& operator--() {
		_index = (_mod + _index - 1) % _mod;
		return *this;
	}

	bool operator==(const QueueIndex &other) const {
		return _index == other._index;
	}

private:
	size_t _index;
	size_t _mod;
};

