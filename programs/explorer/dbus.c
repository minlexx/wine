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
#include "wine/port.h"
#include "wine/unicode.h"
#include "wine/library.h"
#include "wine/debug.h"

#include <stdio.h>
#include <windows.h>

#include "explorer_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dbus);



#ifdef SONAME_LIBDBUS_1

#include <dbus/dbus.h>

/* All DBus APIs, used by this module */
#define DBUS_FUNCS \
    DO_FUNC(dbus_bus_get); \
    DO_FUNC(dbus_bus_get_unique_name); \
    DO_FUNC(dbus_bus_release_name); \
    DO_FUNC(dbus_bus_request_name); \
    DO_FUNC(dbus_connection_set_exit_on_disconnect); \
    DO_FUNC(dbus_connection_unref); \
    DO_FUNC(dbus_error_free)

/* pointers to dbus functions */
#define DO_FUNC(f) static typeof(f) * p_##f
DBUS_FUNCS;
#undef DO_FUNC

/* well-known bus name for wine explorer */
#define EXPLORER_DBUS_NAME "org.winehq.shell"

/* global connection object */
DBusConnection *g_dconn = NULL;


static BOOL load_dbus_functions(void)
{
    void *handle;
    char error[128];

    if (!(handle = wine_dlopen(SONAME_LIBDBUS_1, RTLD_NOW, error, sizeof(error))))
        goto failed;

#define DO_FUNC(f) if (!(p_##f = wine_dlsym( handle, #f, error, sizeof(error) ))) goto failed
    DBUS_FUNCS;
#undef DO_FUNC
    WINE_TRACE("Loaded DBus support.\n");
    return TRUE;

failed:
    WINE_WARN("Failed to load DBus support: %s\n", error);
    return FALSE;
}


/**
 * Returns TRUE if dbus was initialized and available.
 */
BOOL initialize_dbus(void)
{
    DBusError derr = DBUS_ERROR_INIT;
    const char *unique_name = NULL;
    int res = 0;
    
    if (!load_dbus_functions()) return FALSE;
    
    g_dconn = p_dbus_bus_get(DBUS_BUS_SESSION, &derr);
    
    if (!g_dconn)
    {
        WINE_WARN("Failed to connect to DBus session bus! Disabling DBus functions.\n");
        WINE_WARN("  DBus error: %s\n", derr.message);
        p_dbus_error_free(&derr);
        return FALSE;
    }
    
    /* we do not want to terminate explorer process on dbus disconnect */
    p_dbus_connection_set_exit_on_disconnect(g_dconn, FALSE);
    
    unique_name = p_dbus_bus_get_unique_name(g_dconn);
    WINE_TRACE("Connected to DBus session bus as unique name: [%s]\n", unique_name);
    
    /* request well-known name on bus - org.winehq.shell */
    res = p_dbus_bus_request_name(g_dconn, EXPLORER_DBUS_NAME,
            DBUS_NAME_FLAG_ALLOW_REPLACEMENT | DBUS_NAME_FLAG_DO_NOT_QUEUE,
            &derr);
    if (res != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER)
    {
        WINE_WARN("Failed to register unique name on session bus: " EXPLORER_DBUS_NAME "\n");
        WINE_WARN("  DBus error: %s\n", derr.message);
        p_dbus_error_free(&derr);
        p_dbus_connection_unref(g_dconn);
        g_dconn = NULL;
        return FALSE;
    }
    
    WINE_TRACE("Registered on DBus as " EXPLORER_DBUS_NAME "\n");
    
    p_dbus_error_free(&derr);
    return TRUE;
}


void disconnect_dbus(void)
{
    DBusError derr = DBUS_ERROR_INIT;
    if (g_dconn != NULL)
    {
        p_dbus_bus_release_name(g_dconn, EXPLORER_DBUS_NAME, &derr);
        p_dbus_connection_unref(g_dconn);
        p_dbus_error_free(&derr);
        g_dconn = NULL;
        
        WINE_TRACE("Disconnected from DBus session bus.\n");
    }
}

#else

/**
 * Returns TRUE if dbus was initialized and available.
 */
BOOL initialize_dbus(void)
{
    WINE_TRACE("Skipping, DBus support not compiled in\n");
    return FALSE;
}

void void disconnect_dbus(void) { }

#endif
