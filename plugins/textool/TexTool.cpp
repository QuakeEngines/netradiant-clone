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

//-----------------------------------------------------------------------------
//
// DESCRIPTION:
// main plugin implementation
// texturing tools for Radiant
//

#include "StdAfx.h"

static void dialog_button_callback(GtkWidget *widget, gpointer data)
{
    int *loop, *ret;

    auto parent = widget.window();
    loop = (int *) g_object_get_data(G_OBJECT(parent), "loop");
    ret = (int *) g_object_get_data(G_OBJECT(parent), "ret");

    *loop = 0;
    *ret = gpointer_to_int(data);
}

static gint dialog_delete_callback(GtkWidget *widget, GdkEvent *event, gpointer data)
{
    int *loop;

    gtk_widget_hide(widget);
    loop = (int *) g_object_get_data(G_OBJECT(widget), "loop");
    *loop = 0;

    return TRUE;
}

int DoMessageBox(const char *lpText, const char *lpCaption, guint32 uType)
{
    GtkWidget *w, *vbox, *hbox;
    int mode = (uType & MB_TYPEMASK), ret, loop = 1;

    auto window = ui::Window(ui::window_type::TOP);
    window.connect("delete_event",
                   G_CALLBACK(dialog_delete_callback), NULL);
    window.connect("destroy",
                   G_CALLBACK(gtk_widget_destroy), NULL);
    gtk_window_set_title(window, lpCaption);
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);
    g_object_set_data(G_OBJECT(window), "loop", &loop);
    g_object_set_data(G_OBJECT(window), "ret", &ret);
    gtk_widget_realize(window);

    vbox = ui::VBox(FALSE, 10);
    window.add(vbox);
    vbox.show();

    w = ui::Label(lpText);
    vbox.pack_start(w, FALSE, FALSE, 2);
    gtk_label_set_justify(GTK_LABEL(w), GTK_JUSTIFY_LEFT);
    w.show();

    w = gtk_hseparator_new();
    vbox.pack_start(w, FALSE, FALSE, 2);
    w.show();

    hbox = ui::HBox(FALSE, 10);
    vbox.pack_start(hbox, FALSE, FALSE, 2);
    hbox.show();

    if (mode == MB_OK) {
        w = ui::Button("Ok");
        hbox.pack_start(w, TRUE, TRUE, 0);
        w.connect("clicked",
                  G_CALLBACK(dialog_button_callback), GINT_TO_POINTER(IDOK));
        gtk_widget_set_can_default(w, true);
        gtk_widget_grab_default(w);
        w.show();
        ret = IDOK;
    } else if (mode == MB_OKCANCEL) {
        w = ui::Button("Ok");
        hbox.pack_start(w, TRUE, TRUE, 0);
        w.connect("clicked",
                  G_CALLBACK(dialog_button_callback), GINT_TO_POINTER(IDOK));
        gtk_widget_set_can_default(w, true);
        gtk_widget_grab_default(w);
        w.show();

        w = ui::Button("Cancel");
        hbox.pack_start(w, TRUE, TRUE, 0);
        w.connect("clicked",
                  G_CALLBACK(dialog_button_callback), GINT_TO_POINTER(IDCANCEL));
        w.show();
        ret = IDCANCEL;
    } else if (mode == MB_YESNOCANCEL) {
        w = ui::Button("Yes");
        hbox.pack_start(w, TRUE, TRUE, 0);
        w.connect("clicked",
                  G_CALLBACK(dialog_button_callback), GINT_TO_POINTER(IDYES));
        gtk_widget_set_can_default(w, true);
        gtk_widget_grab_default(w);
        w.show();

        w = ui::Button("No");
        hbox.pack_start(w, TRUE, TRUE, 0);
        w.connect("clicked",
                  G_CALLBACK(dialog_button_callback), GINT_TO_POINTER(IDNO));
        w.show();

        w = ui::Button("Cancel");
        hbox.pack_start(w, TRUE, TRUE, 0);
        w.connect("clicked",
                  G_CALLBACK(dialog_button_callback), GINT_TO_POINTER(IDCANCEL));
        w.show();
        ret = IDCANCEL;
    } else /* if (mode == MB_YESNO) */
    {
        w = ui::Button("Yes");
        hbox.pack_start(w, TRUE, TRUE, 0);
        w.connect("clicked",
                  G_CALLBACK(dialog_button_callback), GINT_TO_POINTER(IDYES));
        gtk_widget_set_can_default(w, true);
        gtk_widget_grab_default(w);
        w.show();

        w = ui::Button("No");
        hbox.pack_start(w, TRUE, TRUE, 0);
        w.connect("clicked",
                  G_CALLBACK(dialog_button_callback), GINT_TO_POINTER(IDNO));
        w.show();
        ret = IDNO;
    }

    window.show();
    gtk_grab_add(window);

    while (loop) {
        gtk_main_iteration();
    }

    gtk_grab_remove(window);
    gtk_widget_destroy(window);

    return ret;
}

