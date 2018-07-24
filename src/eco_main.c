#include "e_mod_main.h"
#include "eco_config.h"
#include <X11/Xlib.h>

#define DROPSHADOW "dropshadow"
#define COMPSCALE "scale"
#define COMPOSITE "comp"

int ecomorph_log;

static void  _e_mod_run_cb(void *data, E_Menu *m, E_Menu_Item *mi);
static void  _e_mod_menu_add(void *data, E_Menu *m);
static Eina_Bool _e_mod_event_del_handler(void *data, int type, void *event);
/*void e_mod_run_ecomp_sh();*/

Ecore_X_Atom ECOMORPH_ATOM_MANAGED = 0;
Ecore_X_Atom ECOMORPH_ATOM_MOVE_RESIZE = 0;
Ecore_X_Atom ECOMORPH_ATOM_PLUGIN = 0;

static E_Module *conf_module = NULL;
static E_Int_Menu_Augmentation *maug = NULL;

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

static E_Config_DD *config_edd = NULL;
Config *config;

static int config_module_enable_get(const char *name)
{
   E_Module *m;
   if(!name) return -1;

   m = e_module_find(name);
   if((m) && (e_module_enabled_get(m)))
      return 1;

   return 0;
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
   config->compscale = config_module_enable_get(COMPSCALE);
   config->composite = config_module_enable_get(COMPOSITE);
   config->base_plugins = eina_stringshare_add("ini inotify");
   e_config_save_queue();
}

Eina_Bool ecomorph_ready(void *data)
{
   if(config->ready)
     {
        /*e_mod_run_ecomp_sh();*/
        e_mod_run_ecomorph();
        return ECORE_CALLBACK_DONE;
     }
   return ECORE_CALLBACK_RENEW;
}

