/*
   Copyright (C) 1999-2006 Id Software, Inc. and contributors.
   For a list of contributors, see the accompanying CONTRIBUTORS file.

   This file is part of GtkRadiant.

   GtkRadiant is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   GtkRadiant is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GtkRadiant; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

/*
   Camera plugin for GtkRadiant
   Copyright (C) 2002 Splash Damage Ltd.
 */

#include "camera.h"

void dialog_button_callback(ui::Widget widget, gpointer data)
{
    int *loop, *ret;

    auto parent = widget.window();
    loop = (int *) g_object_get_data(G_OBJECT(parent), "loop");
    ret = (int *) g_object_get_data(G_OBJECT(parent), "ret");

    *loop = 0;
    *ret = gpointer_to_int(data);
}

gint dialog_delete_callback(GtkWidget *widget, GdkEvent *event, gpointer data)
{
    int *loop;

    gtk_widget_hide(widget);
    loop = (int *) g_object_get_data(G_OBJECT(widget), "loop");
    *loop = 0;

    return TRUE;
}