// Radiant function table
_QERFuncTable_1 g_FuncTable;

// plugin name
const char *PLUGIN_NAME = "Q3 Texture Tools";

// commands in the menu
const char *PLUGIN_COMMANDS = "About...;Go...";

// cast to GtkWidget*
void *g_pMainWnd;
IWindow *g_pToolWnd = NULL; // handle to the window
CWindowListener g_Listen;

// plugin interfaces ---------------------------
bool g_bQglInitDone = false;
OpenGLBinding g_QglTable;
bool g_bSelectedFaceInitDone = false;
_QERSelectedFaceTable g_SelectedFaceTable;
bool g_bUITable = false;
_QERUITable g_UITable;

// selected face -------------------------------
// we use this one to commit / read with Radiant
_QERFaceData g_SelectedFaceData;
// g_pSelectedFaceWindings gets allocated with MAX_POINTS_ON_WINDING at plugin startup ( QERPlug_Init )
winding_t *g_pSelectedFaceWinding = NULL;
const float g_ViewportRatio = 1.2f;
// usefull class to manage the 2D view
C2DView g_2DView;
// control points to move the polygon
CControlPointsManagerBFace g_ControlPointsBFace;
// tells if a face is selected and we have something to render in the TexWindow
bool g_bTexViewReady = false;
// data for texture work
int g_NumPoints;
CtrlPts_t g_WorkWinding;
// reference _QERFaceData we use on Cancel, and for Commit
_QERFaceData g_CancelFaceData;

// patches -------------------------------------
bool g_bPatch = false;
//++timo we use this one to grab selected patchMesh_t
// FIXME: update when there's a real interface to read/write patches
bool g_bSurfaceTableInitDone = false;
_QERAppSurfaceTable g_SurfaceTable;
CControlPointsManagerPatch g_ControlPointsPatch;
// data for texture work
patchMesh_t *g_pPatch;
// we only use ctrl[][].st in this one
patchMesh_t g_WorkPatch;
// copy of initial g_pPatch for Cancel situation
patchMesh_t g_CancelPatch;

// ---------------------------------------------
// holds the manager we are currently using
CControlPointsManager *g_pManager = NULL;

// ---------------------------------------------
// globals flags for user preferences
//++timo TODO: this should be retrieved from the Editor's .INI prefs in a dedicated interface
// update camera view during manipulation ?
bool g_bPrefsUpdateCameraView = true;

// misc ----------------------------------------
bool g_bHelp = false;
//++timo FIXME: used to close the plugin window if InitTexView fails
// it's dirty, only use is to prevent infinite loop in DialogProc
bool g_bClosing = false;

const char *PLUGIN_ABOUT = "Texture Tools for Radiant\n\n"
        "Gtk port by Leonardo Zide (leo@lokigames.com)\n"
        "Original version by Timothee \"TTimo\" Besset (timo@qeradiant.com)";

extern "C" void *WINAPI

QERPlug_GetFuncTable()
{
    return &g_FuncTable;
}

const char *QERPlug_Init(void *hApp, void *pWidget)
{
    int size;
    GtkWidget *pMainWidget = static_cast<GtkWidget *>( pWidget );

    g_pMainWnd = pMainWidget;
    memset(&g_FuncTable, 0, sizeof(_QERFuncTable_1));
    g_FuncTable.m_nSize = sizeof(_QERFuncTable_1);
    size = (int) ((winding_t *) 0)->points[MAX_POINTS_ON_WINDING];
    g_pSelectedFaceWinding = (winding_t *) malloc(size);
    memset(g_pSelectedFaceWinding, 0, size);
    return "Texture tools for Radiant";
}

const char *QERPlug_GetName()
{
    return (char *) PLUGIN_NAME;
}

const char *QERPlug_GetCommandList()
{
    return PLUGIN_COMMANDS;
}

