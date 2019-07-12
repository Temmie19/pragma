#ifndef __LUA_HANDLES_HPP__
#define __LUA_HANDLES_HPP__

namespace util
{
	template<typename T>
		class TWeakSharedHandle;
	template<typename T>
		class TSharedHandle;
	template<typename T>
		class WeakHandle;
};
namespace luabind
{
	template<typename T>
		T* get_pointer(const util::TWeakSharedHandle<T>& pointer)
	{
		return const_cast<T*>(pointer.Get());
	}
	template<typename T>
		T* get_pointer(const util::TSharedHandle<T>& pointer)
	{
		return const_cast<T*>(pointer.Get());
	}
	template<typename T>
		T* get_pointer(const util::WeakHandle<T>& pointer)
	{
		return const_cast<T*>(pointer.get());
	}
};
namespace luabind::detail::has_get_pointer_
{
	template<typename T>
		T* get_pointer(const util::TWeakSharedHandle<T>& pointer)
	{
		return const_cast<T*>(pointer.Get());
	}
	template<typename T>
		T* get_pointer(const util::TSharedHandle<T>& pointer)
	{
		return const_cast<T*>(pointer.Get());
	}
	template<typename T>
		T* get_pointer(const util::WeakHandle<T>& pointer)
	{
		return const_cast<T*>(pointer.get());
	}
}

#endif
