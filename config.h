#ifndef OPC_CONFIG_H
#define OPC_CONFIG_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>
//#include <assert.h>
#include <libxml/xmlstring.h>

#ifndef __cplusplus
#include <stdbool.h>
#endif

static inline void myassert(bool x) {}

#define assert(x) myassert(x)


#define OPC_MAX_PATH 512
#define OPC_DEFLATE_BUFFER_SIZE 4096

#ifndef SEEK_SET
#  define SEEK_SET        0
#endif

#ifndef SEEK_CUR
#  define SEEK_CUR        1
#endif

#ifndef SEEK_END
#  define SEEK_END        2
#endif

    /**
      Error codes for the OPC module.
      */
    typedef enum OPC_ERROR_ENUM {
        OPC_ERROR_NONE,
        OPC_ERROR_STREAM,
        OPC_ERROR_SEEK, // can't seek
        OPC_ERROR_UNSUPPORTED_DATA_DESCRIPTOR,
        OPC_ERROR_UNSUPPORTED_COMPRESSION,
        OPC_ERROR_DEFLATE,
        OPC_ERROR_HEADER,
        OPC_ERROR_MEMORY,
        OPC_ERROR_XML, 
        OPC_ERROR_USER // user triggered an abort
    } opc_error_t;
    
    /**
      Compression options for OPC streams.
      */
    typedef enum OPC_COMPRESSIONOPTION_ENUM {
        OPC_COMPRESSIONOPTION_NONE,
        OPC_COMPRESSIONOPTION_NORMAL,
        OPC_COMPRESSIONOPTION_MAXIMUM,
        OPC_COMPRESSIONOPTION_FAST,
        OPC_COMPRESSIONOPTION_SUPERFAST
    } opcCompressionOption_t;


/**
  Abstraction for memset(m, 0, s).
  \hideinitializer
 */
#define opc_bzero_mem(m,s) memset(m, 0, s)

#ifdef NDEBUG
#define OPC_ENSURE(e) (void)(e)
#else
#define OPC_ENSURE(e) assert(e)
#endif


#endif
