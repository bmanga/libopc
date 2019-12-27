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
#include <mce/textreader.h>


int mceTextReaderInit(mceTextReader_t *mceTextReader, xmlTextReaderPtr reader) {
    memset(mceTextReader, 0, sizeof(*mceTextReader));
    mceCtxInit(&mceTextReader->mceCtx);
    mceTextReader->reader=reader;
    return (NULL!=mceTextReader->reader?0:-1);
}

int mceTextReaderCleanup(mceTextReader_t *mceTextReader) {
    mceCtxCleanup(&mceTextReader->mceCtx);
    xmlTextReaderClose(mceTextReader->reader);
    xmlFreeTextReader(mceTextReader->reader);
    return 0;
}

static xmlChar *xmlStrDupArray(const xmlChar *value) {
    uint32_t len=xmlStrlen(value);
    xmlChar *ret=(xmlChar *)xmlMalloc((2+len)*sizeof(xmlChar));
    uint32_t j=0;
    for(uint32_t i=0;i<len;i++) {
        while(i<len && (value[i]==' ' || value[i]=='\t' || value[i]=='\r' || value[i]=='\n')) i++; // skip preceeding spaces
        while(i<len && value[i]!=' ' && value[i]!='\t' && value[i]!='\r' && value[i]!='\n') ret[j++]=value[i++];
        ret[j++]=0;
    }
    ret[j]=0;
    return ret;
}

static xmlChar *xmlStrArrayFirst(xmlChar *a, int *entry_len) {
    assert(NULL!=entry_len);
    *entry_len=xmlStrlen(a);
    return a;
}

static bool xmlStrArrayValid(const xmlChar *e) {
    return NULL!=e && *e!=0;
}

static xmlChar *xmlStrArrayNext(xmlChar *e, int *entry_len) {
    assert(NULL!=entry_len);
    assert(xmlStrArrayValid(e));
    xmlChar *ret=e+1+*entry_len;
    *entry_len=xmlStrlen(ret);
    return ret;
}

void mceRaiseError(xmlTextReader *reader, mceCtx_t *ctx, mceError_t error, const xmlChar *str, ...) {
    va_list args;
    va_start(args, str);
    assert(MCE_ERROR_NONE==ctx->error); // called twice? why?
    ctx->error=error;
    xmlChar buf[1024];
    xmlStrVPrintf(buf, sizeof(buf), str, args);
    xmlTextReaderErrorFunc f=NULL;
    void *arg=NULL;
    xmlTextReaderGetErrorHandler(reader, &f, &arg);    
    if (NULL!=f) {
        f(arg, (const char *)buf, XML_PARSER_SEVERITY_ERROR, (xmlTextReaderLocatorPtr)reader);
    } else {
        xmlGenericError(xmlGenericErrorContext, "%s!\n", (const char *)buf);
    }
    va_end(args);
}


static char ns_mce[]="http://schemas.openxmlformats.org/markup-compatibility/2006";
static const char ns_xml[]="http://www.w3.org/2000/xmlns/";
static const char _xmlns[]="xmlns";

