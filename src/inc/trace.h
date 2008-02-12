#ifndef _TRACE_H
#define _TRACE_H

#if 0
#define TR_INFO
#define TR_WARNING
#define TR_ERROR
#define TR_FATAL
#endif

#if 0
#define TR_DEBUG_M  // Class-level debug
#define TR_DEBUG_L  // Device Driver-level debug
#endif

//------------------------------------------------------------------------------
//      Includes
//------------------------------------------------------------------------------

#ifdef __cplusplus
extern "C"  {
#endif

typedef void (*TracePut_f)( int out );

//------------------------------------------------------------------------------
// Init function. Must be called before first tracef()
//------------------------------------------------------------------------------
extern void tracef_open( int fd, TracePut_f putc, bool LF2CRLF, bool TimeStamp );

//------------------------------------------------------------------------------
// Small version of printf with limitted formating, but self sufficient and
// not stack hungry. Supports %%, %d, %0<n>X, %s, %c formats.
//------------------------------------------------------------------------------
extern void tracef
(
    int fd,
    const char* format, // Argument format
    ... // Arguments to format
    );

//------------------------------------------------------------------------------
//      Macro
//------------------------------------------------------------------------------

#if defined(TR_DEBUG_M)
    #define TRACE_DEBUG_M(...)      tracef(0,__VA_ARGS__)
#else
    #define TRACE_DEBUG_M(...)
#endif // TR_DEBUG_M

#ifdef TR_DEBUG_L
    #define TRACE_DEBUG_L(...)      tracef(0,__VA_ARGS__)
#else
    #define TRACE_DEBUG_L(...)
#endif // TR_DEBUG_L

#ifdef TR_INFO
    #define TRACE_INFO(...)         tracef(0,__VA_ARGS__)
#else
    #define TRACE_INFO(...)
#endif // TR_INFO

#ifdef TR_WARNING
    #define TRACE_WARNING(...)      tracef(0,__VA_ARGS__)
#else
    #define TRACE_WARNING(...)
#endif // TR_WARNING

#ifdef TR_ERROR
    #define TRACE_ERROR(...)        tracef(0,__VA_ARGS__)
#else
    #define TRACE_ERROR(...)
#endif // TR_ERROR

#ifdef TR_FATAL
    #define TRACE_FATAL(...)        tracef(0,__VA_ARGS__)
#else
    #define TRACE_FATAL(...)
#endif // TR_FATAL

#ifdef __cplusplus
} // extern "C"
#endif
    
#endif // _TRACE_H