EAPI void *
e_modapi_init(E_Module *m)
{
   char buf[PATH_MAX];
   const char *log_name = eina_stringshare_add("MOD:ECO");

   ecore_exe_run("killall -9 ecomorph", NULL);
   ecomorph_log = eina_log_domain_register(log_name,EINA_COLOR_CYAN);
   eina_log_domain_level_set(log_name, EINA_LOG_LEVEL_DBG);

   /* Location of message catalogs for localization */
   snprintf(buf, sizeof(buf), "%s/locale", e_module_dir_get(m));
   bindtextdomain(PACKAGE, buf);
   bind_textdomain_codeset(PACKAGE, "UTF-8");

   /* Location of theme to load for this module */
   snprintf(buf, sizeof(buf), "%s/e-module-ecomorph.edj", e_module_dir_get(m));

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
   E_CONFIG_VAL(D, T, base_plugins, STR);

   config = e_config_domain_load("module.ecomorph", config_edd);

   if(config)
     {
        config->dropshadow = config_module_enable_get(DROPSHADOW);
        config->compscale = config_module_enable_get(COMPSCALE);
        config->composite = config_module_enable_get(COMPOSITE);
        e_config_save_queue();
     }

   if(!config) _config_new();

   if(config_module_enable_get(DROPSHADOW) == TRUE)
     config_module_unload_set(DROPSHADOW);

   if(config_module_enable_get(COMPSCALE) == TRUE)
     config_module_unload_set(COMPSCALE);

   if(config_module_enable_get(COMPOSITE) == TRUE)
     config_module_unload_set(COMPOSITE);

   config->edje_file = eina_stringshare_add(buf);

   maug = e_int_menus_menu_augmentation_add("config/1", _e_mod_menu_add, NULL, NULL, NULL);
   
   ECOMORPH_ATOM_MANAGED = ecore_x_atom_get("__ECOMORPH_WINDOW_MANAGED");
   ECOMORPH_ATOM_PLUGIN  = ecore_x_atom_get("__ECOMORPH_PLUGIN");

   ecore_x_netwm_window_type_set
     (e_container_current_get(e_manager_current_get())->bg_win,
      ECORE_X_WINDOW_TYPE_DESKTOP);

   INF("Initialized Ecomorph Module");

   conf_module = m;
   
   e_module_priority_set(m,0);
   // set the module delayed by default, so we normally want to have E the most fully-loaded possible before to start the ecomorph layer
   e_module_delayed_set(m, 1);
   
   eco_actions_create();
   eco_event_init();
   
   e_config->desk_flip_animate_mode = -1; // XXX note: if is not set to -1, the switching between desktops would act strangely (windows creating/destroying visually while switching, someones dissappears, etc)
   //
   // Temporal hack: lock fails if ecomorph is running (not in bodhi, so maybe is because the version of PAM)
   // UPdate: this one seems to be fixed by restarting ecomorph when prompting the passwd
   /*e_config->desklock_on_suspend = 0;*/ 
   /*e_config->desklock_start_locked = 0;*/
   /*e_config->desklock_autolock_screensaver = 0;*/
   /*e_config->desklock_autolock_idle = 0;*/

  
   ecore_idler_add(ecomorph_ready, NULL); 

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

   if(config->eeh)
      ecore_event_handler_del(config->eeh);
   if(config->exe)
      ecore_exe_kill(config->exe);

   if(config->cmd)
      eina_stringshare_del(config->cmd);

   eco_actions_free();
   eco_event_shutdown();

   // FIXME: Temporal hack: set statically the desired values... we should remember the user's selections at least
   e_config->desk_flip_animate_mode = 1;
   /*e_config->desklock_on_suspend = 1;*/

   // make sure that ecomorph is fully unloaded before to load another compositor
   while (system("pidof ecomorph 1>/dev/null"))
      usleep(30000);

   if(config->dropshadow == 1)
      ecore_exe_run("sleep 4 ; if enlightenment_remote -module-list | grep -qsi 'ecomorph.*enabled' ; then true ; else enlightenment_remote -module-load dropshadow ; enlightenment_remote -module-enable dropshadow ; fi", NULL);
      /*config_module_load_set(DROPSHADOW);*/

   if(config->compscale == 1)
      ecore_exe_run("sleep 4 ; if enlightenment_remote -module-list | grep -qsi 'ecomorph.*enabled' ; then true ; else enlightenment_remote -module-load scale ; enlightenment_remote -module-enable scale ; fi", NULL);
      /*config_module_load_set(COMPSCALE);*/

   // update: no need to check for composite, we ALWAYS want composite (user should have it in software mode if no acceleration, so...)
   /* Update desktop: needed to not have an empty state desktop */
   ecore_exe_run("sleep 5 ; if enlightenment_remote -module-list | grep -qsi 'ecomorph.*enabled' ; then true ; else enlightenment_remote -module-load comp ; enlightenment_remote -module-enable comp ; fi", NULL);
   /*if(config_module_enable_get(COMPOSITE) == FALSE)*/
      /*config_module_load_set(COMPOSITE);*/

   E_Action *a;
   a = e_action_find("restart");
   if ((a) && (a->func.go))
      ecore_timer_add(0.2, a->func.go, NULL);

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
   e_menu_item_icon_edje_set(mi, config->edje_file, "icon");
   e_menu_item_callback_set(mi, _e_mod_run_cb, NULL);
}

void
e_mod_run_ecomorph()
{
   if(e_mod_has_opengl() == EINA_FALSE) return;

   char buf[PATH_MAX];
   char *ecomorph[] = 
     {
        "/usr/bin/ecomorph",
        "/opt/bin/ecomorph",
        "/usr/local/bin/ecomorph", 
        "/opt/local/bin/ecomorph"
     };
   int i;

   for (i = 0; i < 3; i++)
     {
        if(ecore_file_exists(ecomorph[i]) == EINA_TRUE) 
          {
             if(!config->cmd_options)
                config->cmd_options = eina_stringshare_add("");

               // update: we don't use cmd_plugins and also doesn't seems to be needed (plugins works with the "ini inotify" options only)
             /*if(!config->cmd_plugins_override)*/
                snprintf(buf, sizeof(buf), "%s %s %s", ecomorph[i], config->cmd_options, config->base_plugins);
             /*else*/
                /*snprintf(buf, sizeof(buf), "%s %s %s", ecomorph[i], config->cmd_options, config->cmd_plugins);*/

             config->cmd = eina_stringshare_add(buf);
             break;
          }
     }

   DBG("CMD:%s",buf);

   config->exe = ecore_exe_pipe_run(buf,
                                    ECORE_EXE_PIPE_WRITE |
                                    ECORE_EXE_PIPE_READ_LINE_BUFFERED |
                                    ECORE_EXE_PIPE_ERROR_LINE_BUFFERED,
                                    //ECORE_EXE_PIPE_ERROR |
                                    //  ECORE_EXE_USE_SH, 
                                    //ECORE_EXE_PIPE_READ,
                                    NULL);

   if(config->exe == NULL)
      CRI("Unable to launch: \"%s\"", buf);

   if((config->exe_pid = ecore_exe_pid_get(config->exe)) == -1 )
     {
        CRI("Unable to get pid!");
     }
   else
      DBG("Ecomorph Pid:%d", config->exe_pid);

   config->eeh = ecore_event_handler_add(ECORE_EXE_EVENT_DEL,
                                         _e_mod_event_del_handler, NULL);
   return;
}

