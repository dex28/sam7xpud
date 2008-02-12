
//---------------------------------------------------------------------------------------
//      Includes
//---------------------------------------------------------------------------------------

#include "trace.h"

//---------------------------------------------------------------------------------------
// C++ Support Functions (instead of using libstdc++) according to ABI 
// (Application Binary Interface) for C++ programs that is the object code interface 
// between user C++ code and the implementation-provided system and libraries.
//---------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------
// Dynamic Shared Library (DSO) Handle. No shared libraries thus defined as dummy.
//---------------------------------------------------------------------------------------
extern "C" const void* const __dso_handle = &__dso_handle;

//---------------------------------------------------------------------------------------
// After constructing a global (or local static) object, that will require destruction on
// exit, a termination function is registered with __cxa_atexit.
// This registration, e.g. __cxa_atexit(f,p,d), is intended to cause the call f(p) 
// when DSO d is unloaded, before all such termination calls registered before this one.
// It returns zero if registration is successful, nonzero on failure. 
// The registration function is not called from within the constructor. 
//---------------------------------------------------------------------------------------
extern "C" int __cxa_atexit( void (*func)(void*), void* arg, void* dso_handle)
{
    TRACE_INFO( "__cxa_atexit: Foo=%08x, Arg=%08x\n", 
                (unsigned int) func, (unsigned int) arg );
    return 0;
    }

//---------------------------------------------------------------------------------------
// Returns 1 if the initialization is not yet complete; 0 otherwise. This function is
// called before initialization takes place. If this function returns 1, either 
// __cxa_guard_release or __cxa_guard_abort must be called with the same argument. 
// The first byte of the guard_object is not modified by this function. 
// A thread-safe implementation will probably guard access to the first byte of the 
// guard_object with a mutex. If this function returns 1, the mutex will have been 
// acquired by the calling thread. 
//---------------------------------------------------------------------------------------
extern "C" int __cxa_guard_acquire( long long int* guard_object )
{
    TRACE_INFO( "__cxa_guard_acquire: Obj=%08x\n", (unsigned int) guard_object );
    return 1;
    }

//---------------------------------------------------------------------------------------
// Sets the first byte of the guard object to a non-zero value. This function is called 
// after initialization is complete.
// A thread-safe implementation will release the mutex acquired by __cxa_guard_acquire
// after setting the first byte of the guard object. 
//---------------------------------------------------------------------------------------
extern "C" void __cxa_guard_release( long long int* guard_object )
{
    TRACE_INFO( "__cxa_guard_release: Obj=%08x\n", (unsigned int) guard_object );
    }

//---------------------------------------------------------------------------------------
// This function is called if the initialization terminates by throwing an exception. 
// A thread-safe implementation will release the mutex acquired by __cxa_guard_acquire. 
//---------------------------------------------------------------------------------------
extern "C" void __cxa_guard_abort( long long int* guard_object )
{
    TRACE_INFO( "__cxa_guard_abort: Obj=%08x\n", (unsigned int) guard_object );
    }

//---------------------------------------------------------------------------------------
// An implementation shall provide a standard entry point that a compiler may reference
// in virtual tables to indicate a pure virtual function. This routine will only be called
// if the user calls a non-overridden pure virtual function, which has undefined behavior 
// according to the C++ Standard. Therefore, this ABI does not specify its behavior, but 
// it is expected that it will terminate the program, possibly with an error message. 
//---------------------------------------------------------------------------------------
extern "C" void __cxa_pure_virtual( void )
{
    TRACE_FATAL( "__cxa_pure_virtual: Fatal error\n" );
    do ; while( true );
}

