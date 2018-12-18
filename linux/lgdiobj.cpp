#include "ideastypes.h"
#include "lgdiobj.h"

static GType type = 0;
static gpointer parent_class;
//--------------------------------------------------------------------------------
static GObject *gdiobject_constructor(GType type,guint n_construct_properties,GObjectConstructParam *construct_properties)
{
	GObject *obj;
 	GdiObjectClass *klass;
    GObjectClass *parent_class;  
    
	klass = GDIOBJECT_CLASS(g_type_class_peek (GDIOBJECT_TYPE));
    parent_class = G_OBJECT_CLASS(g_type_class_peek_parent (klass));
    obj = parent_class->constructor(type,n_construct_properties,construct_properties);
  	return obj;
}
//--------------------------------------------------------------------------------
static void gdiobject_instance_init(GTypeInstance *instance,gpointer g_class)
{
  	GdiObject *self = (GdiObject *)instance;
}
//--------------------------------------------------------------------------------
static void gdiobject_finalize (GObject *object)
{
	G_OBJECT_CLASS (parent_class)->finalize (object);
}
//--------------------------------------------------------------------------------
static void gdiobject_dispose (GObject *gobject)
{
  	GdiObject *object = (GdiObject *)gobject;

	if(object->object != NULL){
		switch(object->type){
			case GDI_FONT:
				pango_font_description_free((PangoFontDescription *)object->object);				
			break;
			case GDI_SOLIDBRUSH:
				cairo_pattern_destroy((cairo_pattern_t *)object->object);
			break;
		}
		object->object =  NULL;
	}
  	G_OBJECT_CLASS (parent_class)->dispose(gobject);
}
//--------------------------------------------------------------------------------
static void gdiobject_class_init(gpointer g_class,gpointer g_class_data)
{
  	GObjectClass *gobject_class;
  	GdiObjectClass *klass;

	parent_class = g_type_class_peek_parent(g_class);
  	gobject_class = G_OBJECT_CLASS(g_class);
  	klass = GDIOBJECT_CLASS(g_class);
  	gobject_class->constructor = gdiobject_constructor;
  	gobject_class->finalize = gdiobject_finalize;
  	gobject_class->dispose = gdiobject_dispose;
}
//--------------------------------------------------------------------------------
GType gdiobject_get_type(void)
{  	
  	if (type == 0) {
    	static const GTypeInfo info = {
      		sizeof (GdiObjectClass),
      		NULL,
      		NULL,
      		gdiobject_class_init,
      		NULL,
      		NULL,
      		sizeof(GdiObject),
      		0,
      		gdiobject_instance_init
    	};
    	type = g_type_register_static(G_TYPE_OBJECT,"GdiObjectType",&info,(GTypeFlags)0);
  	}
  	return type;
}
//--------------------------------------------------------------------------------
GdiObject *gdiobject_new_solid_brush(double r,double g,double b)
{
	GdiObject *obj;

	obj = (GdiObject *)g_object_new(GDIOBJECT_TYPE, NULL);
    obj->type = GDI_SOLIDBRUSH;
	obj->object = cairo_pattern_create_rgba(r,g,b,1);
	return obj;
}
//--------------------------------------------------------------------------------
GdiObject *gdiobject_new_font()
{
	GdiObject *obj;
    
	obj = (GdiObject *)g_object_new(GDIOBJECT_TYPE, NULL);
    obj->type = GDI_FONT;
	obj->object = pango_font_description_new();
	return obj;
}
