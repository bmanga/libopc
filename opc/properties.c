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
#include <opc/properties.h>
#include <libxml/xmlmemory.h>
#include <opc/xmlreader.h>
#include <mce/textreader.h>
#include <mce/textwriter.h>

opc_error_t opcCorePropertiesInit(opcProperties_t *cp) {
    opc_bzero_mem(cp, sizeof(*cp));
    return OPC_ERROR_NONE;
}

static void opcDCSimpleTypeFree(opcDCSimpleType_t *v) {
    if (NULL!=v) {
        if (NULL!=v->lang) xmlFree(v->lang);
        if (NULL!=v->str) xmlFree(v->str);
    }
}

opc_error_t opcCorePropertiesCleanup(opcProperties_t *cp) {
    if (NULL!=cp->category) xmlFree(cp->category);
    if (NULL!=cp->contentStatus) xmlFree(cp->contentStatus);
    if (NULL!=cp->created) xmlFree(cp->created);
    opcDCSimpleTypeFree(&cp->creator);
    opcDCSimpleTypeFree(&cp->description);
    opcDCSimpleTypeFree(&cp->identifier);
    for(uint32_t i=0;i<cp->keyword_items;i++) {
        opcDCSimpleTypeFree(&cp->keyword_array[i]);
    }
    if (NULL!=cp->keyword_array) {
        xmlFree(cp->keyword_array);
    }
    opcDCSimpleTypeFree(&cp->language);
    if (NULL!=cp->lastModifiedBy) xmlFree(cp->lastModifiedBy);
    if (NULL!=cp->lastPrinted) xmlFree(cp->lastPrinted);
    if (NULL!=cp->modified) xmlFree(cp->modified);
    if (NULL!=cp->revision) xmlFree(cp->revision);
    opcDCSimpleTypeFree(&cp->subject);
    opcDCSimpleTypeFree(&cp->title);
    if (NULL!=cp->version) xmlFree(cp->version);
    return OPC_ERROR_NONE;
}

opc_error_t opcCorePropertiesSetString(xmlChar **prop, const xmlChar *str) {
    if (NULL!=prop) {
        if (NULL!=*prop) xmlFree(*prop);
        *prop=xmlStrdup(str);
    }
    return OPC_ERROR_NONE;
}

opc_error_t opcCorePropertiesSetStringLang(opcDCSimpleType_t *prop, const xmlChar *str, const xmlChar *lang) {
    if (NULL!=prop) {
        if (NULL!=prop->str) xmlFree(prop->str);
        if (NULL!=prop->lang) xmlFree(prop->lang);
        prop->str=(NULL!=str?xmlStrdup(str):NULL);
        prop->lang=(NULL!=lang?xmlStrdup(lang):NULL);
    }
    return OPC_ERROR_NONE;
}

static const char cp_ns[]="http://schemas.openxmlformats.org/package/2006/metadata/core-properties";
static const char dc_ns[]="http://purl.org/dc/elements/1.1/";
static const char dcterms_ns[]="http://purl.org/dc/terms/";
static const char xml_ns[]="http://www.w3.org/XML/1998/namespace";
static const char xsi_ns[]="http://www.w3.org/2001/XMLSchema-instance";

static void _opcCorePropertiesAddKeywords(opcProperties_t *cp, const xmlChar *keywords, const xmlChar *lang) {
    int ofs=0;
    do {
        while(0!=keywords[ofs] && (' '==keywords[ofs] || '\t'==keywords[ofs] || '\r'==keywords[ofs] || '\n'==keywords[ofs])) ofs++;
        int start=ofs;
        while(0!=keywords[ofs] && ';'!=keywords[ofs]) ofs++;
        int end=ofs;
        while(end>start && (' '==keywords[end-1] || '\t'==keywords[end-1] || '\r'==keywords[end-1] || '\n'==keywords[end-1])) end--;
        if (end>start) {
            cp->keyword_array=(opcDCSimpleType_t*)xmlRealloc(cp->keyword_array, (cp->keyword_items+1)*sizeof(opcDCSimpleType_t));
            cp->keyword_array[cp->keyword_items].lang=(NULL!=lang?xmlStrdup(lang):NULL);
            cp->keyword_array[cp->keyword_items].str=xmlStrndup(keywords+start, end-start);
            cp->keyword_items++;
        }
        ofs++;
    } while(keywords[ofs-1]!=0);
}