char *TranslateString(char *buf)
{
    static char buf2[32768];
    int i, l;
    char *out;

    l = strlen(buf);
    out = buf2;
    for (i = 0; i < l; i++) {
        if (buf[i] == '\n') {
            *out++ = '\r';
            *out++ = '\n';
        } else {
            *out++ = buf[i];
        }
    }
    *out++ = 0;

    return buf2;
}

// called by InitTexView to fit the view against the bounding box of control points
void FitView(IWindow *hwndDlg, int TexSize[2])
{
    // apply a ratio to get the area we'll draw
    g_2DView.m_Center[0] = 0.5f * (g_2DView.m_Mins[0] + g_2DView.m_Maxs[0]);
    g_2DView.m_Center[1] = 0.5f * (g_2DView.m_Mins[1] + g_2DView.m_Maxs[1]);
    g_2DView.m_Mins[0] = g_2DView.m_Center[0] + g_ViewportRatio * (g_2DView.m_Mins[0] - g_2DView.m_Center[0]);
    g_2DView.m_Mins[1] = g_2DView.m_Center[1] + g_ViewportRatio * (g_2DView.m_Mins[1] - g_2DView.m_Center[1]);
    g_2DView.m_Maxs[0] = g_2DView.m_Center[0] + g_ViewportRatio * (g_2DView.m_Maxs[0] - g_2DView.m_Center[0]);
    g_2DView.m_Maxs[1] = g_2DView.m_Center[1] + g_ViewportRatio * (g_2DView.m_Maxs[1] - g_2DView.m_Center[1]);

    g_2DView.m_rect.left = 0;
    g_2DView.m_rect.top = 0;
    g_2DView.m_rect.bottom = hwndDlg->getHeight();
    g_2DView.m_rect.right = hwndDlg->getWidth();

    // we need to draw this area, now compute a bigger area so the texture scale is the same along X and Y
    // compute box shape in XY space, let's say X <-> S we'll get a ratio for Y:
    if (!g_bPatch) {
        g_SelectedFaceTable.m_pfnGetTextureSize(0, TexSize);
    } else {
        TexSize[0] = g_pPatch->d_texture->width;
        TexSize[1] = g_pPatch->d_texture->height;
    }
    // we want a texture with the same X / Y ratio
    // compute XY space / window size ratio
    float SSize = (float) fabs(g_2DView.m_Maxs[0] - g_2DView.m_Mins[0]);
    float TSize = (float) fabs(g_2DView.m_Maxs[1] - g_2DView.m_Mins[1]);
    float XSize = TexSize[0] * SSize;
    float YSize = TexSize[1] * TSize;
    float RatioX = XSize / (float) abs(g_2DView.m_rect.left - g_2DView.m_rect.right);
    float RatioY = YSize / (float) abs(g_2DView.m_rect.top - g_2DView.m_rect.bottom);
    if (RatioX > RatioY) {
        YSize = (float) abs(g_2DView.m_rect.top - g_2DView.m_rect.bottom) * RatioX;
        TSize = YSize / (float) TexSize[1];
    } else {
        XSize = (float) abs(g_2DView.m_rect.left - g_2DView.m_rect.right) * RatioY;
        SSize = XSize / (float) TexSize[0];
    }
    g_2DView.m_Mins[0] = g_2DView.m_Center[0] - 0.5f * SSize;
    g_2DView.m_Maxs[0] = g_2DView.m_Center[0] + 0.5f * SSize;
    g_2DView.m_Mins[1] = g_2DView.m_Center[1] - 0.5f * TSize;
    g_2DView.m_Maxs[1] = g_2DView.m_Center[1] + 0.5f * TSize;
}

