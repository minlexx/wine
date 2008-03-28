/*
 * Copyright 2008 Jacek Caban for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "activscp.h"

#include "wine/debug.h"

#include "mshtml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

static const CLSID CLSID_JScript =
    {0xf414c260,0x6ac0,0x11cf,{0xb6,0xd1,0x00,0xaa,0x00,0xbb,0xbb,0x58}};

typedef struct {
    const IActiveScriptSiteVtbl  *lpActiveScriptSiteVtbl;

    LONG ref;

    IActiveScript *script;

    SCRIPTSTATE script_state;

    HTMLDocument *doc;

    GUID guid;
    struct list entry;
} ScriptHost;

#define ACTSCPSITE(x)  ((IActiveScriptSite*)               &(x)->lpActiveScriptSiteVtbl)

#define ACTSCPSITE_THIS(iface) DEFINE_THIS(ScriptHost, ActiveScriptSite, iface)

static HRESULT WINAPI ActiveScriptSite_QueryInterface(IActiveScriptSite *iface, REFIID riid, void **ppv)
{
    ScriptHost *This = ACTSCPSITE_THIS(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = ACTSCPSITE(This);
    }else if(IsEqualGUID(&IID_IActiveScriptSite, riid)) {
        TRACE("(%p)->(IID_IActiveScriptSite %p)\n", This, ppv);
        *ppv = ACTSCPSITE(This);
    }else {
        FIXME("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI ActiveScriptSite_AddRef(IActiveScriptSite *iface)
{
    ScriptHost *This = ACTSCPSITE_THIS(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI ActiveScriptSite_Release(IActiveScriptSite *iface)
{
    ScriptHost *This = ACTSCPSITE_THIS(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        if(This->doc)
            list_remove(&This->entry);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI ActiveScriptSite_GetLCID(IActiveScriptSite *iface, LCID *plcid)
{
    ScriptHost *This = ACTSCPSITE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, plcid);
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScriptSite_GetItemInfo(IActiveScriptSite *iface, LPCOLESTR pstrName,
        DWORD dwReturnMask, IUnknown **ppiunkItem, ITypeInfo **ppti)
{
    ScriptHost *This = ACTSCPSITE_THIS(iface);
    FIXME("(%p)->(%s %x %p %p)\n", This, debugstr_w(pstrName), dwReturnMask, ppiunkItem, ppti);
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScriptSite_GetDocVersionString(IActiveScriptSite *iface, BSTR *pbstrVersion)
{
    ScriptHost *This = ACTSCPSITE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pbstrVersion);
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScriptSite_OnScriptTerminate(IActiveScriptSite *iface,
        const VARIANT *pvarResult, const EXCEPINFO *pexcepinfo)
{
    ScriptHost *This = ACTSCPSITE_THIS(iface);
    FIXME("(%p)->(%p %p)\n", This, pvarResult, pexcepinfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScriptSite_OnStateChange(IActiveScriptSite *iface, SCRIPTSTATE ssScriptState)
{
    ScriptHost *This = ACTSCPSITE_THIS(iface);

    TRACE("(%p)->(%x)\n", This, ssScriptState);

    This->script_state = ssScriptState;
    return S_OK;
}

static HRESULT WINAPI ActiveScriptSite_OnScriptError(IActiveScriptSite *iface, IActiveScriptError *pscripterror)
{
    ScriptHost *This = ACTSCPSITE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pscripterror);
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScriptSite_OnEnterScript(IActiveScriptSite *iface)
{
    ScriptHost *This = ACTSCPSITE_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScriptSite_OnLeaveScript(IActiveScriptSite *iface)
{
    ScriptHost *This = ACTSCPSITE_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

#undef ACTSCPSITE_THIS

static const IActiveScriptSiteVtbl ActiveScriptSiteVtbl = {
    ActiveScriptSite_QueryInterface,
    ActiveScriptSite_AddRef,
    ActiveScriptSite_Release,
    ActiveScriptSite_GetLCID,
    ActiveScriptSite_GetItemInfo,
    ActiveScriptSite_GetDocVersionString,
    ActiveScriptSite_OnScriptTerminate,
    ActiveScriptSite_OnStateChange,
    ActiveScriptSite_OnScriptError,
    ActiveScriptSite_OnEnterScript,
    ActiveScriptSite_OnLeaveScript
};

static ScriptHost *create_script_host(HTMLDocument *doc, GUID *guid)
{
    ScriptHost *ret;
    HRESULT hres;

    ret = heap_alloc_zero(sizeof(*ret));
    ret->lpActiveScriptSiteVtbl = &ActiveScriptSiteVtbl;
    ret->ref = 1;
    ret->doc = doc;

    ret->guid = *guid;
    list_add_tail(&doc->script_hosts, &ret->entry);

    hres = CoCreateInstance(&ret->guid, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IActiveScript, (void**)&ret->script);
   if(FAILED(hres))
        WARN("Could not load script engine: %08x\n", hres);

    return ret;
}

static BOOL get_guid_from_type(LPCWSTR type, GUID *guid)
{
    const WCHAR text_javascriptW[] =
        {'t','e','x','t','/','j','a','v','a','s','c','r','i','p','t',0};

    /* FIXME: Handle more types */
    if(!strcmpW(type, text_javascriptW)) {
        *guid = CLSID_JScript;
    }else {
        FIXME("Unknown type %s\n", debugstr_w(type));
        return FALSE;
    }

    return TRUE;
}