static void mceTextReaderProcessAttributes(xmlTextReader *reader, mceCtx_t *ctx, uint32_t level) {
    xmlNodePtr c=xmlTextReaderCurrentNode(reader);
    if (NULL!=c) { // make sure to inherit any namespace declaration on a parent MCE element
        for(xmlNodePtr n=c->parent;
            NULL!=n && XML_ELEMENT_NODE==n->type 
            && NULL!=n->ns && 0==xmlStrcmp(n->ns->href, BAD_CAST(ns_mce)) 
            && NULL!=n->name
            && (0==xmlStrcmp(BAD_CAST("AlternateContent"), n->name) 
                || 0==xmlStrcmp(BAD_CAST("Choice"), n->name)  
                || 0==xmlStrcmp(BAD_CAST("Fallback"), n->name));
            n=n->parent) {
            for(xmlNs *nsDef=n->nsDef;NULL!=nsDef;nsDef=nsDef->next) {
                xmlAttrPtr a=xmlHasNsProp(c, nsDef->prefix, BAD_CAST(ns_xml));
                if (NULL==a) { // only add a namespace if the prefix is not overwritten in the current node                    
                    xmlNsPtr xmlns_ns=xmlNewNs(c, nsDef->href, nsDef->prefix);
                }
            }
        }
    }
    if (1==xmlTextReaderHasAttributes(reader)) {
        if (1==xmlTextReaderMoveToFirstAttribute(reader)) {
            do {
                if (0==xmlStrcmp(BAD_CAST("Ignorable"), xmlTextReaderConstLocalName(reader)) &&
                    0==xmlStrcmp(BAD_CAST(ns_mce), xmlTextReaderConstNamespaceUri(reader))) {
                        xmlChar *v=xmlStrDupArray(xmlTextReaderConstValue(reader));
                        int prefix_len=0;
                        for(xmlChar *prefix=xmlStrArrayFirst(v, &prefix_len);xmlStrArrayValid(prefix);prefix=xmlStrArrayNext(prefix, &prefix_len)) {
                            xmlChar *ns_=xmlTextReaderLookupNamespace(reader, prefix);
                            if (NULL!=ns_ && NULL==mceQNameLevelLookup(&ctx->understands_set, ns_, NULL, false)) {
                                assert(mceQNameLevelAdd(&ctx->ignorable_set, ns_, NULL, level));
                            }
                            xmlFree(ns_);
                        }
                        xmlFree(v);
                } else if (0==xmlStrcmp(BAD_CAST("ProcessContent"), xmlTextReaderConstLocalName(reader)) &&
                           0==xmlStrcmp(BAD_CAST(ns_mce), xmlTextReaderConstNamespaceUri(reader))) {
                        xmlChar *v=xmlStrDupArray(xmlTextReaderConstValue(reader));
                        int qname_len=0;
                        for(xmlChar *qname=xmlStrArrayFirst(v, &qname_len);xmlStrArrayValid(qname);qname=xmlStrArrayNext(qname, &qname_len)) {
                            int prefix=0; while(qname[prefix]!=':' && qname[prefix]!=0) prefix++;
                            assert(prefix<=qname_len);
                            int ln=(prefix<qname_len?prefix+1:0);
                            if (prefix<qname_len) {
                                qname[prefix]=0;
                                prefix=0;
                            };
                            xmlChar *ns_=xmlTextReaderLookupNamespace(reader, qname+prefix);
                            if (NULL!=ns_ && NULL==mceQNameLevelLookup(&ctx->understands_set, ns_, NULL, false)) {
                                assert(mceQNameLevelAdd(&ctx->processcontent_set, ns_, xmlStrdup(qname+ln), level));

                            }
                        }
                        xmlFree(v);
                } else if (0==xmlStrcmp(BAD_CAST("MustUnderstand"), xmlTextReaderConstLocalName(reader)) &&
                           0==xmlStrcmp(BAD_CAST(ns_mce), xmlTextReaderConstNamespaceUri(reader))) {
                        xmlChar *v=xmlStrDupArray(xmlTextReaderConstValue(reader));
                        int prefix_len=0;
                        for(xmlChar *prefix=xmlStrArrayFirst(v, &prefix_len);xmlStrArrayValid(prefix);prefix=xmlStrArrayNext(prefix, &prefix_len)) {
                            xmlChar *ns_=xmlTextReaderLookupNamespace(reader, prefix);
                            if (NULL!=ns_ && NULL==mceQNameLevelLookup(&ctx->understands_set, ns_, NULL, false)) {
                                mceRaiseError(reader, ctx, MCE_ERROR_MUST_UNDERSTAND, BAD_CAST("MustUnderstand namespace \"%s\""), ns_);
                            }
                        }
                        xmlFree(v);
#if (MCE_NAMESPACE_SUBSUMPTION_ENABLED)
                } else if (0==xmlStrcmp(BAD_CAST(ns_xml), xmlTextReaderConstNamespaceUri(reader))) {
                    mceQNameLevel_t *qnl=mceQNameLevelLookup(&ctx->subsume_prefix_set, xmlTextReaderConstValue(reader), NULL, true);
                    if (NULL!=qnl) {
                        const xmlChar *prefix=xmlTextReaderConstLocalName(reader);
                        if (0!=xmlStrcmp(prefix, qnl->ln)) {
                            // different binding!
                            printf("prefix=%s\n", prefix);
                            assert(0); //@TODO add store prefix for subsumption namespaces...
                        }
                    }
                } else {
                    const xmlChar *ns_orig=xmlTextReaderConstNamespaceUri(reader);
                    const xmlChar *ns=mceTextReaderSubsumeNS(ctx, ns_orig, xmlTextReaderConstLocalName(reader));
                    if (ns_orig!=ns) {
                        // namespace needs to be subsumed...
                    }
#endif
                }
            } while (1==xmlTextReaderMoveToNextAttribute(reader));
        }
        xmlAttrPtr remove=NULL;
        if (1==xmlTextReaderMoveToFirstAttribute(reader)) {
            do {
                if (NULL!=remove) {
                    xmlRemoveProp(remove); remove=NULL;
                }
                if (0==xmlStrcmp(BAD_CAST(ns_mce), xmlTextReaderConstNamespaceUri(reader))) {
                    assert(XML_ATTRIBUTE_NODE==xmlTextReaderCurrentNode(reader)->type);
                    remove=(xmlAttrPtr)xmlTextReaderCurrentNode(reader);
                } else if (NULL!=mceQNameLevelLookup(&ctx->ignorable_set,
                    xmlTextReaderConstNamespaceUri(reader), 
                    NULL, 
                    false)) {
                        assert(XML_ATTRIBUTE_NODE==xmlTextReaderCurrentNode(reader)->type);
                        remove=(xmlAttrPtr)xmlTextReaderCurrentNode(reader);
                }
            } while (1==xmlTextReaderMoveToNextAttribute(reader));
            assert(1==xmlTextReaderMoveToElement(reader));
            if (NULL!=remove) {
                xmlRemoveProp(remove); remove=NULL;
            }
            assert(NULL==remove);

        }
    }
}