// call this one each time we need to re-init
//++timo TODO: re-init objects state, g_2DView and g_ControlPointsManager
void InitTexView(IWindow *hwndDlg)
{
    // size of the texture we are working on
    int TexSize[2];
    g_bTexViewReady = false;
    if (g_SelectedFaceTable.m_pfnGetSelectedFaceCount() != 0) {
        g_SelectedFaceTable.m_pfnGetFaceInfo(0, &g_SelectedFaceData, g_pSelectedFaceWinding);
        g_bPatch = false;
        int i;
        // we have something selected
        // setup: compute BBox for the winding ( in ST space )
        //++timo FIXME: move this in a C2DView member ? used as well for patches
        g_2DView.m_Mins[0] = +9999.0f;
        g_2DView.m_Mins[1] = +9999.0f;
        g_2DView.m_Maxs[0] = -9999.0f;
        g_2DView.m_Maxs[1] = -9999.0f;
        for (i = 0; i < g_pSelectedFaceWinding->numpoints; i++) {
            if (g_pSelectedFaceWinding->points[i][3] < g_2DView.m_Mins[0]) {
                g_2DView.m_Mins[0] = g_pSelectedFaceWinding->points[i][3];
            }
            if (g_pSelectedFaceWinding->points[i][3] > g_2DView.m_Maxs[0]) {
                g_2DView.m_Maxs[0] = g_pSelectedFaceWinding->points[i][3];
            }
            if (g_pSelectedFaceWinding->points[i][4] < g_2DView.m_Mins[1]) {
                g_2DView.m_Mins[1] = g_pSelectedFaceWinding->points[i][4];
            }
            if (g_pSelectedFaceWinding->points[i][4] > g_2DView.m_Maxs[1]) {
                g_2DView.m_Maxs[1] = g_pSelectedFaceWinding->points[i][4];
            }
        }
        // NOTE: FitView will read and init TexSize
        FitView(hwndDlg, TexSize);
        // now init the work tables
        g_NumPoints = g_pSelectedFaceWinding->numpoints;
        for (i = 0; i < g_NumPoints; i++) {
            g_WorkWinding.data[i][0] = g_pSelectedFaceWinding->points[i][3];
            g_WorkWinding.data[i][1] = g_pSelectedFaceWinding->points[i][4];
        }
        g_ControlPointsBFace.Init(g_NumPoints, &g_WorkWinding, &g_2DView, TexSize, &g_SelectedFaceData, &g_QglTable);
        // init snap-to-grid
        float fTexStep[2];
        fTexStep[0] = 1.0f / float(TexSize[0]);
        fTexStep[1] = 1.0f / float(TexSize[1]);
        g_2DView.SetGrid(fTexStep[0], fTexStep[1]);
        g_pManager = &g_ControlPointsBFace;
        // prepare the "Cancel" data
        memcpy(&g_CancelFaceData, &g_SelectedFaceData, sizeof(_QERFaceData));
        // we are done
        g_bTexViewReady = true;
    } else if (g_SurfaceTable.m_pfnAnyPatchesSelected()) {
        g_pPatch = g_SurfaceTable.m_pfnGetSelectedPatch();
        g_bPatch = true;
        int i, j;
        // compute BBox for all patch points
        g_2DView.m_Mins[0] = +9999.0f;
        g_2DView.m_Mins[1] = +9999.0f;
        g_2DView.m_Maxs[0] = -9999.0f;
        g_2DView.m_Maxs[1] = -9999.0f;
        for (i = 0; i < g_pPatch->width; i++) {
            for (j = 0; j < g_pPatch->height; j++) {
                if (g_pPatch->ctrl[i][j].st[0] < g_2DView.m_Mins[0]) {
                    g_2DView.m_Mins[0] = g_pPatch->ctrl[i][j].st[0];
                }
                if (g_pPatch->ctrl[i][j].st[0] > g_2DView.m_Maxs[0]) {
                    g_2DView.m_Maxs[0] = g_pPatch->ctrl[i][j].st[0];
                }
                if (g_pPatch->ctrl[i][j].st[1] < g_2DView.m_Mins[1]) {
                    g_2DView.m_Mins[1] = g_pPatch->ctrl[i][j].st[1];
                }
                if (g_pPatch->ctrl[i][j].st[1] > g_2DView.m_Maxs[1]) {
                    g_2DView.m_Maxs[1] = g_pPatch->ctrl[i][j].st[1];
                }
            }
        }
        FitView(hwndDlg, TexSize);
        // init the work tables
        g_WorkPatch = *g_pPatch;
        g_ControlPointsPatch.Init(&g_WorkPatch, &g_2DView, &g_QglTable, g_pPatch);
        // init snap-to-grid
        float fTexStep[2];
        fTexStep[0] = 1.0f / float(TexSize[0]);
        fTexStep[1] = 1.0f / float(TexSize[1]);
        g_2DView.SetGrid(fTexStep[0], fTexStep[1]);
        g_pManager = &g_ControlPointsPatch;
        // prepare the "cancel" data
        g_CancelPatch = *g_pPatch;
        // we are done
        g_bTexViewReady = true;
    }
}

