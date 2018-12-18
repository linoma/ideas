

#ifndef __GDIOBJECTH__
#define __GDIOBJECTH__


#define GDIOBJECT_TYPE   	        (gdiobject_get_type ())
#define GDIOBJECT(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), GDIOBJECT_TYPE, GdiObject))
#define GDIOBJECT_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GDIOBJECT_TYPE, GdiObjectClass))
#define GDI_IS_OBJECT(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GDIOBJECT_TYPE))
#define GDI_IS_OBJECT_CLASS(klass) 	(G_TYPE_CHECK_CLASS_TYPE ((klass), GDIOBJECT_TYPE))
#define GDIOBJECT_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GDIOBJECT_TYPE, GdiObjectClass))

typedef enum {GDI_SOLIDBRUSH  = 1,GDI_FONT,GDI_PEN};
    
typedef struct _GdiObject GdiObject;
typedef struct _GdiObjectClass GdiObjectClass;

struct _GdiObject {
  	GObject parent;  
	guint type;
	void *object;
};

struct _GdiObjectClass {
  	GObjectClass parent; 
};

GType gdiobject_get_type (void);

GdiObject *gdiobject_new_font();
GdiObject *gdiobject_new_solid_brush(double r,double g,double b);

#endif