static Eina_Bool
_e_mod_event_del_handler(void *data, int type, void *event)
{
   Ecore_Exe_Event_Del *eed;
   const char *cmd;

   if(!(eed = event)) return ECORE_CALLBACK_CANCEL;
   if(!eed->exe) return ECORE_CALLBACK_CANCEL;

   cmd = ecore_exe_cmd_get(eed->exe);
   if(!cmd) return ECORE_CALLBACK_CANCEL;

   if(strcmp(config->cmd, cmd) == 0)
     {
        WRN("ECOMORPH DIED!");

        if (config->exe_pid == eed->pid)
          {
          /*   if(config->cmd_retry >= 3) //FIXME: do this better
               {
                  CRI("died 3 times");
                  ecore_event_handler_del(config->eeh);
                  ecore_exe_free(config->exe);
               }*/
             WRN("This was my child, reloading ecomorph!");

             if(config->eeh)
                ecore_event_handler_del(config->eeh);

             e_mod_run_ecomorph();
             config->cmd_retry++;
          }
     }

   return ECORE_CALLBACK_DONE;
}

/*static Eina_Bool*/
/*_e_mod_event_data_handler_ecomp_sh(void *data, int type, void *event)*/
/*{*/
   /*Ecore_Exe_Event_Data *eed;*/
   /*char **arr;*/
   /*int i;*/

   /*if(!(eed = event)) return ECORE_CALLBACK_CANCEL;*/
   /*if(!eed->exe) return ECORE_CALLBACK_CANCEL;*/

   /*if(eed->size >= (BUFFER_SIZE -1 ))*/
     /*{*/
        /*DBG("Data too big for buffer:%d", eed->size);*/
        /*return ECORE_CALLBACK_DONE;*/
     /*}*/

   /*arr = eina_str_split((const char *)eed->data, "[n]", 0);*/
   
   /*for (i = 0; arr[i]; i++)*/
     /*{*/
        /*if(eina_str_has_prefix(arr[i], "ECOMORPH_OPTIONS:") == EINA_TRUE)*/
          /*{*/
             /*char **options;*/
             /*options = eina_str_split(arr[i], ":", 2);*/
             
             /*if(options[1])*/
                /*config->cmd_options = strdup(options[1]);*/
             
             /*E_FREE(options[0]);*/
             /*E_FREE(options);*/
             /*INF("CMD_OPTIONS:%s", config->cmd_options);*/
          /*}*/
       
       /*if(eina_str_has_prefix(arr[i], "ECOMORPH_PLUGINS:") == EINA_TRUE)*/
         /*{*/
            /*char **plugins;*/
            /*plugins = eina_str_split(arr[i], ":", 2);*/

            /*if(plugins[1])*/
              /*config->cmd_plugins = strdup(plugins[1]);*/
            
            /*E_FREE(plugins[0]);*/
            /*E_FREE(plugins);*/
            /*INF("CMD_PLUGINS:%s", config->cmd_plugins);*/
         /*}*/

       /*if(eina_str_has_prefix(arr[i], "ECOMORPH_PLUGINS_OVERRIDE:") == EINA_TRUE)*/
         /*{*/
            /*char **override;*/
            /*override = eina_str_split(arr[i], ":", 2);*/

            /*if(strncmp(override[1],"yes", 3) == 0)*/
               /*config->cmd_plugins_override = EINA_TRUE;*/
              
            /*E_FREE(override[0]);*/
            /*E_FREE(override);*/
            /*INF("CMD_PLUGINS_OVERRIDE:%d", config->cmd_plugins_override);*/
         /*} */

     /*}*/
   /*E_FREE(arr[0]);*/
   /*E_FREE(arr);*/

   /*return ECORE_CALLBACK_DONE;*/
