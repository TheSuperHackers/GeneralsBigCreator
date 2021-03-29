#pragma once

#include "platform.h"

#ifdef _RELEASE
#define CHECK_REFCOUNT_CRASH(x)
#else
#define CHECK_REFCOUNT_CRASH(x) { if (!(x)) *((int*)0) = 0; }
#endif

template <class _I> class _smart_ptr
{
private:
	_I* p;
public:
	_smart_ptr() : p(NULL) {}
	_smart_ptr(_I* p_)
	{
		p = p_;
		if (p)
			p->AddRef();
	}

	_smart_ptr(const _smart_ptr& p_)
	{
		p = p_.p;
		if (p)
			p->AddRef();
	}

	template <typename _Y>
	_smart_ptr(const _smart_ptr<_Y>& p_)
	{
		p = p_.get();
		if (p)
			p->AddRef();
	}

	~_smart_ptr()
	{
		if (p)
			p->Release();
	}
	operator _I*() const { return p; }

	_I& operator*() const
	{
		return *p;
	}

	_I* operator->() const
	{
		return p;
	}

	_I* get() const
	{
		return p;
	}

	_smart_ptr& operator=(_I* newp)
	{
		if (newp)
			newp->AddRef();
		if (p)
			p->Release();
		p = newp;
		return *this;
	}

	void reset()
	{
		_smart_ptr<_I>().swap(*this);
	}

	void reset(_I* p)
	{
		_smart_ptr<_I>(p).swap(*this);
	}

	_smart_ptr& operator=(const _smart_ptr& newp)
	{
		if (newp.p)
			newp.p->AddRef();
		if (p)
			p->Release();
		p = newp.p;
		return *this;
	}

	template <typename _Y>
	_smart_ptr& operator=(const _smart_ptr<_Y>& newp)
	{
		_I* const p2 = newp.get();
		if (p2)
			p2->AddRef();
		if (p)
			p->Release();
		p = p2;
		return *this;
	}

	void swap(_smart_ptr<_I>& other)
	{
		std::swap(p, other.p);
	}

	void Assign_NoAddRef(_I* ptr)
	{
		p = ptr;
	}

	_I* ReleaseOwnership()
	{
		_I* ret = p;
		p = 0;
		return ret;
	}
};

template <typename T>
inline void swap(_smart_ptr<T>& a, _smart_ptr<T>& b)
{
	a.swap(b);
}


template <typename TDerived, typename Counter = int> class _reference_target_no_vtable
{
public:
	_reference_target_no_vtable() :
		m_nRefCounter(0)
	{
	}

	~_reference_target_no_vtable()
	{
	}

	void AddRef()
	{
		CHECK_REFCOUNT_CRASH(m_nRefCounter >= 0);
		++m_nRefCounter;
	}

	void Release()
	{
		CHECK_REFCOUNT_CRASH(m_nRefCounter > 0);
		if (--m_nRefCounter == 0)
		{
			delete static_cast<TDerived*>(this);
		}
		else if (m_nRefCounter < 0)
		{
			assert(0);
		}
	}

	Counter NumRefs()
	{
		return m_nRefCounter;
	}
protected:
	Counter m_nRefCounter;
};


template <typename Counter = int>
class _reference_target
{
public:
	_reference_target() :
		m_nRefCounter(0)
	{
	}

	virtual ~_reference_target()
	{
	}

	void AddRef()
	{
		CHECK_REFCOUNT_CRASH(m_nRefCounter >= 0);
		++m_nRefCounter;
	}

	void Release()
	{
		CHECK_REFCOUNT_CRASH(m_nRefCounter > 0);
		if (--m_nRefCounter == 0)
		{
			delete this;
		}
		else if (m_nRefCounter < 0)
		{
			assert(0);
		}
	}

	Counter NumRefs()
	{
		return m_nRefCounter;
	}
protected:
	Counter m_nRefCounter;
};

typedef _reference_target<int> _reference_target_t;