void Textool_Validate()
{
    // validate current situation into the main view
    g_pManager->Commit();
    // for a brush face we have an aditionnal step
    if (!g_bPatch) {
        // tell Radiant to update (will also send update windows messages )
        g_SelectedFaceTable.m_pfnSetFaceInfo(0, &g_SelectedFaceData);
    } else {
        // ask to rebuild the patch display data
        g_pPatch->bDirty = true;
        // send a repaint to the camera window as well
        g_FuncTable.m_pfnSysUpdateWindows(W_CAMERA);
    }
    // we'll need to update after that as well:
    g_bTexViewReady = false;
    // send a repaint message
    g_pToolWnd->Redraw();
}

void Textool_Cancel()
{
    if (!g_bPatch) {
        // tell Radiant to update (will also send update windows messages )
        g_SelectedFaceTable.m_pfnSetFaceInfo(0, &g_CancelFaceData);
    } else {
        *g_pPatch = g_CancelPatch;
        g_pPatch->bDirty = true;
        g_FuncTable.m_pfnSysUpdateWindows(W_CAMERA);
    }
    // do not call destroy, decref it
    g_pToolWnd->DecRef();
    g_pToolWnd = NULL;
}

static void DoExpose()
{
    int i, j;

    g_2DView.PreparePaint();
    g_QglTable.m_pfn_qglColor3f(1, 1, 1);
    // draw the texture background
    g_QglTable.m_pfn_qglPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    if (!g_bPatch) {
        g_QglTable.m_pfn_qglBindTexture(GL_TEXTURE_2D, g_SelectedFaceTable.m_pfnGetTextureNumber(0));
    } else {
        g_QglTable.m_pfn_qglBindTexture(GL_TEXTURE_2D, g_pPatch->d_texture->texture_number);
    }

    g_QglTable.m_pfn_qglEnable(GL_TEXTURE_2D);
    g_QglTable.m_pfn_qglBegin(GL_QUADS);
    g_QglTable.m_pfn_qglTexCoord2f(g_2DView.m_Mins[0], g_2DView.m_Mins[1]);
    g_QglTable.m_pfn_qglVertex2f(g_2DView.m_Mins[0], g_2DView.m_Mins[1]);
    g_QglTable.m_pfn_qglTexCoord2f(g_2DView.m_Maxs[0], g_2DView.m_Mins[1]);
    g_QglTable.m_pfn_qglVertex2f(g_2DView.m_Maxs[0], g_2DView.m_Mins[1]);
    g_QglTable.m_pfn_qglTexCoord2f(g_2DView.m_Maxs[0], g_2DView.m_Maxs[1]);
    g_QglTable.m_pfn_qglVertex2f(g_2DView.m_Maxs[0], g_2DView.m_Maxs[1]);
    g_QglTable.m_pfn_qglTexCoord2f(g_2DView.m_Mins[0], g_2DView.m_Maxs[1]);
    g_QglTable.m_pfn_qglVertex2f(g_2DView.m_Mins[0], g_2DView.m_Maxs[1]);
    g_QglTable.m_pfn_qglEnd();
    g_QglTable.m_pfn_qglDisable(GL_TEXTURE_2D);

    if (!g_bPatch) {
        g_QglTable.m_pfn_qglBegin(GL_LINE_LOOP);
        for (i = 0; i < g_NumPoints; i++) {
            g_QglTable.m_pfn_qglVertex2f(g_WorkWinding.data[i][0], g_WorkWinding.data[i][1]);
        }
        g_QglTable.m_pfn_qglEnd();
    } else {
        g_QglTable.m_pfn_qglBegin(GL_LINES);
        for (i = 0; i < g_pPatch->width; i++) {
            for (j = 0; j < g_pPatch->height; j++) {
                if (i < g_pPatch->width - 1) {
                    g_QglTable.m_pfn_qglVertex2f(g_WorkPatch.ctrl[i][j].st[0], g_WorkPatch.ctrl[i][j].st[1]);
                    g_QglTable.m_pfn_qglVertex2f(g_WorkPatch.ctrl[i + 1][j].st[0], g_WorkPatch.ctrl[i + 1][j].st[1]);
                }

                if (j < g_pPatch->height - 1) {
                    g_QglTable.m_pfn_qglVertex2f(g_WorkPatch.ctrl[i][j].st[0], g_WorkPatch.ctrl[i][j].st[1]);
                    g_QglTable.m_pfn_qglVertex2f(g_WorkPatch.ctrl[i][j + 1].st[0], g_WorkPatch.ctrl[i][j + 1].st[1]);
                }
            }
        }
        g_QglTable.m_pfn_qglEnd();
    }

    // let the control points manager render
    g_pManager->render();
}

