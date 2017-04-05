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

#ifdef SONAME_LIBDBUS_1

#include <dbus/dbus.h>
#include "dbus_common.h"

WINE_DEFAULT_DEBUG_CHANNEL(dbus);


/* this creates response message with simlple string value */
DBusMessage *winedbus_create_introspect_reply(
        DBusMessage *msg,
        const char *reply_xml)
{
    dbus_bool_t retb;
    DBusMessage *reply;
    
    reply = g_fn_dbus_message_new_method_return(msg);
    if (reply)
    {
        retb = g_fn_dbus_message_append_args(reply,
            DBUS_TYPE_STRING, &reply_xml,
            DBUS_TYPE_INVALID);
        if( retb == FALSE )
        {
            g_fn_dbus_message_unref(reply);
            reply = NULL;
        }
    }
    return reply;
}


/* this should reply STRING value */
DBusMessage *winedbus_create_reply_propget_s(DBusMessage *msg, const char *s)
{
    DBusMessage *reply;
    DBusMessageIter iter;
    
    reply = g_fn_dbus_message_new_method_return(msg);
    if (!reply) return NULL;
    
    g_fn_dbus_message_iter_init_append(reply, &iter);
    g_fn_dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &s);
    return reply;
}


/* this should reply DBUS_VARIANT type with internal STRING value */
DBusMessage *winedbus_create_reply_propget_vs(DBusMessage *msg, const char *s)
{
    DBusMessage *reply;
    DBusMessageIter iter, sub_iter;
    
    reply = g_fn_dbus_message_new_method_return(msg);
    if (!reply) return NULL;
    
    /* we can send VARIANT only with iterator */
    g_fn_dbus_message_iter_init_append(reply, &iter);
    g_fn_dbus_message_iter_open_container(&iter, DBUS_TYPE_VARIANT, "s", &sub_iter);
    g_fn_dbus_message_iter_append_basic(&sub_iter, DBUS_TYPE_STRING, &s);
    g_fn_dbus_message_iter_close_container(&iter, &sub_iter);
    return reply;
}

#endif