static bool mceTextReaderProcessStartElement(xmlTextReader *reader, mceCtx_t *ctx, uint32_t level, const xmlChar *ns, const xmlChar *ln) {
    if (!mceSkipStackSkip(&ctx->skip_stack, level)) {
        mceTextReaderProcessAttributes(reader, ctx, level);
        if (0==xmlStrcmp(BAD_CAST(ns_mce), ns) && 0==xmlStrcmp(BAD_CAST("AlternateContent"), ln)) {
            mceSkipStackPush(&ctx->skip_stack, level, level+1, MCE_SKIP_STATE_ALTERNATE_CONTENT);
        } else if (0==xmlStrcmp(BAD_CAST(ns_mce), ns) && 0==xmlStrcmp(BAD_CAST("Choice"), ln)) {
            xmlChar *req_ns=NULL;
            if (1==xmlTextReaderMoveToAttribute(reader, BAD_CAST("Requires"))) {
                req_ns=xmlTextReaderLookupNamespace(reader, xmlTextReaderConstValue(reader));
                assert(1==xmlTextReaderMoveToElement(reader));
            } else if (1==xmlTextReaderMoveToAttributeNs(reader, BAD_CAST("Requires"), BAD_CAST(ns_mce))) {
                req_ns=xmlTextReaderLookupNamespace(reader, xmlTextReaderConstValue(reader));
                assert(1==xmlTextReaderMoveToElement(reader));
            }
            if (NULL==req_ns) {
                mceRaiseError(reader, ctx, MCE_ERROR_XML, BAD_CAST("Missing \"Requires\" attribute"));
            } else if (NULL==mceSkipStackTop(&ctx->skip_stack)
                   || !(mceSkipStackTop(&ctx->skip_stack)->state==MCE_SKIP_STATE_ALTERNATE_CONTENT || mceSkipStackTop(&ctx->skip_stack)->state==MCE_SKIP_STATE_CHOICE_MATCHED)
                   || mceSkipStackTop(&ctx->skip_stack)->level_start+1!=level
                   || mceSkipStackTop(&ctx->skip_stack)->level_end!=level) {
                mceRaiseError(reader, ctx, MCE_ERROR_XML, BAD_CAST("Choice element does not appear at a valid position."));
            } else if (mceQNameLevelLookup(&ctx->understands_set, req_ns, NULL, false) && mceSkipStackTop(&ctx->skip_stack)->state!=MCE_SKIP_STATE_CHOICE_MATCHED) {
                mceSkipStackTop(&ctx->skip_stack)->state=MCE_SKIP_STATE_CHOICE_MATCHED;
                mceSkipStackPush(&ctx->skip_stack, level, level+1, MCE_SKIP_STATE_IGNORE);
            } else {
                mceSkipStackPush(&ctx->skip_stack, level, UINT32_MAX, MCE_SKIP_STATE_IGNORE);
            }
            if (NULL!=req_ns) xmlFree(req_ns);
        } else if (0==xmlStrcmp(BAD_CAST(ns_mce), ns) && 0==xmlStrcmp(BAD_CAST("Fallback"), ln)) {
            if (NULL==mceSkipStackTop(&ctx->skip_stack) 
                || !(mceSkipStackTop(&ctx->skip_stack)->state==MCE_SKIP_STATE_ALTERNATE_CONTENT || mceSkipStackTop(&ctx->skip_stack)->state==MCE_SKIP_STATE_CHOICE_MATCHED)
                || mceSkipStackTop(&ctx->skip_stack)->level_start+1!=level
                || mceSkipStackTop(&ctx->skip_stack)->level_end!=level) {
                mceRaiseError(reader, ctx, MCE_ERROR_XML, BAD_CAST("Fallback element does not appear at a valid position."));
            } else if (mceSkipStackTop(&ctx->skip_stack)->state!=MCE_SKIP_STATE_CHOICE_MATCHED) {
                mceSkipStackPush(&ctx->skip_stack, level, level+1, MCE_SKIP_STATE_IGNORE);
            } else {
                mceSkipStackPush(&ctx->skip_stack, level, UINT32_MAX, MCE_SKIP_STATE_IGNORE);
            }
        } else if (mceQNameLevelLookup(&ctx->ignorable_set, ns, NULL, false)
            && !mceQNameLevelLookup(&ctx->understands_set, ns, NULL, false)) {
                if (mceQNameLevelLookup(&ctx->processcontent_set, ns, ln, false)) {
                    mceSkipStackPush(&ctx->skip_stack, level, level+1, MCE_SKIP_STATE_IGNORE);
                } else {
                    mceSkipStackPush(&ctx->skip_stack, level, UINT32_MAX, MCE_SKIP_STATE_IGNORE);
                }
        }
    }
    return mceSkipStackSkip(&ctx->skip_stack, level);
}

