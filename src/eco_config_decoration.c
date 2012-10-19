#include "eco_config.h"


/* Apply Function */
static void
_apply(E_Config_Dialog_Data *cfdata)
{
  eco_config_group_apply("decoration");  
}

/* Main creation function */
EAPI void
eco_config_decoration(void *data)
{
  ECO_PAGE_BEGIN("decoration");
  ECO_PAGE_TABLE("Normal Shadow");

  ECO_CREATE_SLIDER_DOUBLE(-1, shadow_radius, "Radius", 0.0, 48.0, "%1.1f", 0, 0);
  ECO_CREATE_SLIDER_DOUBLE(-1, shadow_opacity, "Opacity", 0.0, 1.0, "%1.2f", 0, 1);
  ECO_CREATE_SLIDER_INT(-1,    shadow_x_offset, "Offset X", -16, 16, "%1.0f", 0, 2);
  ECO_CREATE_SLIDER_INT(-1,    shadow_y_offset, "Offset Y", -16, 16, "%1.0f", 0, 3);
  ECO_CREATE_ENTRY(-1, shadow_match, "Match Windows", 0, 4);

  ECO_CREATE_SLIDER_INT(-1,    shadow_color_r, "R", 0, 255, "%1.0f", 0, 5);
  ECO_CREATE_SLIDER_INT(-1,    shadow_color_g, "G", 0, 255, "%1.0f", 0, 6);
  ECO_CREATE_SLIDER_INT(-1,    shadow_color_b, "B", 0, 255, "%1.0f", 0, 7);
  ECO_PAGE_TABLE_END;
  
  ECO_PAGE_TABLE("Second Shadow (e.g. Menus))");
  ECO_CREATE_SLIDER_DOUBLE(-1, menu_shadow_radius, "Radius", 0.0, 48.0, "%1.1f", 0, 0);
  ECO_CREATE_SLIDER_DOUBLE(-1, menu_shadow_opacity, "Opacity", 0.0, 1.0, "%1.2f", 0, 1);
  ECO_CREATE_SLIDER_INT(-1,    menu_shadow_x_offset, "Offset X", -16, 16, "%1.0f", 0, 2);
  ECO_CREATE_SLIDER_INT(-1,    menu_shadow_y_offset, "Offset Y", -16, 16, "%1.0f", 0, 3);
  ECO_CREATE_ENTRY(-1,         menu_shadow_match, "Match Windows", 0, 4);

  ECO_CREATE_SLIDER_INT(-1,    menu_shadow_color_r, "R", 0, 255, "%1.0f", 0, 5);
  ECO_CREATE_SLIDER_INT(-1,    menu_shadow_color_g, "G", 0, 255, "%1.0f", 0, 6);
  ECO_CREATE_SLIDER_INT(-1,    menu_shadow_color_b, "B", 0, 255, "%1.0f", 0, 7);
  ECO_PAGE_TABLE_END;

  ECO_PAGE_END;
}

