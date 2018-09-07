#pragma once

template <class T> class MayBe {
private:
	bool initialized;
	T value;
public:
	MayBe() : initialized(false), value() {};
	MayBe(const T val) : initialized(true), value(val) {};
	~MayBe() { };
	MayBe(const MayBe& x) : initialized(x.initialized), value(x.value) {};
	operator T() const { return value; };
	operator bool() const { return initialized; };
	T& get() { return value; };
	T const* operator->() const { return &value; };
	void operator= (const MayBe& x) { initialized = x.initialized; value = x.value; }
};
