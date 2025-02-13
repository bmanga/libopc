/*
 Copyright (c) 2010, Florian Reuter
 All rights reserved.
 
 Redistribution and use in source and binary forms, with or without 
 modification, are permitted provided that the following conditions 
 are met:
 
 * Redistributions of source code must retain the above copyright 
   notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright 
   notice, this list of conditions and the following disclaimer in 
   the documentation and/or other materials provided with the 
   distribution.
 * Neither the name of Florian Reuter nor the names of its contributors 
   may be used to endorse or promote products derived from this 
   software without specific prior written permission.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE 
 COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, 
 INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
 BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
 CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
 STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED 
 OF THE POSSIBILITY OF SUCH DAMAGE.
 
*/
/* @file opc/internal.h
 Contains all internally shared datastructures.
 This file contains non-public definitions and will not be available for users in the SDK.
*/

#ifndef OPC_INTERNAL_H
#define OPC_INTERNAL_H

#include <opc/config.h>
#include <opc/container.h>
#include <opc/zip.h>
#include <zlib.h>
#include <opc/mce/textreader.h>

#ifdef __cplusplus
extern "C" {
#endif    

    typedef struct OPC_FILERAWBUFFER_STRUCT {
        opcFileRawState state;
        uint32_t   buf_ofs;
        uint32_t   buf_len;
        uint8_t buf[OPC_DEFLATE_BUFFER_SIZE];
    } opcFileRawBuffer;


    typedef struct OPC_ZIPSEGMENT_STRUCT {
        uint32_t deleted_segment :1;
        uint32_t rels_segment :1;
        uint32_t next_segment_id;
        const xmlChar *partName; // NOT!!! owned by me... owned by opcContainer
        size_t stream_ofs;
        size_t segment_size;
        uint16_t padding;
        uint32_t header_size;
        uint16_t bit_flag;
        uint32_t crc32;
        uint16_t compression_method;
        size_t compressed_size;
        size_t uncompressed_size;
        uint32_t growth_hint; 
    } opcZipSegment;

    struct OPC_ZIP_STRUCT {
        opcIO_t *io;
        uint32_t first_free_segment_id;
        opcZipSegment *segment_array;
        uint32_t segment_items;
    };

    typedef struct OPC_ZIPINFLATESTATE_STRUCT {
        z_stream stream;
        uint16_t compression_method;
        int inflate_state;
        size_t compressed_size;
    } opcZipInflateState;

    struct OPC_ZIPOUTPUTSTREAM_STRUCT {
        uint32_t segment_id;
        uint16_t compression_method;
        uint32_t crc32;
        z_stream stream;
        int inflate_state;
        uint32_t buf_len;
        uint32_t buf_ofs;
        uint32_t buf_size;
        uint8_t *buf /*[buf_size]*/;
    };

    struct OPC_ZIPINPUTSTREAM_STRUCT {
        uint32_t segment_id;
        opcZipInflateState inflateState;
        opcFileRawBuffer rawBuffer;
    };

    typedef struct OPC_QNAME_LEVEL {
        const xmlChar *ns;
        xmlChar *ln;
        uint32_t level;
        uint32_t alternatecontent_handled : 1;
    } opcQNameLevel_t;

    struct OPC_CONTAINER_INPUTSTREAM_STRUCT {
        opcZipInputStream *stream;
        opcContainer *container; // weak reference
        opc_error_t error;
        uint32_t reader_consume_element : 1;
        uint32_t reader_element_handled : 1;
        uint32_t reader_mce : 1;
    };

    struct OPC_CONTAINER_OUTPUTSTREAM_STRUCT {
        opcZipOutputStream *stream;
        uint32_t segment_id;  
        opcContainer *container; // weak reference
        const xmlChar *partName;
        bool rels_segment;
    };

    typedef struct OPC_CONTAINER_RELATION_TYPE_STRUCT {
        xmlChar *type;
    } opcContainerRelationType;

    typedef struct OPC_CONTAINER_EXTERNAL_RELATION_STRUCT {
        xmlChar *target;
    } opcContainerExternalRelation;


    struct OPC_RELATION_STRUCT {
        xmlChar *part_name; // owned by part array
        xmlChar *relation_id; // owned by relation array
    };

    typedef struct OPC_CONTAINER_RELATION_STRUCT {
        uint32_t relation_id;
        xmlChar *relation_type; // owned by relationtypes_array
        uint32_t target_mode; // 0==internal, 1==external
        xmlChar* target_ptr; // 0==targetMode: points to xmlChar owned part_array, 1==targetMode: points to xmlChar owned by externalrelation_array
    } opcContainerRelation;

    typedef struct OPC_CONTAINER_PART_STRUCT {
        xmlChar *name;
        const xmlChar *type; // owned by type_array
        uint32_t first_segment_id;
        uint32_t last_segment_id;
        uint32_t segment_count;
        uint32_t rel_segment_id;
        opcContainerRelation *relation_array;
        uint32_t relation_items;
    } opcContainerPart;

    typedef struct OPC_CONTAINER_PART_PREFIX_STRUCT {
        xmlChar *prefix;
    } opcContainerRelPrefix;

    #define OPC_CONTAINER_RELID_COUNTER_NONE 0xFFFF
    #define OPC_CONTAINER_RELID_COUNTER(rel) ((rel)&0xFFFF)
    #define OPC_CONTAINER_RELID_PREFIX(rel) (((rel)>>16)&0xFFFF)

    typedef struct OPC_CONTAINER_TYPE_STRUCT {
        xmlChar *type;
    } opcContainerType;

    typedef struct OPC_CONTAINER_EXTENSION_STRUCT {
        xmlChar *extension;
        const xmlChar *type; // owned by opcContainerType
    } opcContainerExtension;

    struct OPC_CONTAINER_STRUCT {
        opcIO_t io;
        opcZip *storage;
        opcContainerOpenMode mode;

        opcContainerPart *part_array;
        uint32_t part_items;
        opcContainerRelPrefix *relprefix_array;
        uint32_t relprefix_items;
        opcContainerType *type_array;
        uint32_t type_items;
        opcContainerExtension *extension_array;
        uint32_t extension_items;
        opcContainerRelationType *relationtype_array;
        uint32_t relationtype_items;
        opcContainerExternalRelation *externalrelation_array;
        uint32_t externalrelation_items;
        uint32_t content_types_segment_id;
        uint32_t rels_segment_id;
        opcContainerRelation *relation_array;
        uint32_t relation_items;
        void *userContext;
    };

    opc_error_t opcXmlReaderOpenEx(opcContainer *container, mceTextReader_t *mceTextReader, const xmlChar *partName, bool rels_segment, const char * URL, const char * encoding, int options);
    opcContainerInputStream* opcContainerOpenInputStreamEx(opcContainer *container, const xmlChar *name, bool rels_segment);
    opcContainerOutputStream* opcContainerCreateOutputStreamEx(opcContainer *container, const xmlChar *name, bool rels_segment, opcCompressionOption_t compression_option);


    opcContainerExtension *opcContainerInsertExtension(opcContainer *container, const xmlChar *extension, bool insert);
    opcContainerPart *opcContainerInsertPart(opcContainer *container, const xmlChar *name, bool insert);
    opc_error_t opcContainerDeletePart(opcContainer *container, const xmlChar *name);
    opcContainerRelation *opcContainerFindRelation(opcContainer *container, opcContainerRelation *relation_array, uint32_t relation_items, opcRelation relation);
    opc_error_t opcContainerDeleteRelation(opcContainer *container, opcContainerRelation **relation_array, uint32_t *relation_items, opcRelation relation);
    opcContainerRelation *opcContainerInsertRelation(opcContainerRelation **relation_array, uint32_t *relation_items, 
                                                     uint32_t relation_id,
                                                     xmlChar *relation_type,
                                                     uint32_t target_mode, xmlChar *target_ptr);
    opcContainerExternalRelation*insertExternalRelation(opcContainer *container, const xmlChar *target, bool insert);
    opcContainerRelationType *opcContainerInsertRelationType(opcContainer *container, const xmlChar *type, bool insert);
    opcContainerType *insertType(opcContainer *container, const xmlChar *type, bool insert);

    bool opcContainerDeletePartEx(opcContainer *container, const xmlChar *partName, bool rels_segment);

    opcContainerRelation *opcContainerFindRelationById(opcContainer *container, opcContainerRelation *relation_array, uint32_t relation_items, const xmlChar *relation_id);

    opc_error_t opcQNameLevelAdd(opcQNameLevel_t **list_array, uint32_t *list_items, opcQNameLevel_t *item);
    opcQNameLevel_t* opcQNameLevelLookup(opcQNameLevel_t *list_array, uint32_t list_items, const xmlChar *ns, const xmlChar *ln);
    opc_error_t opcQNameLevelCleanup(opcQNameLevel_t *list_array, uint32_t *list_items, uint32_t level, uint32_t *max_level);
    opc_error_t opcQNameLevelPush(opcQNameLevel_t **list_array, uint32_t *list_items, opcQNameLevel_t *item);
    bool opcQNameLevelPopIfMatch(opcQNameLevel_t *list_array, uint32_t *list_items, const xmlChar *ns, const xmlChar *ln, uint32_t level);

#ifdef __cplusplus
} /* extern "C" */
#endif    

#endif /* OPC_INTERNAL_H */
