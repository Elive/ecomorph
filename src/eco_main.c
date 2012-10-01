#include "e_mod_main.h"
#include <X11/Xlib.h>

#define DROPSHADOW "dropshadow"
#define COMPSCALE "scale"
#define COMPOSITE "comp"

static void  _e_mod_run_cb(void *data, E_Menu *m, E_Menu_Item *mi);
static void  _e_mod_menu_add(void *data, E_Menu *m);

Ecore_X_Atom ECOMORPH_ATOM_MANAGED = 0;
Ecore_X_Atom ECOMORPH_ATOM_MOVE_RESIZE = 0;
Ecore_X_Atom ECOMORPH_ATOM_PLUGIN = 0;

static E_Module *conf_module = NULL;
static E_Int_Menu_Augmentation *maug = NULL;

Eet_Data_Descriptor *eco_edd_group, *eco_edd_option;
/* Eet_Data_Descriptor_Class eddc_option;
 * Eet_Data_Descriptor_Class eddc_group; */

/* module setup */
EAPI E_Module_Api e_modapi =
{
   E_MODULE_API_VERSION,
     "Ecomorph"
};

static Eina_Hash*
eet_eina_hash_add(Eina_Hash *hash, const char *key, const void *data)
{
  if (!hash) hash = eina_hash_string_superfast_new(NULL);
  if (!hash) return NULL;

  eina_hash_add(hash, key, data);
  return hash;
}

static int active = 0;
static E_Config_DD *config_edd = NULL;
Config *config;

static int config_module_enable_get(const char *name)
{
   E_Module *m;
   if(!name) return -1;

   m = e_module_find(name);
   if((m) && (e_module_enabled_get(m)))
     {
        return 1;
     }
   else if (!m)
     {
        return 0;
     }
}

static void config_module_unload_set(const char *name)
{
   E_Module *m;
   if(!name) return;

   m = e_module_find(name);
   if((m) && (e_module_enabled_get(m)))
     {
        e_module_disable(m);
     }
}

static void config_module_load_set(const char *name)
{
   E_Module *m;
   if(!name) return;

   m = e_module_find(name);
   if(m)
     {
        e_module_enable(m);
     }
}

static void
_config_new(void)
{
   config = E_NEW(Config, 1);

   config->dropshadow = config_module_enable_get(DROPSHADOW);
   config->compscale = config_module_enable_get(COMPOSITE);
   config->composite = config_module_enable_get(COMPSCALE);
   e_config_save_queue();
}

