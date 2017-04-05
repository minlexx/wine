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
#include "dbus_common.h"

/* pointers to dbus functions */
#define DO_FUNC(f) typeof(f) * g_fn_##f
DBUS_FUNCS;
#undef DO_FUNC


/* global connection object */
static DBusConnection *g_dconn = NULL;
/* flag, indicating that dbus message loop thread is running */
BOOL g_dbus_thread_running = FALSE;
/* guard for read/write operations on dbus connection */
CRITICAL_SECTION g_dconn_cs;

/* forward declarations */
static void start_dbus_thread(void);
static void root_message_handler_unreg(DBusConnection *conn, void *user_data);
static DBusHandlerResult root_message_handler(DBusConnection *conn,
        DBusMessage *msg, void *user_data );


/* Loads libdbus-1 DLL and binds to all used dbus functions */
static BOOL load_dbus_functions(void)
{
    void *handle;
    char error[128];

    if (!(handle = wine_dlopen(SONAME_LIBDBUS_1, RTLD_NOW, error, sizeof(error))))
        goto failed;

#define DO_FUNC(f) if (!(g_fn_##f = wine_dlsym( handle, #f, error, sizeof(error) ))) goto failed
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
    dbus_bool_t resb = FALSE;
    DBusObjectPathVTable vtable;
    
    /* 1. Load dbus-1 DLL */
    if (!load_dbus_functions()) return FALSE;
    
    /* 2. Connect to session bus */
    g_dconn = g_fn_dbus_bus_get(DBUS_BUS_SESSION, &derr);
    
    if (!g_dconn)
    {
        WINE_WARN("Failed to connect to DBus session bus! Disabling DBus functions.\n");
        WINE_WARN("  DBus error: %s\n", derr.message);
        g_fn_dbus_error_free(&derr);
        return FALSE;
    }
    
    /* we do not want to terminate explorer process on dbus disconnect */
    g_fn_dbus_connection_set_exit_on_disconnect(g_dconn, FALSE);
    
    unique_name = g_fn_dbus_bus_get_unique_name(g_dconn);
    WINE_TRACE("Connected to DBus session bus as unique name: [%s]\n", unique_name);
    
    /* 3. Request well-known name on bus */
    res = g_fn_dbus_bus_request_name(g_dconn, EXPLORER_DBUS_NAME,
            DBUS_NAME_FLAG_ALLOW_REPLACEMENT | DBUS_NAME_FLAG_DO_NOT_QUEUE,
            &derr);
    if (res != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER)
    {
        WINE_WARN("Failed to register unique name on session bus: " EXPLORER_DBUS_NAME "\n");
        WINE_WARN("  DBus error: %s\n", derr.message);
        g_fn_dbus_error_free(&derr);
        g_fn_dbus_connection_unref(g_dconn);
        g_dconn = NULL;
        return FALSE;
    }
    WINE_TRACE("Registered on DBus as " EXPLORER_DBUS_NAME "\n");
    
    /* 4. Register root object callbacks/vtable on bus and start message loop */
    InitializeCriticalSection(&g_dconn_cs);
    /* Register default message handler for root DBus object "/" */
    ZeroMemory(&vtable, sizeof(vtable));
    vtable.unregister_function = root_message_handler_unreg;
    vtable.message_function    = root_message_handler;
    resb = g_fn_dbus_connection_try_register_fallback(
            g_dconn,
            "/",           /* object path */
            &vtable,       /* message handler functions */
            NULL,          /* user_data */
            &derr);
    if( resb == FALSE )
    {
        WINE_WARN("DBus: ERROR: Failed to register DBus root object "
             "message handler! Error: %s\n", derr.message);
        g_fn_dbus_error_free(&derr);
        g_fn_dbus_connection_unref(g_dconn);
        g_dconn = NULL;
        return FALSE;
    }
    /* start message loop thread */
    start_dbus_thread();
    
    g_fn_dbus_error_free(&derr);
    return TRUE;
}


