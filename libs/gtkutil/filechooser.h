/*
   Copyright (C) 2001-2006, William Joseph.
   All Rights Reserved.

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

#include <uilib/uilib.h>

#if !defined( INCLUDED_GTKUTIL_FILECHOOSER_H )
#define INCLUDED_GTKUTIL_FILECHOOSER_H

/// \file
/// GTK+ file-chooser dialogs.

const char *file_dialog(ui::Window parent, bool open, const char *title, const char *path = 0, const char *pattern = 0,
                        bool want_load = false, bool want_import = false, bool want_save = false);


/// \brief Prompts the user to browse for a directory.
/// The prompt window will be transient to \p parent.
/// The directory will initially default to \p path, which must be an absolute path.
/// The returned string is allocated with \c g_malloc and must be freed with \c g_free.
char *dir_dialog(ui::Window parent, const char *title = "Choose Directory", const char *path = "");

#endif
