#include <libxml/xmlmemory.h>
#include <opc/mce/helper.h>

static bool mceQNameLevelLookupEx(mceQNameLevelSet_t *qname_level_set, const xmlChar *ns, const xmlChar *ln, uint32_t *pos, bool ignore_ln) {
    uint32_t i=0;
    uint32_t j=qname_level_set->list_items;
    while(i<j) {
        uint32_t m=i+(j-i)/2;
        assert(i<=m && m<j);
        mceQNameLevel_t *q2=&qname_level_set->list_array[m];
        int const ns_cmp=(NULL==ns?(NULL==q2->ns?0:-1):(NULL==q2->ns?+1:xmlStrcmp(ns, q2->ns)));
        int const cmp=(ignore_ln?ns_cmp:(0==ns_cmp?xmlStrcmp(ln, q2->ln):ns_cmp));
        if (cmp<0) { j=m; } else if (cmp>0) { i=m+1; } else { *pos=m; return true; }
    }
    assert(i==j); 
    *pos=i;
    return false;
}

mceQNameLevel_t* mceQNameLevelLookup(mceQNameLevelSet_t *qname_level_set, const xmlChar *ns, const xmlChar *ln, bool ignore_ln) {
    uint32_t pos=0;
    return (mceQNameLevelLookupEx(qname_level_set, ns, ln, &pos, ignore_ln)?qname_level_set->list_array+pos:NULL);
}

bool mceQNameLevelAdd(mceQNameLevelSet_t *qname_level_set, const xmlChar *ns, const xmlChar *ln, uint32_t level) {
    uint32_t i=0;
    bool ret=false;
    if (!mceQNameLevelLookupEx(qname_level_set, ns, ln, &i, false)) {
        mceQNameLevel_t *new_list_array=NULL;
        if (NULL!=(new_list_array=(mceQNameLevel_t *)xmlRealloc(qname_level_set->list_array, (1+qname_level_set->list_items)*sizeof(*qname_level_set->list_array)))) {
            qname_level_set->list_array=new_list_array;
            for (uint32_t k=qname_level_set->list_items;k>i;k--) {
                qname_level_set->list_array[k]=qname_level_set->list_array[k-1];
            }
            qname_level_set->list_items++;
            assert(i>=0 && i<qname_level_set->list_items);
            memset(&qname_level_set->list_array[i], 0, sizeof(qname_level_set->list_array[i]));
            qname_level_set->list_array[i].level=level;
            qname_level_set->list_array[i].ln=(NULL!=ln?xmlStrdup(ln):NULL);
            qname_level_set->list_array[i].ns=xmlStrdup(ns);
            if (qname_level_set->max_level<level) qname_level_set->max_level=level;
            ret=true;
        }
    } else {
        ret=true;
    }
    return ret;
}

bool mceQNameLevelCleanup(mceQNameLevelSet_t *qname_level_set, uint32_t level) {
    if (qname_level_set->max_level>=level) {
        qname_level_set->max_level=0;
        uint32_t i=0;
        for(uint32_t j=0;j<qname_level_set->list_items;j++) {
            if (qname_level_set->list_array[j].level>=level) {
                assert(qname_level_set->list_array[j].level==level); // cleanup should be called for every level...
                if (NULL!=qname_level_set->list_array[j].ln) xmlFree(qname_level_set->list_array[j].ln);
                if (NULL!=qname_level_set->list_array[j].ns) xmlFree(qname_level_set->list_array[j].ns);
            } else {
                if (qname_level_set->list_array[j].level>qname_level_set->max_level) {
                    qname_level_set->max_level=qname_level_set->list_array[j].level;
                }
                qname_level_set->list_array[i++]=qname_level_set->list_array[j];
            }
        }
        qname_level_set->list_items=i;
    }
    assert(0==level || qname_level_set->max_level<level);
    return true;
}


bool mceSkipStackPush(mceSkipStack_t *skip_stack, uint32_t level_start, uint32_t level_end, mceSkipState_t state) {
    bool ret=false;
    mceSkipItem_t *new_stack_array=NULL;
    if (NULL!=(new_stack_array=(mceSkipItem_t *)xmlRealloc(skip_stack->stack_array, (1+skip_stack->stack_items)*sizeof(*skip_stack->stack_array)))) {
        skip_stack->stack_array=new_stack_array;
        memset(&skip_stack->stack_array[skip_stack->stack_items], 0, sizeof(skip_stack->stack_array[skip_stack->stack_items]));
        skip_stack->stack_array[skip_stack->stack_items].level_start=level_start;
        skip_stack->stack_array[skip_stack->stack_items].level_end=level_end;
        skip_stack->stack_array[skip_stack->stack_items].state=state;
        skip_stack->stack_items++;
        ret=true;
    }
    return ret;
}