static BOOL get_guid_from_language(LPCWSTR type, GUID *guid)
{
    HRESULT hres;

    hres = CLSIDFromProgID(type, guid);
    if(FAILED(hres))
        return FALSE;

    /* FIXME: Check CATID_ActiveScriptParse */

    return TRUE;
}

static BOOL get_script_guid(nsIDOMHTMLScriptElement *nsscript, GUID *guid)
{
    nsAString attr_str, val_str;
    BOOL ret = FALSE;
    nsresult nsres;

    static const PRUnichar languageW[] = {'l','a','n','g','u','a','g','e',0};

    nsAString_Init(&val_str, NULL);

    nsres = nsIDOMHTMLScriptElement_GetType(nsscript, &val_str);
    if(NS_SUCCEEDED(nsres)) {
        const PRUnichar *type;

        nsAString_GetData(&val_str, &type);
        if(*type) {
            ret = get_guid_from_type(type, guid);
            nsAString_Finish(&val_str);
            return ret;
        }
    }else {
        ERR("GetType failed: %08x\n", nsres);
    }

    nsAString_Init(&attr_str, languageW);

    nsres = nsIDOMHTMLScriptElement_GetAttribute(nsscript, &attr_str, &val_str);
    if(NS_SUCCEEDED(nsres)) {
        const PRUnichar *language;

        nsAString_GetData(&val_str, &language);

        if(*language) {
            ret = get_guid_from_language(language, guid);
        }else {
            *guid = CLSID_JScript;
            ret = TRUE;
        }
    }else {
        ERR("GetAttribute(language) failed: %08x\n", nsres);
    }

    nsAString_Finish(&attr_str);
    nsAString_Finish(&val_str);

    return ret;
}

static ScriptHost *get_script_host(HTMLDocument *doc, nsIDOMHTMLScriptElement *nsscript)
{
    ScriptHost *iter;
    GUID guid;

    if(!get_script_guid(nsscript, &guid)) {
        WARN("Could not find script GUID\n");
        return NULL;
    }

    if(IsEqualGUID(&CLSID_JScript, &guid)) {
        FIXME("Ignoring JScript\n");
        return NULL;
    }

    LIST_FOR_EACH_ENTRY(iter, &doc->script_hosts, ScriptHost, entry) {
        if(IsEqualGUID(&guid, &iter->guid))
            return iter;
    }

    return create_script_host(doc, &guid);
}

void doc_insert_script(HTMLDocument *doc, nsIDOMHTMLScriptElement *nsscript)
{
    get_script_host(doc, nsscript);
}

void release_script_hosts(HTMLDocument *doc)
{
    ScriptHost *iter;

    while(!list_empty(&doc->script_hosts)) {
        iter = LIST_ENTRY(list_head(&doc->script_hosts), ScriptHost, entry);

        list_remove(&iter->entry);
        iter->doc = NULL;
        IActiveScript_Release(ACTSCPSITE(iter));
    }
}