static void _opcCorePropertiesReadString(mceTextReader_t *r, xmlChar **v) {
    mce_skip_attributes(r);
    mce_start_children(r) {
        mce_start_text(r) {
            if (NULL!=v && NULL!=*v) xmlFree(*v);
            *v=xmlStrdup(xmlTextReaderConstValue(r->reader));
        } mce_end_text(r);
    } mce_end_children(r);
}

static void _opcCorePropertiesReadSimpleType(mceTextReader_t *r, opcDCSimpleType_t *v) {
    opc_bzero_mem(v, sizeof(*v));
    mce_start_attributes(r) {
        mce_start_attribute(r, BAD_CAST(xml_ns), BAD_CAST("lang")) {
            if (NULL==v->lang) {
                v->lang=xmlStrdup(xmlTextReaderConstValue(r->reader));
            }
        } mce_end_attribute(r);
    } mce_end_attributes(r);
    mce_start_children(r) {
        mce_start_text(r) {
            if (NULL==v->str) {
                v->str=xmlStrdup(xmlTextReaderConstValue(r->reader));
            }
        } mce_end_text(r);
    } mce_end_children(r);
}

opc_error_t opcCorePropertiesRead(opcProperties_t *cp, opcContainer *c) {
    mceTextReader_t r;
    if (OPC_ERROR_NONE==opcXmlReaderOpen(c, &r, BAD_CAST("docProps/core.xml"), NULL, NULL, 0)) {
            mce_start_document(&r) {
                mce_start_element(&r, BAD_CAST(cp_ns), BAD_CAST("coreProperties")) {
                    mce_skip_attributes(&r);
                    mce_start_children(&r) {
                        mce_start_element(&r, BAD_CAST(cp_ns), BAD_CAST("category")) {
                            _opcCorePropertiesReadString(&r, &cp->category);
                        } mce_end_element(&r);
                        mce_start_element(&r, BAD_CAST(cp_ns), BAD_CAST("contentStatus")) {
                            _opcCorePropertiesReadString(&r, &cp->contentStatus);
                        } mce_end_element(&r);
                        mce_start_element(&r, BAD_CAST(dcterms_ns), BAD_CAST("created")) {
                            _opcCorePropertiesReadString(&r, &cp->created);
                        } mce_end_element(&r);
                        mce_start_element(&r, BAD_CAST(dc_ns), BAD_CAST("creator")) {
                            _opcCorePropertiesReadSimpleType(&r, &cp->creator);
                        } mce_end_element(&r);
                        mce_start_element(&r, BAD_CAST(dc_ns), BAD_CAST("description")) {
                            _opcCorePropertiesReadSimpleType(&r, &cp->description);
                        } mce_end_element(&r);
                        mce_start_element(&r, BAD_CAST(dc_ns), BAD_CAST("identifier")) {
                            _opcCorePropertiesReadSimpleType(&r, &cp->identifier);
                        } mce_end_element(&r);
                        mce_start_element(&r, BAD_CAST(cp_ns), BAD_CAST("keywords")) {
                            xmlChar *xml_lang=NULL;
                            mce_start_attributes(&r) {
                                mce_start_attribute(&r, BAD_CAST(xml_ns), BAD_CAST("lang")) {
                                    xml_lang=xmlStrdup(xmlTextReaderConstValue(r.reader));
                                } mce_end_attribute(&r);
                            } mce_end_attributes(&r);
                            mce_start_children(&r) {
                                mce_start_element(&r, BAD_CAST(cp_ns), BAD_CAST("value")) {
                                    xmlChar *value_xml_lang=NULL;
                                    mce_start_attributes(&r) {
                                        mce_start_attribute(&r, BAD_CAST(xml_ns), BAD_CAST("lang")) {
                                            value_xml_lang=xmlStrdup(xmlTextReaderConstValue(r.reader));
                                        } mce_end_attribute(&r);
                                    } mce_end_attributes(&r);
                                    mce_start_children(&r) {
                                        mce_start_text(&r) {
                                            _opcCorePropertiesAddKeywords(cp, xmlTextReaderConstValue(r.reader), value_xml_lang);
                                        } mce_end_text(&r);
                                    } mce_end_children(&r);
                                if (NULL!=value_xml_lang) { xmlFree(value_xml_lang); value_xml_lang=NULL; };
                                } mce_end_element(&r);
                                mce_start_text(&r) {
                                    _opcCorePropertiesAddKeywords(cp, xmlTextReaderConstValue(r.reader), xml_lang);
                                } mce_end_text(&r);
                            } mce_end_children(&r);
                            if (NULL!=xml_lang) { xmlFree(xml_lang); xml_lang=NULL; };
                        } mce_end_element(&r);
                        mce_start_element(&r, BAD_CAST(dc_ns), BAD_CAST("language")) {
                            _opcCorePropertiesReadSimpleType(&r, &cp->language);
                        } mce_end_element(&r);
                        mce_start_element(&r, BAD_CAST(cp_ns), BAD_CAST("lastModifiedBy")) {
                            _opcCorePropertiesReadString(&r, &cp->lastModifiedBy);
                        } mce_end_element(&r);
                        mce_start_element(&r, BAD_CAST(cp_ns), BAD_CAST("lastPrinted")) {
                            _opcCorePropertiesReadString(&r, &cp->lastPrinted);
                        } mce_end_element(&r);
                        mce_start_element(&r, BAD_CAST(dcterms_ns), BAD_CAST("modified")) {
                            _opcCorePropertiesReadString(&r, &cp->modified);
                        } mce_end_element(&r);
                        mce_start_element(&r, BAD_CAST(cp_ns), BAD_CAST("revision")) {
                            _opcCorePropertiesReadString(&r, &cp->revision);
                        } mce_end_element(&r);
                        mce_start_element(&r, BAD_CAST(dc_ns), BAD_CAST("subject")) {
                            _opcCorePropertiesReadSimpleType(&r, &cp->subject);
                        } mce_end_element(&r);
                        mce_start_element(&r, BAD_CAST(dc_ns), BAD_CAST("title")) {
                            _opcCorePropertiesReadSimpleType(&r, &cp->title);
                        } mce_end_element(&r);
                        mce_start_element(&r, BAD_CAST(cp_ns), BAD_CAST("version")) {
                            _opcCorePropertiesReadString(&r, &cp->version);
                        } mce_end_element(&r);
                    } mce_end_children(&r);
                } mce_end_element(&r);
            } mce_end_document(&r);
        mceTextReaderCleanup(&r);
    }
    return OPC_ERROR_NONE;
}

