#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <assert.h>
#include <stdint.h>
#include <type_traits>

static inline void overWriteMem(uintptr_t address, void* newFn)
{
	SIZE_T bytesWritten = 0;
	BOOL res = WriteProcessMemory(GetCurrentProcess(),
	    (void*)address, &newFn, sizeof(newFn), &bytesWritten);
	assert(res);
	assert(bytesWritten == sizeof(newFn));
}

static inline void overWriteMemDirectly(uintptr_t address, void* newFn)
{
	*(uintptr_t*)address = (uintptr_t)newFn;
}

static inline void replaceFn(uintptr_t oldFn, void* newFn)
{
#if _M_X64
	// 48 b8 35 08 40 00 00 00 00 00   mov rax, 0x0000000000400835
	// ff e0                           jmp rax
	unsigned char codeBytes[12] = { 0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   0xff, 0xe0 };
	memcpy(&codeBytes[2], &newFn, sizeof(void*));
#elif _M_IX86
	// E9 00000000   jmp rel  displacement relative to next instruction
	unsigned char codeBytes[5] = { 0xE9, 0x00, 0x00, 0x00, 0x00 };
	uintptr_t p = reinterpret_cast<uintptr_t>(newFn) - oldFn - sizeof(codeBytes);
	memcpy(&codeBytes[1], &p, sizeof(p));
#else
#error "The following code only works for x86 and x64!"
#endif

	SIZE_T bytesWritten = 0;
	BOOL res = WriteProcessMemory(GetCurrentProcess(),
		(void*)oldFn, codeBytes, sizeof(codeBytes), &bytesWritten);
	assert(res);
	assert(bytesWritten == sizeof(codeBytes));
}

// relies on the target section being writable
static inline void replaceFnDirectly(uintptr_t oldFn, void* newFn)
{
#if _M_X64
	// 48 b8 35 08 40 00 00 00 00 00   mov rax, 0x0000000000400835
	// ff e0                           jmp rax
	*(uint16_t*)oldFn = 0xB848;
	*(uintptr_t*)(oldFn + 2) = (uintptr_t)newFn;
	*(uint16_t*)(oldFn + 10) = 0xe0ff;
#elif _M_IX86
	// E9 00000000   jmp rel  displacement relative to next instruction
	*(uint8_t*)oldFn = 0xE9;
	*(uintptr_t*)(oldFn + 1) = (uintptr_t)newFn - oldFn - 5;
#else
#error "The following code only works for x86 and x64!"
#endif
}

//! helper for getting address of member function
#if __GNUC__
#pragma GCC diagnostic ignored "-Wpmf-conversions"
#define memFn2ptr(t) ((void*)t)
#elif _MSC_VER
template <typename T>
void* memFn2ptr(T t)
{
	static_assert(std::is_member_function_pointer_v<T> || std::is_function_v<std::remove_pointer_t<T>>, "");
	static_assert(sizeof(T) == sizeof(void*), "");
	return (void*&)t; // non-standard
	// does not work on virtual methods cause of vcall thunks
	// https://godbolt.org/z/bTMhsfsbj
}
#endif

struct FunctionReplacement
{
	uintptr_t original;
	void* replacement;
};

template <typename T>
struct MemoryReplacement
{
	uintptr_t original = 0;
	T replacement = nullptr;
};
template <typename T>
MemoryReplacement(uintptr_t, T) -> MemoryReplacement<T>;

extern FunctionReplacement g_replacements[];
extern size_t g_replacementsIdx;
extern MemoryReplacement<void*> g_memReplacements[];
extern size_t g_memReplacementsIdx;
#define CONCAT_(x, y) x##y
#define CONCAT(x, y) CONCAT_(x, y)
//#define HOOK_FUNC(old, new) const char CONCAT(dummy, __COUNTER__) = (replaceFn(old, &new), 0)
//#define HOOK_METH(old, new) const char CONCAT(dummy, __COUNTER__) = (replaceFn(old, memFn2ptr(&new)), 0)

//#define HOOK_FUNC(old) static const char CONCAT(dummy, __COUNTER__) = (replaceFnDirectly(old, &__FUNCTION__), 0)
//#define HOOK_METHOD(old, new) static const char CONCAT(dummy, __COUNTER__) = (replaceFnDirectly(old, memFn2ptr(&std::remove_pointer_t<decltype(this)>::##new)), 0)

#define HOOK(new, old) static const char CONCAT(dummy, __COUNTER__) = (g_replacements[g_replacementsIdx++] = {old, memFn2ptr(&new)}, 0)

#if 1
#define OVERWRITE(new, old) static const char CONCAT(dummy, __COUNTER__) = (g_memReplacements[g_memReplacementsIdx++] = {old, memFn2ptr(&new)}, 0)
#else
#pragma section("memrepl$b", read)
#define OVERWRITE(new, old) __declspec(allocate("memrepl$b")) \
extern constexpr MemoryReplacement CONCAT(g_memReplacement, __COUNTER__) = {old, &new}
#endif