static bool CanProcess()
{
    if (!g_bTexViewReady && !g_bClosing) {
        InitTexView(g_pToolWnd);

        if (!g_bTexViewReady) {
            g_bClosing = true;
            DoMessageBox("You must have brush primitives activated in your project settings and\n"
                                 "have a patch or a single face selected to use the TexTool plugin.\n"
                                 "See plugins/TexToolHelp for documentation.", "TexTool plugin", MB_ICONERROR | MB_OK);
            // decref, this will destroy
            g_pToolWnd->DecRef();
            g_pToolWnd = NULL;
            return 0;
        } else {
            g_bClosing = false;
        }
    } else if (!g_bTexViewReady && g_bClosing) {
        return 0;
    }

    return 1;
}

#if 0
static void button_press( GtkWidget *widget, GdkEventButton *event, gpointer data ){
    if ( CanProcess() ) {
        switch ( event->button )
        {
        case 1:
            g_pManager->OnLButtonDown( event->x, event->y ); break;
        case 3:
            g_2DView.OnRButtonDown( event->x, event->y ); break;
        }
    }
}

static void button_release( GtkWidget *widget, GdkEventButton *event, gpointer data ){
    if ( CanProcess() ) {
        switch ( event->button )
        {
        case 1:
            g_pManager->OnLButtonUp( event->x, event->y ); break;
        case 3:
            g_2DView.OnRButtonUp( event->x, event->y ); break;
        }
    }
}

static void motion( GtkWidget *widget, GdkEventMotion *event, gpointer data ){
    if ( CanProcess() ) {
        if ( g_2DView.OnMouseMove( event->x, event->y ) ) {
            return;
        }

        if ( g_pManager->OnMouseMove( event->x, event->y ) ) {
            return;
        }
    }
}

static gint expose( GtkWidget *widget, GdkEventExpose *event, gpointer data ){
    if ( event->count > 0 ) {
        return TRUE;
    }

    if ( !CanProcess() ) {
        return TRUE;
    }

    if ( g_bTexViewReady ) {
        g_2DView.m_rect.bottom = widget->allocation.height;
        g_2DView.m_rect.right = widget->allocation.width;

        if ( !g_QglTable.m_pfn_glwidget_make_current( g_pToolWidget ) ) {
            Sys_Printf( "TexTool: glMakeCurrent failed\n" );
            return TRUE;
        }

        DoExpose();

        g_QglTable.m_pfn_glwidget_swap_buffers( g_pToolWidget );
    }

    return TRUE;
}

static gint keypress( GtkWidget* widget, GdkEventKey* event, gpointer data ){
    unsigned int code = gdk_keyval_to_upper( event->keyval );

    if ( code == GDK_Escape ) {
        gtk_widget_destroy( g_pToolWnd );
        g_pToolWnd = NULL;
        return TRUE;
    }

    if ( CanProcess() ) {
        if ( g_2DView.OnKeyDown( code ) ) {
            return FALSE;
        }

        if ( code == GDK_Return ) {
            Textool_Validate();
            return FALSE;
        }
    }

    return TRUE;
}

static gint close( GtkWidget *widget, GdkEvent* event, gpointer data ){
    gtk_widget_destroy( widget );
    g_pToolWnd = NULL;

    return TRUE;
}

static GtkWidget* CreateOpenGLWidget(){
    g_pToolWidget = g_QglTable.m_pfn_glwidget_new( FALSE, g_QglTable.m_pfn_GetQeglobalsGLWidget() );

    gtk_widget_set_events( g_pToolWidget, GDK_DESTROY | GDK_EXPOSURE_MASK | GDK_KEY_PRESS_MASK |
                           GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK );

    // Connect signal handlers
    g_pToolWidget.connect( "expose_event", G_CALLBACK( expose ), NULL );
    g_pToolWidget.connect( "motion_notify_event",
                        G_CALLBACK( motion ), NULL );
    g_pToolWidget.connect( "button_press_event",
                        G_CALLBACK( button_press ), NULL );
    g_pToolWidget.connect( "button_release_event",
                        G_CALLBACK( button_release ), NULL );

    g_pToolWnd.connect( "delete_event", G_CALLBACK( close ), NULL );
    g_pToolWnd.connect( "key_press_event",
                        G_CALLBACK( keypress ), NULL );

    return g_pToolWidget;
}
#endif

