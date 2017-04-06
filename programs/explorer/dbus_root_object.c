/*
 * Explorer dbus functions support
 *
 * Copyright 2017 Alexey Minnekhanov
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
#include "wine/debug.h"
#include <windows.h>

#ifdef SONAME_LIBDBUS_1

#include "dbus_common.h"

WINE_DEFAULT_DEBUG_CHANNEL(dbus);


/* root object XML */
static const char *g_winedbus_introspect_xml_root = ""
"<!DOCTYPE node PUBLIC \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\""
" \"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">\n"
"<node name=\"/\">\n"
"  <interface name=\"org.freedesktop.DBus.Introspectable\">\n"
"    <method name=\"Introspect\">\n"
"      <arg name=\"xml_data\" type=\"s\" direction=\"out\"/>\n"
"    </method>\n"
"  </interface>\n"
"  <interface name=\"org.freedesktop.DBus.Properties\">\n"
"    <method name=\"Get\">\n"
"      <arg name=\"interface_name\" type=\"s\" direction=\"in\"/>\n"
"      <arg name=\"property_name\" type=\"s\" direction=\"in\"/>\n"
"      <arg name=\"value\" type=\"v\" direction=\"out\"/>\n"
"    </method>\n"
"    <method name=\"Set\">\n"
"      <arg name=\"interface_name\" type=\"s\" direction=\"in\"/>\n"
"      <arg name=\"property_name\" type=\"s\" direction=\"in\"/>\n"
"      <arg name=\"value\" type=\"v\" direction=\"in\"/>\n"
"    </method>\n"
"    <method name=\"GetAll\">\n"
"      <arg name=\"interface_name\" type=\"s\" direction=\"in\"/>\n"
"      <arg name=\"props\" type=\"{sv}\" direction=\"out\"/>\n"
"    </method>\n"
"  </interface>\n"
"  <interface name=\"" EXPLORER_DBUS_NAME "\">\n"
"    <property name=\"WineVersion\" type=\"s\" access=\"read\"/>\n"
"  </interface>\n"
"</node>";


void winedbus_message_unregistered_fn(DBusConnection *conn, void *user_data)
{
    UNREFERENCED_PARAMETER(conn);      /* unused */
    UNREFERENCED_PARAMETER(user_data); /* unused */
    WINE_TRACE("DBus: object was unregistered\n");
}