void disconnect_dbus(void)
{
    DBusError derr = DBUS_ERROR_INIT;
    if (g_dconn != NULL)
    {
        g_fn_dbus_bus_release_name(g_dconn, EXPLORER_DBUS_NAME, &derr);
        g_fn_dbus_connection_unref(g_dconn);
        g_fn_dbus_error_free(&derr);
        g_dconn = NULL;
        
        DeleteCriticalSection(&g_dconn_cs);
        
        WINE_TRACE("Disconnected from DBus session bus.\n");
    }
}


/* Locks connection lock, sends message and unlocks the lock.
 * Safe to use with message loop thread running. */
dbus_bool_t dbus_send_safe(DBusMessage *msg)
{
    dbus_bool_t resb;
    
    if (!g_dconn) return FALSE;
    if (!msg) return FALSE;
    
    EnterCriticalSection(&g_dconn_cs);
    resb = g_fn_dbus_connection_send(g_dconn, msg, NULL);
    LeaveCriticalSection(&g_dconn_cs);
    
    if (resb == FALSE)
    {
        WINE_WARN("dbus_connection_send() failed!\n");
    }
    
    return resb;
}


/* DBus message loop thread function */
/* Infinite loop? Yes! dlls/mountmgr.sys/dbus.c uses similar solution. */
static DWORD WINAPI message_loop_thread(void *arg)
{
    dbus_bool_t retb;
    
    UNREFERENCED_PARAMETER(arg);
    if (!g_dconn) return 1;
    
    retb = TRUE;
    while (retb)
    {
        /* When we call dbus_connection_send() sometimes, this can lead to race 
         * condition. That's why there is a critical section here */
        EnterCriticalSection(&g_dconn_cs);
        /* reads data from socket (wait 100 ms) and calls message handler */
        /* retb will be TRUE if the disconnect message has NOT been processed */
        retb = g_fn_dbus_connection_read_write_dispatch(g_dconn, 100);
        LeaveCriticalSection(&g_dconn_cs);
        /* We need to release lock at least sometimes, that's why 
         * here is Sleep() call here with timeout of 100 ms */
        Sleep(100);
        /* This timeout can be tweaked */
   }
    
    /* This infinite loop can break if there was some DBus error 
     * and/or connection was closed */
    WINE_WARN("DBus: message loop has ended for some reason! Closing connection.\n");
    
    /* if we are here, we are probably not connected already */
    g_fn_dbus_connection_unref(g_dconn);
    g_dconn = NULL;
   
    /* mark as not running */
    g_dbus_thread_running = FALSE;
    return 0;
}


/* Starts dbus message loop thread, if it's not already running */
static void start_dbus_thread(void)
{
    HANDLE handle;
    if (g_dbus_thread_running) return;
    
    handle = CreateThread(NULL, 0, message_loop_thread, NULL, 0, NULL);
    if (handle)
    {
        g_dbus_thread_running = TRUE;
        CloseHandle(handle);
        WINE_TRACE("Started dbus message loop thread.\n");
    }
}


/* Called when a DBusObjectPathVTable is unregistered (or its connection is freed). */
static void root_message_handler_unreg(DBusConnection *conn, void *user_data)
{
    UNREFERENCED_PARAMETER(conn);      /* unused */
    UNREFERENCED_PARAMETER(user_data); /* unused */
    TRACE("DBus: root object was unregistered\n");
}


/* Called when a message is sent to a registered object path. */
static DBusHandlerResult root_message_handler(
        DBusConnection *conn,
        DBusMessage *msg,
        void *user_data)
{
    DBusHandlerResult ret;
    const char *mpath, *mintf, *mmemb, *mdest, *msig;
    
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
    WINE_TRACE("DBus: message for path [%s] dest [%s], intf [%s.%s]\n", mpath, mdest, mintf, mmemb);
    
    /* Message handler can return one of these values  from enum DBusHandlerResult:
     * DBUS_HANDLER_RESULT_HANDLED - Message has had its effect,
                                     no need to run more handlers.
     * DBUS_HANDLER_RESULT_NOT_YET_HANDLED - Message has not had any effect,
                                             see if other handlers want it.
     * DBUS_HANDLER_RESULT_NEED_MEMORY - Please try again later with more memory.
     */
    return ret;
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