#if 0
static void DoPaint(){
    if ( !CanProcess() ) {
        return;
    }

    if ( g_bTexViewReady ) {
        g_2DView.m_rect.bottom = g_pToolWnd->getHeight();
        g_2DView.m_rect.right = g_pToolWnd->getWidth();

        // set GL_PROJECTION
        g_2DView.PreparePaint();
        // render the objects
        // the master is not rendered the same way, draw over a unified texture background
        g_2DView.TextureBackground( g_DrawObjects[0].pObject->getTextureNumber() );
        if ( g_nDrawObjects >= 1 ) {
            int i;
            for ( i = 1; i < g_nDrawObjects; i++ )
            {
                // we use a first step to the GL_MODELVIEW for the master object
                // GL_MODELVIEW will be altered in RenderAuxiliary too
                g_DrawObjects[0].pObject->PrepareModelView( g_DrawObjects[i].pTopo );
                g_DrawObjects[i].pObject->RenderAuxiliary();
            }
        }
        // draw the polygon outline and control points
        g_DrawObjects[0].pObject->PrepareModelView( NULL );
        g_DrawObjects[0].pObject->RenderUI();
    }
}
#endif

bool CWindowListener::OnLButtonDown(guint32 nFlags, double x, double y)
{
    if (CanProcess()) {
        g_pManager->OnLButtonDown((int) x, (int) y);
        return true;
    }
    return false;
}

bool CWindowListener::OnRButtonDown(guint32 nFlags, double x, double y)
{
    if (CanProcess()) {
        g_2DView.OnRButtonDown((int) x, (int) y);
        return true;
    }
    return false;
}

bool CWindowListener::OnLButtonUp(guint32 nFlags, double x, double y)
{
    if (CanProcess()) {
        g_pManager->OnLButtonUp((int) x, (int) y);
        return true;
    }
    return false;
}

bool CWindowListener::OnRButtonUp(guint32 nFlags, double x, double y)
{
    if (CanProcess()) {
        g_2DView.OnRButtonUp((int) x, (int) y);
        return true;
    }
    return false;
}

bool CWindowListener::OnMouseMove(guint32 nFlags, double x, double y)
{
    if (CanProcess()) {
        if (g_2DView.OnMouseMove((int) x, (int) y)) {
            return true;
        }

        g_pManager->OnMouseMove((int) x, (int) y);
        return true;
    }
    return false;
}

// the widget is closing
void CWindowListener::Close()
{
    g_pToolWnd = NULL;
}

bool CWindowListener::Paint()
{
    if (!CanProcess()) {
        return false;
    }

    if (g_bTexViewReady) {
        DoExpose();
    }

    return true;
}

bool CWindowListener::OnKeyPressed(char *s)
{
    if (!strcmp(s, "Escape")) {
        Textool_Cancel();
        return TRUE;
    }
    if (CanProcess()) {
        if (g_2DView.OnKeyDown(s)) {
            return TRUE;
        }

        if (!strcmp(s, "Return")) {
            Textool_Validate();
            return TRUE;
        }
    }
    return FALSE;
}