static DBusHandlerResult winedbus_rootobj_message_fn(
        DBusConnection *conn,
        DBusMessage *msg,
        void *user_data)
{
    DBusHandlerResult ret;
    DBusMessage *reply = NULL;
    DBusError derr = DBUS_ERROR_INIT;
    dbus_bool_t resb = FALSE;
    const char *mpath, *mintf, *mmemb, *mdest, *msig;
    char *req_intf, *req_prop;
    
    UNREFERENCED_PARAMETER(user_data); /* unused */
    
    /* probably, default fallback message handler should always
     * mark message as handled, otherwise it will cause errors */
    ret = DBUS_HANDLER_RESULT_HANDLED;
    
    /* Get message information */
    mpath = g_fn_dbus_message_get_path(msg);
    mintf = g_fn_dbus_message_get_interface(msg);
    mmemb = g_fn_dbus_message_get_member(msg);
    mdest = g_fn_dbus_message_get_destination(msg);
    msig  = g_fn_dbus_message_get_signature(msg);
    
    /* path [/] dest [org.winehq.shell], intf [org.freedesktop.DBus.Introspectable.Introspect] */
    /* path [/] dest [org.winehq.shell], intf [org.freedesktop.DBus.Properties.Get] */
    
    /* handle only messages for path "/" and "org.winehq.shell" */
    if ((strcmp(mpath, "/") == 0) && (strcmp(mdest, EXPLORER_DBUS_NAME) == 0))
    {
        if (strcmp(mintf, "org.freedesktop.DBus.Introspectable") == 0)
        {
            if (g_fn_dbus_message_is_method_call(msg, mintf, mmemb))
            {
                if (strcmp(mmemb, "Introspect") == 0)
                {
                    /* root object introspection request */
                    reply = winedbus_create_introspect_reply(msg, g_winedbus_introspect_xml_root);
                }
                else
                {
                    /* unknown method */
                    winedbus_error_reply_unknown_method(msg, mmemb);
                }
            }
        }
        else if (strcmp(mintf, "org.freedesktop.DBus.Properties") == 0)
        {
            if (g_fn_dbus_message_is_method_call(msg, mintf, mmemb))
            {
                if (strcmp(mmemb, "Get") == 0)
                {
                    if (strcmp(msig, "ss"))
                    {
                        /* incorrect call signature, must be 2 string arguments */
                        winedbus_error_reply_invalid_args(msg);
                        return ret;
                    }
                    resb = g_fn_dbus_message_get_args(msg, &derr,
                        DBUS_TYPE_STRING, &req_intf,
                        DBUS_TYPE_STRING, &req_prop,
                        DBUS_TYPE_INVALID);
                    if (resb == FALSE)
                    {
                        winedbus_error_reply_invalid_args(msg);
                        return ret;
                    }
                    WINE_TRACE("DBus: call [%s] Properties.Get([%s], [%s])\n",
                            mintf, req_intf, req_prop);
                    
                    if(strcmp(req_prop, "WineVersion") == 0) {
                        HANDLE hntdll = GetModuleHandleA("ntdll.dll");
                        if (hntdll)
                        {
                            static const char * (CDECL *pwine_get_version)(void);
                            pwine_get_version = (void *)GetProcAddress(hntdll, "wine_get_version");
                            if(pwine_get_version) {
                                reply = winedbus_create_reply_propget_s(msg, pwine_get_version());
                            } else {
                                reply = winedbus_create_reply_propget_s(msg, "not wine");
                            }
                        }
                        else
                        {
                            reply = winedbus_create_reply_propget_s(msg, "unknown");
                        }
                    }
                }
                else if (strcmp(mmemb, "Set") == 0)
                {
                    if (strcmp(msig, "ssv"))
                    {
                        winedbus_error_reply_invalid_args(msg);
                        return ret;
                    }

                    resb = g_fn_dbus_message_get_args(msg, &derr,
                        DBUS_TYPE_STRING, &req_intf,
                        DBUS_TYPE_STRING, &req_prop,
                        DBUS_TYPE_INVALID );
                    
                    if (resb == TRUE)
                    {
                        WINE_TRACE("DBus: call [%s] Properties.Set([%s], [%s]): replying: read-only\n",
                                mintf, req_intf, req_prop);
                        winedbus_error_reply_read_only_prop(msg, req_prop);
                    }
                    else
                    {
                        winedbus_error_reply_read_only_prop(msg, "FAILED_TO_GET_PROP_NAME");
                    }
                    return ret;
                }
                else
                {
                    /* unknown method */
                    winedbus_error_reply_unknown_method(msg, mmemb);
                }
            }
        }
        else
        {
            /* unknown interface */
            winedbus_error_reply_unknown_interface(msg, mintf);
        }
    }
    
    /* common place to send reply and delete it */
    if (reply)
    {
        winedbus_send_safe(reply);
        g_fn_dbus_message_unref(reply);
    }
    else
    {
        WINE_TRACE("Unhandled message for path [%s] dest [%s], intf [%s.%s()]\n",
                mpath, mdest, mintf, mmemb);
    }
    
    /* Message handler can return one of these values  from enum DBusHandlerResult:
     * DBUS_HANDLER_RESULT_HANDLED - Message has had its effect,
                                     no need to run more handlers.
     * DBUS_HANDLER_RESULT_NOT_YET_HANDLED - Message has not had any effect,
                                             see if other handlers want it.
     * DBUS_HANDLER_RESULT_NEED_MEMORY - Please try again later with more memory.
     */
    return ret;
}


dbus_bool_t winedbus_register_root_object(DBusConnection *dconn)
{
    DBusObjectPathVTable vtable;
    dbus_bool_t resb = FALSE;
    DBusError derr = DBUS_ERROR_INIT;
    
    ZeroMemory(&vtable, sizeof(vtable));
    vtable.unregister_function = winedbus_message_unregistered_fn;
    vtable.message_function    = winedbus_rootobj_message_fn;
    resb = g_fn_dbus_connection_try_register_fallback(
            dconn,
            "/",           /* object path */
            &vtable,       /* message handler functions */
            NULL,          /* user_data */
            &derr);
    
    if(resb == FALSE)
    {
        WINE_WARN("DBus: ERROR: Failed to register DBus root object "
             "message handler! Error: %s\n", derr.message);
    }
    
    g_fn_dbus_error_free(&derr);
    return resb;
}


#endif