static void _opcCorePropertiesWriteString(mceTextWriter *w, const xmlChar *ns, const xmlChar *name, const xmlChar *value, const xmlChar *type) {
    if (NULL!=value) {
        mceTextWriterStartElement(w, ns, name);
        if (NULL!=type) {
            mceTextWriterAttributeF(w, BAD_CAST(xsi_ns), BAD_CAST("type"), (const char *)type);
        }
        mceTextWriterWriteString(w, value);
        mceTextWriterEndElement(w, ns, name);
    }
}

static void _opcCorePropertiesWriteSimpleType(mceTextWriter *w, const xmlChar *ns, const xmlChar *name, const opcDCSimpleType_t*value) {
    if (NULL!=value->str) {
        mceTextWriterStartElement(w, ns, name);
        if (NULL!=value->lang) {
            mceTextWriterAttributeF(w, BAD_CAST(xml_ns), BAD_CAST("lang"), (const char *)value->lang);
        }
        mceTextWriterWriteString(w, value->str);
        mceTextWriterEndElement(w, ns, name);
    }
}

opc_error_t opcCorePropertiesWrite(opcProperties_t *cp, opcContainer *c) {
    opcPart part=opcPartCreate(c, BAD_CAST("docProps/core.xml"), BAD_CAST("application/vnd.openxmlformats-package.core-properties+xml"), 0);
    if (NULL!=part) {
        mceTextWriter *w=mceTextWriterOpen(c, part, OPC_COMPRESSIONOPTION_SUPERFAST);
        if (NULL!=w) {
            mceTextWriterStartDocument(w);
            mceTextWriterRegisterNamespace(w, BAD_CAST(cp_ns), BAD_CAST("cp"), MCE_DEFAULT);
            mceTextWriterRegisterNamespace(w, BAD_CAST(dc_ns), BAD_CAST("dc"), MCE_DEFAULT);
            mceTextWriterRegisterNamespace(w, BAD_CAST(dcterms_ns), BAD_CAST("dcterms"), MCE_DEFAULT);
            mceTextWriterRegisterNamespace(w, BAD_CAST(xsi_ns), BAD_CAST("xsi"), MCE_DEFAULT);
            mceTextWriterStartElement(w, BAD_CAST(cp_ns), BAD_CAST("coreProperties"));
            _opcCorePropertiesWriteString(w, BAD_CAST(cp_ns), BAD_CAST("category"), cp->category, NULL);
            _opcCorePropertiesWriteString(w, BAD_CAST(cp_ns), BAD_CAST("contentStatus"), cp->contentStatus, NULL);
            _opcCorePropertiesWriteString(w, BAD_CAST(dcterms_ns), BAD_CAST("created"), cp->created, BAD_CAST("dcterms:W3CDTF"));
            _opcCorePropertiesWriteSimpleType(w, BAD_CAST(dc_ns), BAD_CAST("creator"), &cp->creator);
            _opcCorePropertiesWriteSimpleType(w, BAD_CAST(dc_ns), BAD_CAST("description"), &cp->description);
            _opcCorePropertiesWriteSimpleType(w, BAD_CAST(dc_ns), BAD_CAST("identifier"), &cp->identifier);
            if (cp->keyword_items>0) {
                mceTextWriterStartElement(w, BAD_CAST(cp_ns), BAD_CAST("keywords"));
                for(uint32_t i=0;i<cp->keyword_items;i++) {
                    _opcCorePropertiesWriteSimpleType(w, BAD_CAST(cp_ns), BAD_CAST("value"), &cp->keyword_array[i]);
                    if (i+1<cp->keyword_items) {
                        mceTextWriterWriteString(w, BAD_CAST(";"));
                    }
                }
                mceTextWriterEndElement(w, BAD_CAST(cp_ns), BAD_CAST("keywords"));
            }
            _opcCorePropertiesWriteSimpleType(w, BAD_CAST(dc_ns), BAD_CAST("language"), &cp->language);
            _opcCorePropertiesWriteString(w, BAD_CAST(cp_ns), BAD_CAST("lastModifiedBy"), cp->lastModifiedBy, NULL);
            _opcCorePropertiesWriteString(w, BAD_CAST(cp_ns), BAD_CAST("lastPrinted"), cp->lastPrinted, BAD_CAST("dcterms:W3CDTF"));
            _opcCorePropertiesWriteString(w, BAD_CAST(dcterms_ns), BAD_CAST("modified"), cp->modified, BAD_CAST("dcterms:W3CDTF"));
            _opcCorePropertiesWriteString(w, BAD_CAST(cp_ns), BAD_CAST("revision"), cp->revision, NULL);
            _opcCorePropertiesWriteSimpleType(w, BAD_CAST(dc_ns), BAD_CAST("subject"), &cp->subject);
            _opcCorePropertiesWriteSimpleType(w, BAD_CAST(dc_ns), BAD_CAST("title"), &cp->title);
            _opcCorePropertiesWriteString(w, BAD_CAST(cp_ns), BAD_CAST("version"), cp->version, NULL);
            mceTextWriterEndElement(w, BAD_CAST(cp_ns), BAD_CAST("coreProperties"));
            mceTextWriterEndDocument(w);
            mceTextWriterFree(w);
        }
    }
    return OPC_ERROR_NONE;
}