/*}*/

/*static Eina_Bool*/
/*_e_mod_event_del_handler_ecomp_sh(void *data, int type, void *event)*/
/*{*/
   /*Ecore_Exe_Event_Del *eed;*/
   /*const char *cmd;*/

   /*if(!(eed = event)) return ECORE_CALLBACK_CANCEL;*/
   /*if(!eed->exe) return ECORE_CALLBACK_CANCEL;*/

   /*cmd = ecore_exe_cmd_get(eed->exe);*/
   /*if(!cmd) return ECORE_CALLBACK_CANCEL;*/

   
   /*if(strcmp(config->cmd_sh, cmd) == 0)*/
     /*{*/
        /*WRN("CMD_SH:%s", config->cmd_sh);*/
        /*config->cmd_sh_ended = EINA_TRUE;*/
        /*ecore_event_handler_del(config->eeh);*/

        /*e_mod_run_ecomorph();*/
     /*}*/

   /*return ECORE_CALLBACK_DONE;*/
/*}*/

/*void*/
/*e_mod_run_ecomp_sh()*/
/*{*/
   /*char *ecomp_sh[] = */
     /*{*/
        /*"/usr/bin/ecomp.sh",*/
        /*"/opt/bin/ecomp.sh",*/
        /*"/usr/local/bin/ecomp.sh", */
        /*"/opt/local/bin/ecomp.sh"*/
     /*};*/
   /*int i;*/

   /*for (i = 0; i < 3; i++)*/
      /*if(ecore_file_exists(ecomp_sh[i]) == EINA_TRUE)*/
         /*break;*/

   /*config->cmd_sh = eina_stringshare_add(ecomp_sh[i]);*/

   /*config->exe = ecore_exe_pipe_run(ecomp_sh[i],*/
                                    /*ECORE_EXE_PIPE_WRITE |*/
                                    /*ECORE_EXE_PIPE_READ_LINE_BUFFERED |*/
                                    /*ECORE_EXE_PIPE_ERROR_LINE_BUFFERED |*/
                                    /*ECORE_EXE_PIPE_ERROR |*/
                                    /*ECORE_EXE_USE_SH |*/
                                    /*ECORE_EXE_PIPE_READ,*/
                                    /*NULL);*/

   /*config->eeh = ecore_event_handler_add(ECORE_EXE_EVENT_DATA,*/
                                         /*_e_mod_event_data_handler_ecomp_sh, NULL);*/

   /*config->eeh = ecore_event_handler_add(ECORE_EXE_EVENT_DEL,*/
                                         /*_e_mod_event_del_handler_ecomp_sh, NULL);*/
   /*return;*/
/*}*/

Eina_Bool
_e_mod_xvisual_get(Ecore_X_Display *dpy, int scrnum)
{
   XVisualInfo *visinfo;
   int attribSingle[] = {
        GLX_RGBA,
        GLX_RED_SIZE, 1,
        GLX_GREEN_SIZE, 1,
        GLX_BLUE_SIZE, 1,
        None
    };
    int attribDouble[] = {
        GLX_RGBA,
        GLX_RED_SIZE, 1,
        GLX_GREEN_SIZE, 1,
        GLX_BLUE_SIZE, 1,
        GLX_DOUBLEBUFFER,
        None
    };
    visinfo = glXChooseVisual(dpy, scrnum, attribSingle);

    if (!visinfo)
       visinfo = glXChooseVisual(dpy, scrnum, attribDouble);

#define VisualNoMask 0x0
    if(visinfo == VisualNoMask)
      {
         XFree(visinfo);
         return EINA_FALSE;
      }
#undef VisualNoMask

    if(visinfo)
       XFree(visinfo);

    return EINA_TRUE;
}
    