extern "C" void QERPlug_Dispatch(const char *p, vec3_t vMin, vec3_t vMax, bool bSingleBrush)
{
#if 0
    // if it's the first call, perhaps we need some additional init steps
    if ( !g_bQglInitDone ) {
        g_QglTable.m_nSize = sizeof( OpenGLBinding );
        if ( g_FuncTable.m_pfnRequestInterface( QERQglTable_GUID, static_cast<LPVOID>( &g_QglTable ) ) ) {
            g_bQglInitDone = true;
        }
        else
        {
            Sys_Printf( "TexTool plugin: OpenGLBinding interface request failed\n" );
            return;
        }
    }

    if ( !g_bSelectedFaceInitDone ) {
        g_SelectedFaceTable.m_nSize = sizeof( _QERSelectedFaceTable );
        if ( g_FuncTable.m_pfnRequestInterface( QERSelectedFaceTable_GUID,
                                                static_cast<LPVOID>( &g_SelectedFaceTable ) ) ) {
            g_bSelectedFaceInitDone = true;
        }
        else
        {
            Sys_Printf( "TexTool plugin: _QERSelectedFaceTable interface request failed\n" );
            return;
        }
    }

    if ( !g_bSurfaceTableInitDone ) {
        g_SurfaceTable.m_nSize = sizeof( _QERAppSurfaceTable );
        if ( g_FuncTable.m_pfnRequestInterface( QERAppSurfaceTable_GUID, static_cast<LPVOID>( &g_SurfaceTable ) ) ) {
            g_bSurfaceTableInitDone = true;
        }
        else
        {
            Sys_Printf( "TexTool plugin: _QERAppSurfaceTable interface request failed\n" );
            return;
        }
    }

    if ( !g_bUITable ) {
        g_UITable.m_nSize = sizeof( _QERUITable );
        if ( g_FuncTable.m_pfnRequestInterface( QERUI_GUID, static_cast<LPVOID>( &g_UITable ) ) ) {
            g_bUITable = true;
        }
        else
        {
            Sys_Printf( "TexTool plugin: _QERUITable interface request failed\n" );
            return;
        }
    }
#endif

    if (!strcmp(p, "About...")) {
        DoMessageBox(PLUGIN_ABOUT, "About ...", MB_OK);
    } else if (!strcmp(p, "Go...")) {
        if (!g_pToolWnd) {
            g_pToolWnd = g_UITable.m_pfnCreateGLWindow();
            g_pToolWnd->setSizeParm(300, 300);
            g_pToolWnd->setName("TexTool");
            // g_Listener is a static class, we need to bump the refCount to avoid premature release problems
            g_Listen.IncRef();
            // setListener will incRef on the listener too
            g_pToolWnd->setListener(&g_Listen);
            if (!g_pToolWnd->Show()) {
                DoMessageBox("Error creating texture tools window!", "TexTool plugin", MB_ICONERROR | MB_OK);
                return;
            }
        }

        g_bTexViewReady = false;
        g_bClosing = false;
    } else if (!strcmp(p, "Help...")) {
        if (!g_bHelp) {
            DoMessageBox("Select a brush face (ctrl+shift+left mouse) or a patch, and hit Go...\n"
                                 "See tutorials for more", "TexTool plugin", MB_OK);
        } else {
            DoMessageBox("Are you kidding me ?", "TexTool plugin", MB_OK);
        }
        g_bHelp = true;
    }
}

// =============================================================================
// SYNAPSE

CSynapseServer *g_pSynapseServer = NULL;
CSynapseClientTexTool g_SynapseClient;

extern "C" CSynapseClient *SYNAPSE_DLL_EXPORT

Synapse_EnumerateInterfaces(const char *version, CSynapseServer *pServer)
{
    if (strcmp(version, SYNAPSE_VERSION)) {
        Syn_Printf("ERROR: synapse API version mismatch: should be '"
        SYNAPSE_VERSION
        "', got '%s'\n", version );
        return NULL;
    }
    g_pSynapseServer = pServer;
    g_pSynapseServer->IncRef();
    Set_Syn_Printf(g_pSynapseServer->Get_Syn_Printf());

    g_SynapseClient.AddAPI(PLUGIN_MAJOR, "textool", sizeof(_QERPluginTable));
    g_SynapseClient.AddAPI(RADIANT_MAJOR, NULL, sizeof(g_FuncTable), SYN_REQUIRE, &g_FuncTable);
    g_SynapseClient.AddAPI(QGL_MAJOR, NULL, sizeof(g_QglTable), SYN_REQUIRE, &g_QglTable);
    g_SynapseClient.AddAPI(SELECTEDFACE_MAJOR, NULL, sizeof(g_SelectedFaceTable), SYN_REQUIRE, &g_SelectedFaceTable);

    return &g_SynapseClient;
}

bool CSynapseClientTexTool::RequestAPI(APIDescriptor_t *pAPI)
{
    if (!strcmp(pAPI->major_name, PLUGIN_MAJOR)) {
        _QERPluginTable *pTable = static_cast<_QERPluginTable *>( pAPI->mpTable );
        pTable->m_pfnQERPlug_Init = QERPlug_Init;
        pTable->m_pfnQERPlug_GetName = QERPlug_GetName;
        pTable->m_pfnQERPlug_GetCommandList = QERPlug_GetCommandList;
        pTable->m_pfnQERPlug_Dispatch = QERPlug_Dispatch;
        return true;
    }

    Syn_Printf("ERROR: RequestAPI( '%s' ) not found in '%s'\n", pAPI->major_name, GetInfo());
    return false;
}

#include "version.h"

const char *CSynapseClientTexTool::GetInfo()
{
    return "Texture Tools plugin built " __DATE__ " "
    RADIANT_VERSION;
}
