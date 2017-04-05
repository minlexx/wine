/*
 * Explorer dbus introspection functions support
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
    const char *mpath, *mintf, *mmemb, *mdest;
    DBusMessage *reply = NULL;
    
    UNREFERENCED_PARAMETER(user_data); /* unused */
    
    /* probably, default fallback message handler should always
     * mark message as handled, otherwise it will cause errors */
    ret = DBUS_HANDLER_RESULT_HANDLED;
    
    /* Get message information */
    mpath = g_fn_dbus_message_get_path(msg);
    mintf = g_fn_dbus_message_get_interface(msg);
    mmemb = g_fn_dbus_message_get_member(msg);
    mdest = g_fn_dbus_message_get_destination(msg);
    /* msig  = g_fn_dbus_message_get_signature(msg); */
    WINE_TRACE("msg for path [%s] dest [%s], intf [%s.%s]\n", mpath, mdest, mintf, mmemb);
    
    /* path [/] dest [org.winehq.shell], intf [org.freedesktop.DBus.Introspectable.Introspect] */
    /* path [/] dest [org.winehq.shell], intf [org.freedesktop.DBus.Properties.Get] */
    
    /* handle only messages for path "/" and "org.winehq.shell" */
    if ((strcmp(mpath, "/") == 0) && (strcmp(mdest, EXPLORER_DBUS_NAME) == 0))
    {
        if ((strcmp(mintf, "org.freedesktop.DBus.Introspectable") == 0) &&
            (strcmp(mmemb, "Introspect") == 0))
        {
            /* root object introspection request */
            reply = winedbus_create_introspect_reply(msg, g_winedbus_introspect_xml_root);
        }
    }
    
    /* common place to send reply and delete it */
    if (reply)
    {
        winedbus_send_safe(reply);
        g_fn_dbus_message_unref(reply);
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