Eina_Bool 
e_mod_has_opengl()
{
   const char *renderer, *glext, *glext_client, *glext_server;
   int mtx = 0, scrnum;
   Ecore_Evas *ee;
   Ecore_X_Display *dpy;
   Ecore_X_Window_Attributes att;

   if(!ecore_x_composite_query()) return EINA_FALSE;
   if(!ecore_x_damage_query()) return EINA_FALSE;

   memset((&att), 0, sizeof(Ecore_X_Window_Attributes));
   ecore_x_window_attributes_get(ecore_x_window_root_first_get(), &att);
   if((att.depth <= 8)) return EINA_FALSE;

   dpy = ecore_x_display_get();
   scrnum = ecore_x_screen_index_get(ecore_x_default_screen_get());
   
   if(!_e_mod_xvisual_get(dpy, scrnum)) return EINA_FALSE;

   ee = ecore_evas_gl_x11_new(NULL, ecore_x_window_root_first_get(), -64, -64, 32, 32);
   if(ee)
     {
        renderer = eina_stringshare_add(glGetString(GL_RENDERER));
        glext = eina_stringshare_add(glGetString(GL_EXTENSIONS));
        glext_server = eina_stringshare_add(glXQueryServerString(dpy, scrnum, GLX_EXTENSIONS));
        glext_client = eina_stringshare_add(glXGetClientString(dpy, GLX_EXTENSIONS));
        ecore_evas_free(ee);
     }
   else
     {
        ERR("NO OPENGL");
        return EINA_FALSE;
     }

   if (!renderer)
     {
        ERR("Unable to get OpenGL Renderer");
        return EINA_FALSE;
     }

   if (!glext||!glext_server||!glext_client)
     {
        ERR("Unable to get OpenGL Extensions");
        return EINA_FALSE;
     }
  
   DBG("Scrnum:%d", scrnum);
   DBG("Renderer:%s", renderer);
   DBG("OpenGL Extensions:%s", glext);
   DBG("OpenGL Server Extensions: %s", glext_server);
   DBG("OpenGL Client Extensions: %s", glext_client);


   if (strstr((const char *)renderer, "softpipe"))
     {
        CRI("Detected: softpipe");
        return EINA_FALSE;
     }

   if (strstr((const char *)renderer, "llvmpipe"))
     {
        CRI("Detected: llvmpipe");
        return EINA_FALSE;
     }

   if (!strstr((const char *)glext, "GL_ARB_texture_non_power_of_two"))
     {
        ERR("Missing: GL_ARB_texture_non_power_of_two");
        return EINA_FALSE; 
     }

   if (!strstr((const char *)glext, "GL_NV_texture_rectangle"))
     {
        ERR("Missing: GL_NV_texture_rectangle");

        //If Card is not AMD Radeon Return False!
        if (!strstr((const char *)renderer, "AMD Radeon HD"))
          {
             return EINA_FALSE;
          }
     }

   if (!strstr((const char *)glext, "GL_EXT_texture_rectangle"))
     {
        ERR("Missing: GL_EXT_texture_rectangle");
        
        //If Card is not Nvidia Return False!
        if(!strstr((const char *)renderer, "GeForce"))
          {
             return EINA_FALSE;
          }
     }

   if (!strstr((const char *)glext, "GL_ARB_texture_rectangle"))
     {
        ERR("Missing: GL_ARB_texture_rectangle");
        return EINA_FALSE;
     }

   if (!strstr((const char *)glext_server, "GLX_SGIX_fbconfig") || 
       !strstr((const char *)glext_client, "GLX_SGIX_fbconfig") )
     {
        ERR("Missing: GLX_SGIX_fbconfig");
        return EINA_FALSE;
     }

   if (!strstr((const char *)glext_server, "GLX_EXT_texture_from_pixmap") || 
       !strstr((const char *)glext_client, "GLX_EXT_texture_from_pixmap") )
     {
        ERR("Missing: GLX_EXT_texture_from_pixmap");
        return EINA_FALSE;
     }
   
   //Disable ecomorph on AMD E-350 / Radeon HD 6310.
   if (strstr((const char *)renderer, "AMD PALM"))
     return EINA_FALSE;
   
   glGetIntegerv(GL_MAX_TEXTURE_SIZE, &mtx);
   if(mtx > 1)
     {
        E_Manager *man;
        Ecore_X_Randr_Screen_Size size;
        
        man = e_manager_current_get();
        ecore_x_randr_screen_primary_output_current_size_get(man->root,
                           &size.width,&size.height,NULL,NULL,0);
        if((size.width > mtx) || (size.height > mtx))
          {
             ERR("Screen Larger than texture limit!!");
             return EINA_FALSE;
          }
        DBG("Max Texture Size:%d", mtx);
     }

   return EINA_TRUE;
}

/* vim:set ts=8 sw=3 sts=3 expandtab cino=>5n-2f0^-2{2(0W1st0 :*/
