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
#include <stdio.h>
#include <windows.h>

#ifdef SONAME_LIBDBUS_1

#include "dbus_common.h"

WINE_DEFAULT_DEBUG_CHANNEL(dbus);

/*  DBus error replies  */

dbus_bool_t winedbus_error_reply_failed(DBusMessage *message)
{
    DBusMessage *reply;
    dbus_bool_t ret;
    reply = g_fn_dbus_message_new_error(message, DBUS_ERROR_FAILED, "operation failed");
    if (!reply) return FALSE;
    ret = winedbus_send_safe(reply);
    g_fn_dbus_message_unref(reply);
    return ret;
}


dbus_bool_t winedbus_error_reply_unknown_interface(DBusMessage *message, const char *iface)
{
    char error_message[256] = {0};
    DBusMessage *reply;
    dbus_bool_t ret;
    snprintf(error_message, sizeof(error_message)-1, "Unknown interface: [%s]", iface);
    reply = g_fn_dbus_message_new_error(message, DBUS_ERROR_UNKNOWN_INTERFACE, error_message);
    if (!reply) return FALSE;
    ret = winedbus_send_safe(reply);
    g_fn_dbus_message_unref(reply);
    return ret;
}


dbus_bool_t winedbus_error_reply_unknown_property(DBusMessage *message, const char *prop_name)
{
    DBusMessage *reply;
    dbus_bool_t ret;
    char error_message[256];
    snprintf(error_message, sizeof(error_message)-1, "Unknown property: [%s]", prop_name);
    reply = g_fn_dbus_message_new_error(message, DBUS_ERROR_UNKNOWN_PROPERTY, error_message);
    if (!reply) return FALSE;
    ret = winedbus_send_safe(reply);
    g_fn_dbus_message_unref(reply);
    return ret;
}


dbus_bool_t winedbus_error_reply_read_only_prop(DBusMessage *message, const char *prop_name)
{
    char error_message[256];
    DBusMessage *reply;
    dbus_bool_t ret;
    snprintf(error_message, sizeof(error_message)-1, "property: %s", prop_name);
    reply = g_fn_dbus_message_new_error(message, DBUS_ERROR_PROPERTY_READ_ONLY, error_message);
    if (!reply) return FALSE;
    ret = winedbus_send_safe(reply);
    g_fn_dbus_message_unref(reply);
    return ret;
}


dbus_bool_t winedbus_error_reply_unknown_method(DBusMessage *message, const char *meth_name)
{
    DBusMessage *reply;
    dbus_bool_t ret;
    char error_message[256];
    snprintf(error_message, sizeof(error_message)-1, "Unknown method: [%s]", meth_name);
    reply = g_fn_dbus_message_new_error(message, DBUS_ERROR_UNKNOWN_METHOD, error_message);
    if (!reply) return FALSE;
    ret = winedbus_send_safe(reply);
    g_fn_dbus_message_unref(reply);
    return ret;
}


dbus_bool_t winedbus_error_reply_invalid_args(DBusMessage *message)
{
    DBusMessage *reply;
    dbus_bool_t ret;
    reply = g_fn_dbus_message_new_error(message, DBUS_ERROR_INVALID_ARGS, "Invalid arguments!");
    if (!reply) return FALSE;
    ret = winedbus_send_safe(reply);
    g_fn_dbus_message_unref(reply);
    return ret;
}

#endif