static bool mceTextReaderProcessEndElement(xmlTextReader *reader, mceCtx_t *ctx, uint32_t level, const xmlChar *ns, const xmlChar *ln) {
    bool skiped=false;
    if (mceSkipStackSkip(&ctx->skip_stack, level)) {
        if (mceSkipStackTop(&ctx->skip_stack)->level_start==level) {
            mceSkipStackPop(&ctx->skip_stack);
        } else if (mceSkipStackTop(&ctx->skip_stack)->level_end==level) {
            mceSkipStackTop(&ctx->skip_stack)->level_end--;
            assert(mceSkipStackTop(&ctx->skip_stack)->level_start<mceSkipStackTop(&ctx->skip_stack)->level_end);
        }
        skiped=true;
    }
    mceQNameLevelCleanup(&ctx->ignorable_set, level);
    mceQNameLevelCleanup(&ctx->processcontent_set, level);
    mceQNameLevelCleanup(&ctx->understands_set, level);
    mceQNameLevelCleanup(&ctx->suspended_set, level); 
    return skiped;
}

int mceTextReaderPostprocess(xmlTextReader *reader, mceCtx_t *ctx, int ret) {
    bool suspend=ctx->mce_disabled;
    const xmlChar *ns=NULL;
    const xmlChar *ln=NULL;
    if (MCE_ERROR_NONE!=ctx->error) {
        ret=-1;
    } else {
        ns=xmlTextReaderConstNamespaceUri(reader);
        ln=xmlTextReaderConstLocalName(reader);
        if (XML_READER_TYPE_ELEMENT==xmlTextReaderNodeType(reader)) {
            if (ctx->suspended_level>0 || NULL!=mceQNameLevelLookup(&ctx->suspended_set, ns, ln, false)) {
                suspend=true;
                if (!xmlTextReaderIsEmptyElement(reader)) {
                    ctx->suspended_level++;
                }
            }
        } else if (ctx->suspended_level>0) {
            suspend=true;
            if (XML_READER_TYPE_END_ELEMENT==xmlTextReaderNodeType(reader)) {
                ctx->suspended_level--;
            }
        }
    }
    bool skip=!suspend;
    while(1==ret && skip) {
        if (XML_READER_TYPE_ELEMENT==xmlTextReaderNodeType(reader)) {
            skip=mceTextReaderProcessStartElement(reader, ctx, xmlTextReaderDepth(reader), ns, ln);
            if (xmlTextReaderIsEmptyElement(reader)) {
                assert(mceTextReaderProcessEndElement(reader, ctx, xmlTextReaderDepth(reader), ns, ln)==skip);
            }
        } else if (XML_READER_TYPE_END_ELEMENT==xmlTextReaderNodeType(reader)) {
            skip=mceTextReaderProcessEndElement(reader, ctx, xmlTextReaderDepth(reader), ns, ln);
        } else {
            skip=mceSkipStackSkip(&ctx->skip_stack, xmlTextReaderDepth(reader));
        }
        if (skip) {
            if (-1!=(ret=xmlTextReaderRead(reader))) { // skip element
                ns=xmlTextReaderConstNamespaceUri(reader); // get next ns
                ln=xmlTextReaderConstLocalName(reader);  // get net local name
            }
        }
        if (MCE_ERROR_NONE!=ctx->error) {
            ret=-1;
        }
    }
    if (-1==ret && MCE_ERROR_NONE==ctx->error) {
        ctx->error=MCE_ERROR_XML; //@TODO be more specific about the type of libxml2 error.
    }
    return ret;
}