void mceSkipStackPop(mceSkipStack_t *skip_stack) {
    assert(skip_stack->stack_items>0);
    skip_stack->stack_items--;
}

mceSkipItem_t *mceSkipStackTop(mceSkipStack_t *skip_stack) {
    return NULL!=skip_stack->stack_array && skip_stack->stack_items>0 ? &skip_stack->stack_array[skip_stack->stack_items-1] : NULL;
}

bool mceSkipStackSkip(mceSkipStack_t *skip_stack, uint32_t level) {
    return NULL!=skip_stack->stack_array && skip_stack->stack_items>0 
        && level>=skip_stack->stack_array[skip_stack->stack_items-1].level_start 
        && level<skip_stack->stack_array[skip_stack->stack_items-1].level_end;
}

bool mceCtxInit(mceCtx_t *ctx) {
    memset(ctx, 0, sizeof(*ctx));
    mceCtxSuspendProcessing(ctx, BAD_CAST("http://schemas.openxmlformats.org/presentationml/2006/main"), BAD_CAST("extLst"));
    return true;
}

bool mceCtxCleanup(mceCtx_t *ctx) {

    assert(ctx->error!=MCE_ERROR_NONE || 0==ctx->ignorable_set.list_items);
    OPC_ENSURE(mceQNameLevelCleanup(&ctx->ignorable_set, 0));
    OPC_ENSURE(mceQNameLevelCleanup(&ctx->understands_set, 0));
    assert(ctx->error!=MCE_ERROR_NONE || 0==ctx->skip_stack.stack_items);
    while (NULL!=mceSkipStackTop(&ctx->skip_stack)) mceSkipStackPop(&ctx->skip_stack);
    assert(ctx->error!=MCE_ERROR_NONE || 0==ctx->processcontent_set.list_items);
    OPC_ENSURE(mceQNameLevelCleanup(&ctx->processcontent_set, 0));
    OPC_ENSURE(mceQNameLevelCleanup(&ctx->suspended_set, 0));
    assert(ctx->error!=MCE_ERROR_NONE || 0==ctx->suspended_level);
#if (MCE_NAMESPACE_SUBSUMPTION_ENABLED)
    OPC_ENSURE(mceQNameLevelCleanup(&ctx->subsume_namespace_set, 0));
    OPC_ENSURE(mceQNameLevelCleanup(&ctx->subsume_exclude_set, 0));
    OPC_ENSURE(mceQNameLevelCleanup(&ctx->subsume_prefix_set, 0));
#endif
    
    if (NULL!=ctx->ignorable_set.list_array) xmlFree(ctx->ignorable_set.list_array);
    if (NULL!=ctx->understands_set.list_array) xmlFree(ctx->understands_set.list_array);
    if (NULL!=ctx->skip_stack.stack_array) xmlFree(ctx->skip_stack.stack_array);
    if (NULL!=ctx->processcontent_set.list_array) xmlFree(ctx->processcontent_set.list_array);
    if (NULL!=ctx->suspended_set.list_array) xmlFree(ctx->suspended_set.list_array);
#if (MCE_NAMESPACE_SUBSUMPTION_ENABLED)
    if (NULL!=ctx->subsume_namespace_set.list_array) xmlFree(ctx->subsume_namespace_set.list_array);
    if (NULL!=ctx->subsume_exclude_set.list_array) xmlFree(ctx->subsume_exclude_set.list_array);
    if (NULL!=ctx->subsume_prefix_set.list_array) xmlFree(ctx->subsume_prefix_set.list_array);
#endif
    return true;
}

bool mceCtxUnderstandsNamespace(mceCtx_t *ctx, const xmlChar *ns) {
    return mceQNameLevelAdd(&ctx->understands_set, ns, NULL, 0);
}

bool mceCtxSuspendProcessing(mceCtx_t *ctx, const xmlChar *ns, const xmlChar *ln) {
    return mceQNameLevelAdd(&ctx->suspended_set, ns, ln, 0);
}


#if (MCE_NAMESPACE_SUBSUMPTION_ENABLED)
bool mceCtxSubsumeNamespace(mceCtx_t *ctx, const xmlChar *prefix_new, const xmlChar *ns_new, const xmlChar *ns_old) {
    return mceQNameLevelAdd(&ctx->subsume_namespace_set, ns_old, ns_new, 0)
        && mceQNameLevelAdd(&ctx->subsume_prefix_set, ns_new, prefix_new, 0);
}
#endif

