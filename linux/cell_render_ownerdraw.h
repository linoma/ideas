#ifndef __WIN32__

#ifndef __CELL_RENDERER_OWNERDRAWH__
#define __CELL_RENDERER_OWNERDRAWH__

#define CUSTOM_TYPE_CELL_RENDERER_OWNERDRAW              (cell_renderer_ownerdraw_get_type())
#define CUSTOM_CELL_RENDERER_OWNERDRAW(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj),  CUSTOM_TYPE_CELL_RENDERER_OWNERDRAW, CellRendererOwnerDraw))
#define CUSTOM_CELL_RENDERER_OWNERDRAW_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass),  CUSTOM_TYPE_CELL_RENDERER_OWNERDRAW, CellRendererOwnerDrawClass))
#define CUSTOM_IS_CELL_OWNERDRAW_OWNERDRAW(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CUSTOM_TYPE_CELL_RENDERER_OWNERDRAW))
#define CUSTOM_IS_CELL_OWNERDRAW_OWNERDRAW_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass),  CUSTOM_TYPE_CELL_RENDERER_OWNERDRAW))
#define CUSTOM_CELL_RENDERER_OWNERDRAW_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj),  CUSTOM_TYPE_CELL_RENDERER_OWNERDRAW, CellRendererOwnerDrawClass))

typedef struct _CellRendererOwnerDraw CellRendererOwnerDraw;
typedef struct _CellRendererOwnerDrawClass CellRendererOwnerDrawClass;

struct _CellRendererOwnerDraw
{
	GtkCellRenderer parent;
	int id;
};

struct _CellRendererOwnerDrawClass
{
	GtkCellRendererClass  parent_class;
};

GType                cell_renderer_ownerdraw_get_type (void);
GtkCellRenderer     *cell_renderer_ownerdraw_new (void);


#endif

#endif