mceError_t mceTextReaderGetError(mceTextReader_t *mceTextReader) {
    return mceTextReader->mceCtx.error;
}

int mceTextReaderRead(mceTextReader_t *mceTextReader) {
    return mceTextReaderPostprocess(mceTextReader->reader, &mceTextReader->mceCtx, xmlTextReaderRead(mceTextReader->reader));
}

int mceTextReaderNext(mceTextReader_t *mceTextReader) {
    return mceTextReaderPostprocess(mceTextReader->reader, &mceTextReader->mceCtx, xmlTextReaderNext(mceTextReader->reader));
}


int mceTextReaderDump(mceTextReader_t *mceTextReader, xmlTextWriter *writer, bool fragment) {
    int ret=-1;
    if (XML_READER_TYPE_ELEMENT==xmlTextReaderNodeType(mceTextReader->reader)) {
        const xmlChar *ns=xmlTextReaderConstNamespaceUri(mceTextReader->reader);
        const xmlChar *ln=xmlTextReaderConstLocalName(mceTextReader->reader);
        if (NULL!=ns) {
            xmlTextWriterStartElementNS(writer, xmlTextReaderConstPrefix(mceTextReader->reader), ln, NULL);
        } else {
            xmlTextWriterStartElement(writer, ln);
        }
        if (xmlTextReaderHasAttributes(mceTextReader->reader)) {
            if (1==xmlTextReaderMoveToFirstAttribute(mceTextReader->reader)) {
                do {
                    const xmlChar *attr_ns=xmlTextReaderConstNamespaceUri(mceTextReader->reader);
                    const xmlChar *attr_ln=xmlTextReaderConstLocalName(mceTextReader->reader);
                    const xmlChar *attr_val=xmlTextReaderConstValue(mceTextReader->reader);
                    if (NULL!=attr_ns && 0==xmlStrcmp(attr_ns, BAD_CAST(ns_xml))) {
                        if (0==xmlStrcmp(attr_ln, BAD_CAST(_xmlns))) {
                            xmlTextWriterWriteAttribute(writer, attr_ln, attr_val);
                        } else {
                            xmlTextWriterWriteAttributeNS(writer, BAD_CAST(_xmlns), attr_ln, NULL, attr_val);
                        }
                    } else {
                        if (NULL!=ns) {
                            xmlTextWriterWriteAttributeNS(writer, xmlTextReaderConstPrefix(mceTextReader->reader), attr_ln, NULL, attr_val);
                        } else {
                            xmlTextWriterWriteAttribute(writer, attr_ln, attr_val);
                        }
                    }
                } while (1==xmlTextReaderMoveToNextAttribute(mceTextReader->reader));
            }
            assert(1==xmlTextReaderMoveToElement(mceTextReader->reader));
        }
        if (!xmlTextReaderIsEmptyElement(mceTextReader->reader)) {
            ret=mceTextReaderRead(mceTextReader); // read start element
            while (1==ret && XML_READER_TYPE_END_ELEMENT!=xmlTextReaderNodeType(mceTextReader->reader)) {
                ret=mceTextReaderDump(mceTextReader, writer, fragment);
            }
            assert(-1==ret || XML_READER_TYPE_END_ELEMENT==xmlTextReaderNodeType(mceTextReader->reader));
            ret=mceTextReaderRead(mceTextReader); // read end element
        } else {
            ret=mceTextReaderRead(mceTextReader); // read empty element
        }
        xmlTextWriterEndElement(writer);
    } else if (XML_READER_TYPE_TEXT==xmlTextReaderNodeType(mceTextReader->reader)) {
        xmlTextWriterWriteString(writer, xmlTextReaderConstValue(mceTextReader->reader));
        ret=mceTextReaderRead(mceTextReader); // read end element
    } else if (XML_READER_TYPE_SIGNIFICANT_WHITESPACE==xmlTextReaderNodeType(mceTextReader->reader)) {
        xmlTextWriterWriteString(writer, xmlTextReaderConstValue(mceTextReader->reader));
        ret=mceTextReaderRead(mceTextReader); // read end element
    } else if (XML_READER_TYPE_NONE==xmlTextReaderNodeType(mceTextReader->reader)) {
        ret=mceTextReaderRead(mceTextReader);
        if (1==ret && XML_READER_TYPE_NONE!=xmlTextReaderNodeType(mceTextReader->reader)) {
            if (!fragment) xmlTextWriterStartDocument(writer, NULL, NULL, NULL);
            ret=mceTextReaderDump(mceTextReader, writer, fragment);
            if (!fragment) xmlTextWriterEndDocument(writer);
        }
    } else {
        if (1==(ret=mceTextReaderNext(mceTextReader))) { // skip element
            ret=mceTextReaderDump(mceTextReader, writer, fragment);
        }
    }
    return ret;
}

int mceTextReaderUnderstandsNamespace(mceTextReader_t *mceTextReader, const xmlChar *ns) {
    return (mceCtxUnderstandsNamespace(&mceTextReader->mceCtx, ns)?0:-1);
}

bool mceTextReaderDisableMCE(mceTextReader_t *mceTextReader, bool flag) {
    bool ret=mceTextReader->mceCtx.mce_disabled>0;
    mceTextReader->mceCtx.mce_disabled=flag;
    return ret;
}