EAPI void *
e_modapi_init(E_Module *m)
{
   char buf[PATH_MAX];
   
   /* Location of message catalogs for localization */
   snprintf(buf, sizeof(buf), "%s/locale", e_module_dir_get(m));
   bindtextdomain(PACKAGE, buf);
   bind_textdomain_codeset(PACKAGE, "UTF-8");

   /* Location of theme to load for this module */
   snprintf(buf, sizeof(buf), "%s/e-module-ecomorph.edj", e_module_dir_get(m));
   edje_file = eina_stringshare_add(buf);

   e_configure_registry_category_add("appearance", 10, _("Look"),
                                     NULL, "enlightenment/appearance");
   e_configure_registry_item_add("appearance/eco", 50, _("Ecomorph"),
                                 NULL, buf, e_int_config_eco);
#undef T
#undef  D
#define T Config
#define D config_edd
   config_edd = E_CONFIG_DD_NEW("Config", Config);
   E_CONFIG_VAL(D, T, dropshadow, INT);
   E_CONFIG_VAL(D, T, compscale, INT);
   E_CONFIG_VAL(D, T, composite, INT);

   config = e_config_domain_load("module.ecomorph", config_edd);

   if(config)
     {
        config->dropshadow = config_module_enable_get(DROPSHADOW);
        config->compscale = config_module_enable_get(COMPOSITE);
        config->composite = config_module_enable_get(COMPSCALE);
        e_config_save_queue();
     }

   if(!config) _config_new();

   if(config_module_enable_get(DROPSHADOW) == TRUE)
     config_module_unload_set(DROPSHADOW);

   if(config_module_enable_get(COMPOSITE) == TRUE)
     config_module_unload_set(COMPOSITE);

   if(config_module_enable_get(COMPSCALE) == TRUE)
     config_module_unload_set(COMPSCALE);

   maug = e_int_menus_menu_augmentation_add("config/1", _e_mod_menu_add, NULL, NULL, NULL);
    
   eco_edd_group = eet_data_descriptor_new
     ("group", sizeof(Eco_Group),
      NULL, NULL, NULL, NULL,
      (void  (*) (void *, int (*) (void *, const char *, void *, void *), void *))eina_hash_foreach,
      (void *(*) (void *, const char *, void *))eet_eina_hash_add,
      (void  (*) (void *))eina_hash_free);
   
   eco_edd_option = eet_data_descriptor_new
     ("option", sizeof(Eco_Option),
      (void *(*) (void *))eina_list_next,
      (void *(*) (void *, void *)) eina_list_append,
      (void *(*) (void *))eina_list_data_get,
      (void *(*) (void *))eina_list_free,
      NULL, NULL, NULL);

   /* EET_EINA_STREAM_DATA_DESCRIPTOR_CLASS_SET(&eddc_option, Eco_Option);
    * EET_EINA_STREAM_DATA_DESCRIPTOR_CLASS_SET(&eddc_group,  Eco_Group);
    * eco_edd_option = eet_data_descriptor_stream_new(&eddc_option);
    * eco_edd_group =  eet_data_descriptor_stream_new(&eddc_group); */
   
   EET_DATA_DESCRIPTOR_ADD_BASIC(eco_edd_option, Eco_Option, "type",	 type,	      EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(eco_edd_option, Eco_Option, "int",	 intValue,    EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(eco_edd_option, Eco_Option, "double",	 doubleValue, EET_T_DOUBLE);
   EET_DATA_DESCRIPTOR_ADD_BASIC(eco_edd_option, Eco_Option, "string",	 stringValue, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_LIST (eco_edd_option, Eco_Option, "list",	 listValue,   eco_edd_option);

   EET_DATA_DESCRIPTOR_ADD_HASH (eco_edd_group,  Eco_Group,  "options", data, eco_edd_option);

   ECOMORPH_ATOM_MANAGED = ecore_x_atom_get("__ECOMORPH_WINDOW_MANAGED");
   ECOMORPH_ATOM_PLUGIN  = ecore_x_atom_get("__ECOMORPH_PLUGIN");

   ecore_x_netwm_window_type_set
     (e_container_current_get(e_manager_current_get())->bg_win,
      ECORE_X_WINDOW_TYPE_DESKTOP);


   snprintf(buf, sizeof(buf), "%s/%s", e_user_homedir_get(), ".ecomp/run_ecomorph");
   evil = (ecore_file_exists(buf) ? 1 : 0);
   
   if (evil)
     {
       eco_actions_create();
       eco_event_init();
       e_config->desk_flip_animate_mode = -1;
       active = 1;
     }
   
   conf_module = m;
   e_module_delayed_set(m, 0);

   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m)
{
   E_Config_Dialog *cfd;
   while ((cfd = e_config_dialog_get("E", "appearance/eco"))) e_object_del(E_OBJECT(cfd));
   e_configure_registry_item_del("appearance/eco");
   e_configure_registry_category_del("appearance");

   /* remove module-supplied menu additions */
   if (maug)
     {
	e_int_menus_menu_augmentation_del("config/1", maug);
	maug = NULL;
     }

   if (active)
     {
       eco_actions_free();
       eco_event_shutdown();

       e_config->desk_flip_animate_mode = 0;

       /* module is unloaded via gui */
       /* FIXME got to restart e after unloading the module anyway */
       /* if (!stopping)
        * 	 {
        * 	   Eina_List *l;
        * 	   E_Border *bd;
        * 
        * 	   EINA_LIST_FOREACH(e_border_client_list(), l, bd)
        * 	     {
        * 	       bd->changed = 1;
        * 	       bd->changes.pos = 1;
        * 	       bd->fx.x = 0;
        * 	       bd->fx.y = 0;
        * 		
        * 	       ecore_x_window_move(bd->win, bd->x, bd->y);
        * 	     }
        * 	 } */
     }


   if(config->dropshadow == 1)
     config_module_load_set(DROPSHADOW);

   if(config->composite == 1)
     config_module_load_set(COMPOSITE);

   if(config->compscale == 1)
     config_module_load_set(COMPSCALE);
   
   E_FREE(config);
   config = NULL;
   conf_module = NULL;
   return 1;
}

EAPI int
e_modapi_save(E_Module *m)
{
   e_config_domain_save("module.ecomorph", config_edd, config);
   return 1;
}


/* menu item callback(s) */
static void 
_e_mod_run_cb(void *data, E_Menu *m, E_Menu_Item *mi)
{
   e_int_config_eco(e_container_current_get(e_manager_current_get()), NULL);
}

/* menu item add hook */
static void
_e_mod_menu_add(void *data, E_Menu *m)
{
   E_Menu_Item *mi;
   
   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, _("Ecomorph"));
   e_menu_item_icon_edje_set(mi, edje_file, "icon");
   e_menu_item_callback_set(mi, _e_mod_run_cb, NULL);
}

