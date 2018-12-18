#include "ideastypes.h"
#include "cell_render_ownerdraw.h"

#define FIXED_WIDTH   100
#define FIXED_HEIGHT  10

#ifndef __WIN32__
//--------------------------------------------------------------------------------
static void cell_renderer_ownerdraw_init(CellRendererOwnerDraw *cellprogress);
static void cell_renderer_ownerdraw_class_init(CellRendererOwnerDrawClass *klass);
static void cell_renderer_ownerdraw_get_property(GObject *object,guint param_id,GValue *value,GParamSpec *pspec);
static void cell_renderer_ownerdraw_set_property(GObject *object,guint param_id,const GValue *value,GParamSpec *pspec);
static void cell_renderer_ownerdraw_finalize(GObject *gobject);
static void cell_renderer_ownerdraw_get_size(GtkCellRenderer *cell,GtkWidget *widget,GdkRectangle *cell_area,gint *x_offset,gint *y_offset,gint *width,gint *height);
static void cell_renderer_ownerdraw_render(GtkCellRenderer *cell,GdkDrawable *window,GtkWidget *widget,GdkRectangle *background_area,GdkRectangle *cell_area,GdkRectangle *expose_area,GtkCellRendererState flags);
//--------------------------------------------------------------------------------
static gpointer parent_class;
static GType cell_ownerdraw_type = 0;
//--------------------------------------------------------------------------------
GType cell_renderer_ownerdraw_get_type (void)
{
    if (cell_ownerdraw_type == 0)
    {
        static const GTypeInfo cell_ownerdraw_info =
        {
            sizeof (CellRendererOwnerDrawClass),NULL,NULL,(GClassInitFunc) cell_renderer_ownerdraw_class_init,
            NULL,NULL,sizeof (CellRendererOwnerDraw),0,(GInstanceInitFunc) cell_renderer_ownerdraw_init,
        };
        cell_ownerdraw_type = g_type_register_static(GTK_TYPE_CELL_RENDERER,"CellRendererOwnerDraw",&cell_ownerdraw_info,(GTypeFlags)0);
    }
    return cell_ownerdraw_type;
}
//--------------------------------------------------------------------------------
static void cell_renderer_ownerdraw_init (CellRendererOwnerDraw *cellrenderer)
{
    GTK_CELL_RENDERER(cellrenderer)->mode = GTK_CELL_RENDERER_MODE_INERT;
    GTK_CELL_RENDERER(cellrenderer)->xpad = 2;
    GTK_CELL_RENDERER(cellrenderer)->ypad = 2;
}
//--------------------------------------------------------------------------------
static void cell_renderer_ownerdraw_class_init(CellRendererOwnerDrawClass *klass)
{
    GtkCellRendererClass *cell_class   = GTK_CELL_RENDERER_CLASS(klass);
    GObjectClass         *object_class = G_OBJECT_CLASS(klass);

    parent_class = g_type_class_peek_parent (klass);
    object_class->finalize = cell_renderer_ownerdraw_finalize;
    object_class->get_property = cell_renderer_ownerdraw_get_property;
    object_class->set_property = cell_renderer_ownerdraw_set_property;
    cell_class->get_size = cell_renderer_ownerdraw_get_size;
    cell_class->render   = cell_renderer_ownerdraw_render;
    g_object_class_install_property(object_class,1,g_param_spec_string("text","Text","",NULL,(GParamFlags)G_PARAM_READWRITE));
    g_object_class_install_property(object_class,2,g_param_spec_int("id","ID","",-1,65535,-1,(GParamFlags)G_PARAM_READWRITE));
}
//--------------------------------------------------------------------------------
static void cell_renderer_ownerdraw_finalize (GObject *object)
{
    (*G_OBJECT_CLASS (parent_class)->finalize) (object);
}
//--------------------------------------------------------------------------------
static void cell_renderer_ownerdraw_get_property (GObject *object,guint param_id,GValue *value,GParamSpec *psec)
{
    CellRendererOwnerDraw *cell = CUSTOM_CELL_RENDERER_OWNERDRAW(object);

    switch (param_id)
    {
        case 2:
			g_value_set_int(value,cell->id);
		break;
		default:
        break;
    }
}
//--------------------------------------------------------------------------------
static void cell_renderer_ownerdraw_set_property (GObject *object,guint param_id,const GValue *value,GParamSpec *pspec)
{
    CellRendererOwnerDraw *cell = CUSTOM_CELL_RENDERER_OWNERDRAW(object);

    switch (param_id)
    {
        case 1:
			g_object_notify(object,"text");
        break;
        case 2:
            cell->id = g_value_get_int(value);
			g_object_notify(object,"id");
        break;
		default:
			cell = NULL;
		break;
    }
}
//--------------------------------------------------------------------------------
GtkCellRenderer *cell_renderer_ownerdraw_new(void)
{
    return (GtkCellRenderer *)g_object_new((GType)CUSTOM_TYPE_CELL_RENDERER_OWNERDRAW, NULL);
}
//--------------------------------------------------------------------------------
static void cell_renderer_ownerdraw_get_size(GtkCellRenderer *cell,GtkWidget *widget,GdkRectangle *cell_area,gint *x_offset,
                                        gint *y_offset,gint *width,gint *height)
{
    gint calc_width,calc_height;

	calc_width  = (gint) cell->xpad * 2 + cell->width;
    calc_height = (gint) cell->ypad * 2 + cell->height;
    if(width)
        *width = calc_width;
    if(height)
        *height = calc_height;
    if(cell_area)
    {
        if(x_offset)
        {
            *x_offset = cell->xalign * (cell_area->width - calc_width);
            *x_offset = MAX (*x_offset, 0);
        }
        if(y_offset)
        {
            *y_offset = cell->yalign * (cell_area->height - calc_height);
            *y_offset = MAX (*y_offset, 0);
        }
    }
}
//--------------------------------------------------------------------------------
static void cell_renderer_ownerdraw_render(GtkCellRenderer *cell_renderer,GdkDrawable *window,GtkWidget *widget,GdkRectangle *background_area,
                                      GdkRectangle *cell_area,GdkRectangle *expose_area,GtkCellRendererState flags)
{
	DRAWITEMSTRUCT dis={0};
	HWND hwnd;
    GtkStateType state;
	CellRendererOwnerDraw *cell;
	gint i;

	cell = CUSTOM_CELL_RENDERER_OWNERDRAW(cell_renderer);
    if(gtk_widget_is_focus(widget))
        dis.itemState = ODS_FOCUS;
    else
        dis.itemState = 0;
	if(flags & GTK_CELL_RENDERER_SELECTED)
		dis.itemState |= ODS_SELECTED;
	dis.itemID = cell->id;
	hwnd = gtk_widget_get_parent(widget);

	dis.rcItem.left = expose_area->x;
	dis.rcItem.top = expose_area->y;
	dis.rcItem.right = expose_area->x + expose_area->width;
	dis.rcItem.bottom = expose_area->y + expose_area->height;
	gtk_widget_style_get(GTK_WIDGET(widget),"vertical-separator", &i,NULL);
	dis.rcItem.top -= i/2;
	dis.rcItem.bottom += i;
	gtk_widget_style_get(GTK_WIDGET(widget),"horizontal-separator", &i,NULL);
	dis.rcItem.left -= i/2;
	dis.rcItem.right += i;

	dis.hDC = GetDC((HWND)window);
	SelectObject(dis.hDC,GetWindowFont(widget));
	SendMessage((HWND)GetWindowLong(widget,GWL_NOTIFYPARENT),WM_DRAWITEM,GetWindowLong(widget,GWL_ID),(LPARAM)&dis);
	ReleaseDC(widget,dis.hDC);
}

#endif
