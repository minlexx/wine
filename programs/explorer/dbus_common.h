/*
 * Explorer dbus private definitions
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

#ifndef __WINE_DBUS_COMMON_H
#define __WINE_DBUS_COMMON_H


#ifdef SONAME_LIBDBUS_1
#include <dbus/dbus.h>

/* All DBus APIs, used by this module */
#define DBUS_FUNCS \
    DO_FUNC(dbus_bus_get); \
    DO_FUNC(dbus_bus_get_unique_name); \
    DO_FUNC(dbus_bus_release_name); \
    DO_FUNC(dbus_bus_request_name); \
    DO_FUNC(dbus_connection_read_write_dispatch); \
    DO_FUNC(dbus_connection_send); \
    DO_FUNC(dbus_connection_set_exit_on_disconnect); \
    DO_FUNC(dbus_connection_try_register_fallback); \
    DO_FUNC(dbus_connection_try_register_object_path); \
    DO_FUNC(dbus_connection_unref); \
    DO_FUNC(dbus_error_free); \
    DO_FUNC(dbus_message_append_args); \
    DO_FUNC(dbus_message_get_args); \
    DO_FUNC(dbus_message_get_destination); \
    DO_FUNC(dbus_message_get_interface); \
    DO_FUNC(dbus_message_get_member); \
    DO_FUNC(dbus_message_get_path); \
    DO_FUNC(dbus_message_get_signature); \
    DO_FUNC(dbus_message_is_method_call); \
    DO_FUNC(dbus_message_iter_append_basic); \
    DO_FUNC(dbus_message_iter_init_append); \
    DO_FUNC(dbus_message_iter_close_container); \
    DO_FUNC(dbus_message_iter_open_container); \
    DO_FUNC(dbus_message_new_error); \
    DO_FUNC(dbus_message_new_method_return); \
    DO_FUNC(dbus_message_unref)

/* pointers to dbus functions */
#define DO_FUNC(f) extern typeof(f) * g_fn_##f
DBUS_FUNCS;
#undef DO_FUNC


/* well-known bus name for wine explorer */
#define EXPLORER_DBUS_NAME "org.winehq.shell"


dbus_bool_t winedbus_send_safe(DBusMessage *msg);
dbus_bool_t winedbus_register_object(
        const char *path,
        DBusObjectPathMessageFunction message_handler_fn,
        void *user_data);

/* root object message handlers */
void winedbus_message_unregistered_fn(DBusConnection *conn, void *user_data);
dbus_bool_t winedbus_register_root_object(DBusConnection *dconn);


/* introspection stuff */
DBusMessage *winedbus_create_introspect_reply(DBusMessage *msg, const char *reply_xml);
DBusMessage *winedbus_create_reply_propget_s(DBusMessage *msg, const char *s);
DBusMessage *winedbus_create_reply_propget_vs(DBusMessage *msg, const char *s);

/* error replies */
dbus_bool_t winedbus_error_reply_failed(DBusMessage *message);
dbus_bool_t winedbus_error_reply_unknown_interface(DBusMessage *message, const char *iface);
dbus_bool_t winedbus_error_reply_unknown_property(DBusMessage *message, const char *prop_name);
dbus_bool_t winedbus_error_reply_unknown_method(DBusMessage *message, const char *meth_name);
dbus_bool_t winedbus_error_reply_read_only_prop(DBusMessage *message, const char *prop_name);
dbus_bool_t winedbus_error_reply_invalid_args(DBusMessage *message);


#endif


#endif /* __WINE_DBUS_COMMON_H */
