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

#endif
