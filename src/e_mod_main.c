#include <e.h>
#include "e_mod_main.h"
#include "parser.h"
#include "utility.h"
 
#define GOLDEN_RATIO 1.618033989
#define DEFAULT_CITY ""
#define DEFAULT_LANG e_intl_language_get()
#define TIMER_DELAY 2.0

 
EINTERN int _e_forecast_log_dom = -1;

 
/* Gadcon Function Protos */
static E_Gadcon_Client *_gc_init(E_Gadcon *gc, const char *name,
                                 const char *id, const char *style);
static void             _gc_shutdown(E_Gadcon_Client *gcc);
static void             _gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient);
static const char      *_gc_label(const E_Gadcon_Client_Class *client_class __UNUSED__);
static Evas_Object     *_gc_icon(const E_Gadcon_Client_Class *client_class __UNUSED__, Evas *evas);
static const char      *_gc_id_new(const E_Gadcon_Client_Class *client_class __UNUSED__);
 
static E_Config_DD *conf_edd = NULL;
static E_Config_DD *conf_item_edd = NULL;
 
Config *forecasts_config = NULL;
 
/* Define Gadcon Class */
static const E_Gadcon_Client_Class _gadcon_class = {
   GADCON_CLIENT_CLASS_VERSION,
   "forecasts", {_gc_init, _gc_shutdown, _gc_orient, _gc_label, _gc_icon, _gc_id_new, NULL, NULL},
   E_GADCON_CLIENT_STYLE_PLAIN
};
 
/* Module Function Protos */
static void         _cb_fc_mouse_down(void *data,Evas *e __UNUSED__, Evas_Object *obj __UNUSED__,
                         void *event_info __UNUSED__);
static void         _cb_fc_menu_configure(void *data, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__);
static void         _cb_fc_menu_post(void *data, E_Menu *m);
static Eina_Bool    _cb_fc_check(void *data);
static Config_Item *_fc_config_item_get(const char *id);
static Forecasts   *_fc_new(Evas *evas);
static void         _fc_free(Forecasts *w);
static void         _fc_converter(Instance *inst);
static void         _fc_display_set(Instance *inst, Eina_Bool ok __UNUSED__);
static void         _fc_popup_content_create(Instance *inst);
static Eina_Bool    _cb_url_data(void *data, int type __UNUSED__, void *event);
static Eina_Bool    _cb_url_complete(void *data, int type __UNUSED__, void *event);
static void         _cb_mouse_down(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__);
static void         _cb_mouse_in(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__);
static void         _cb_mouse_out(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__);
static Evas_Object *_fc_popup_icon_create(Evas *evas, int code);
static void         _fc_popup_destroy(Instance *inst);
static void         _right_values_update(Instance *inst);
static void         _fc_config_free(void);

/* Gadcon Functions */
static E_Gadcon_Client *
_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style)
{
   Evas_Object *o;
   E_Gadcon_Client *gcc;
   Forecasts *w;
   Instance *inst;
 
   inst = E_NEW(Instance, 1);
 
   inst->ci = _fc_config_item_get(id);
   inst->area = eina_stringshare_add(inst->ci->code);
   inst->language = eina_stringshare_add(inst->ci->lang);
   inst->label = eina_stringshare_add(inst->ci->label);
   inst->buffer = eina_binbuf_new();
 
   w = _fc_new(gc->evas);
   w->inst = inst;
   inst->forecasts = w;

   o = w->forecasts_obj;
   gcc = e_gadcon_client_new(gc, name, id, style, o);
   gcc->data = inst;
   inst->gcc = gcc;
   inst->popup = NULL;
   inst->forecasts_obj = o;
   evas_object_event_callback_add(inst->forecasts_obj, EVAS_CALLBACK_MOUSE_DOWN,
                                  _cb_mouse_down, inst);
   evas_object_event_callback_add(inst->forecasts_obj, EVAS_CALLBACK_MOUSE_IN,
                                  _cb_mouse_in, inst);
   evas_object_event_callback_add(inst->forecasts_obj, EVAS_CALLBACK_MOUSE_OUT,
                                  _cb_mouse_out, inst);
   evas_object_event_callback_add(w->forecasts_obj, EVAS_CALLBACK_MOUSE_DOWN,	
                                  _cb_fc_mouse_down, inst);

   E_LIST_HANDLER_APPEND(inst->handlers, ECORE_CON_EVENT_URL_COMPLETE,
                         _cb_url_complete, inst);
   E_LIST_HANDLER_APPEND(inst->handlers, ECORE_CON_EVENT_URL_DATA,
                         _cb_url_data, inst);

   forecasts_config->instances =
     eina_list_append(forecasts_config->instances, inst);
 
   inst->check_timer = ecore_timer_add(TIMER_DELAY, _cb_fc_check, inst);

   return gcc;
}
 
static void
_gc_shutdown(E_Gadcon_Client *gcc)
{
   Instance *inst;
   Forecasts *w;
 
   inst = gcc->data;
   w = inst->forecasts;
 
   if (inst->popup) _fc_popup_destroy(inst);
   if (inst->check_timer)
     ecore_timer_del(inst->check_timer);

   E_FREE_LIST(inst->handlers, ecore_event_handler_del);
   if (inst->buffer) eina_binbuf_free(inst->buffer);

   if (inst->location)
     eina_stringshare_del(inst->location);
   if (inst->country)
     eina_stringshare_del(inst->country);
   if (inst->area)
     eina_stringshare_del(inst->area);
   if (inst->language)
     eina_stringshare_del(inst->language);
   if (inst->label)
     eina_stringshare_del(inst->label);
 
   forecasts_config->instances =
     eina_list_remove(forecasts_config->instances, inst);
 
   evas_object_event_callback_del(w->forecasts_obj, EVAS_CALLBACK_MOUSE_DOWN,
                                  _cb_fc_mouse_down);
 
   _fc_free(w);
   E_FREE(inst);
}
 
static void
_gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient)
{
   Instance *inst;
 
   inst = gcc->data;
 
   switch (orient)
     {
      case E_GADCON_ORIENT_FLOAT:
        edje_object_signal_emit(inst->forecasts_obj, "e,state,orientation,float", "e");
        e_gadcon_client_aspect_set(gcc, 240, 120);
        e_gadcon_client_min_size_set(gcc, 240, 120);
        break;
 
      default:
        edje_object_signal_emit(inst->forecasts_obj, "e,state,orientation,default", "e");
        e_gadcon_client_aspect_set(gcc, 16, 16);
        e_gadcon_client_min_size_set(gcc, 16, 16);
        break;
     }
}
 
static const char *
_gc_label(const E_Gadcon_Client_Class *client_class __UNUSED__)
{
   return D_("Forecasts");
}
 
static Evas_Object *
_gc_icon(const E_Gadcon_Client_Class *client_class __UNUSED__, Evas *evas)
{
   Evas_Object *o;
   char buf[4096];
 
   o = edje_object_add(evas);
   snprintf(buf, sizeof(buf), "%s/e-module-forecasts.edj",
            e_module_dir_get(forecasts_config->module));
   edje_object_file_set(o, buf, "icon");
   return o;
}
 
static const char *
_gc_id_new(const E_Gadcon_Client_Class *client_class __UNUSED__)
{
   Config_Item *ci;
 
   ci = _fc_config_item_get(NULL);
   return ci->id;
}
 
static void
_cb_fc_mouse_down(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__,
                         void *event_info __UNUSED__)
{
   EINA_SAFETY_ON_NULL_RETURN(data);
   
   Instance *inst = data;
   Evas_Event_Mouse_Down *ev = event_info;
 
   if ((ev->button == 3)) // && (!forecasts_config->menu))  Segfault issue removal
     {
        E_Menu *m;
        E_Menu_Item *mi;
        int x, y, w, h;
 
        m = e_menu_new();
        mi = e_menu_item_new(m);
        e_menu_item_label_set(mi, D_("Settings"));
        e_util_menu_item_theme_icon_set(mi, "preferences-system");
        e_menu_item_callback_set(mi, _cb_fc_menu_configure, inst);
 
        m = e_gadcon_client_util_menu_items_append(inst->gcc, m, 0);
        e_menu_post_deactivate_callback_set(m, _cb_fc_menu_post, inst);
        forecasts_config->menu = m;
 
        e_gadcon_canvas_zone_geometry_get(inst->gcc->gadcon, &x, &y, &w, &h);
        e_menu_activate_mouse(m,
                              e_util_zone_current_get(e_manager_current_get
                                                        ()), x + ev->output.x,
                              y + ev->output.y, 1, 1,
                              E_MENU_POP_DIRECTION_DOWN, ev->timestamp);
        evas_event_feed_mouse_up(inst->gcc->gadcon->evas, ev->button,
                                 EVAS_BUTTON_NONE, ev->timestamp, NULL);
     }
}

static void
_cb_fc_menu_post(void *data, E_Menu *m)
{
   if (!forecasts_config->menu)
     return;
   e_object_del(E_OBJECT(forecasts_config->menu));
   forecasts_config->menu = NULL;
}
 
static void
_cb_fc_menu_configure(void *data, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
   EINA_SAFETY_ON_NULL_RETURN(data);
   
   Instance *inst = data;
 
   if (!forecasts_config) return;
   if (forecasts_config->config_dialog) return;
 
   _config_forecasts_module(inst->ci);
}
 

static Config_Item *
_fc_config_item_get(const char *id)
{
   Eina_List *l;
   Config_Item *ci;
   char buf[128];
   
 
   if (!id)
     {
        int num = 0;
 
        /* Create id */
        if (forecasts_config->items)
          {
             const char *p;
             ci = eina_list_last(forecasts_config->items)->data;
             p = strrchr(ci->id, '.');
             if (p) num = strtol(p + 1, NULL, 10) + 1;
          }
        snprintf(buf, sizeof(buf), "%s.%d", _gadcon_class.name, num);
        id = buf;
     }
   else
     {
        for (l = forecasts_config->items; l; l = l->next)
          {
             ci = l->data;
             if (!ci->id)
               continue;
             if (!strcmp(ci->id, id))
               return ci;
          }
     }
 
   ci = E_NEW(Config_Item, 1);
   ci->id = eina_stringshare_add(id);
   ci->poll_time = 3600.0;
   ci->days = 2.0;
   ci->degrees = DEGREES_C;
   ci->host = eina_stringshare_add("wttr.in");
   ci->code = eina_stringshare_add(DEFAULT_CITY);
   ci->lang = eina_stringshare_add(lang_normalize_str(DEFAULT_LANG));
   ci->label = eina_stringshare_add("");
   ci->show_text = 1;
   ci->popup_on_hover = 1;
 
   forecasts_config->items = eina_list_append(forecasts_config->items, ci);
   return ci;
}
 
/* Gadman Module Setup */
EAPI E_Module_Api e_modapi = {
   E_MODULE_API_VERSION,
   "Forecasts"
};

/* This is called when we need to cleanup the actual configuration,
 * for example when our configuration is too old */
static void
_fc_config_free(void)
{
   EINA_SAFETY_ON_NULL_RETURN(forecasts_config);
   while (forecasts_config->items)
     {
        Config_Item *ci;
 
        ci = forecasts_config->items->data;
        if (ci->id)
          eina_stringshare_del(ci->id);
        if (ci->host)
          eina_stringshare_del(ci->host);
        if (ci->code)
          eina_stringshare_del(ci->code);
        if (ci->lang)
          eina_stringshare_del(ci->lang);
        if(ci->label)
          eina_stringshare_del(ci->label);
        forecasts_config->items =
          eina_list_remove_list(forecasts_config->items, forecasts_config->items);
        free(ci);
        ci = NULL;
     }
     E_FREE(forecasts_config);
}
 
EAPI void *
e_modapi_init(E_Module *m)
{
   char buf[4095];
 
   snprintf(buf, sizeof(buf), "%s/locale", e_module_dir_get(m));
   bindtextdomain(PACKAGE, buf);
   bind_textdomain_codeset(PACKAGE, "UTF-8");
 
   conf_item_edd = E_CONFIG_DD_NEW("Forecasts_Config_Item", Config_Item);
#undef T
#undef D
#define T Config_Item
#define D conf_item_edd
   E_CONFIG_VAL(D, T, id, STR);
   E_CONFIG_VAL(D, T, poll_time, DOUBLE);
   E_CONFIG_VAL(D, T, days, DOUBLE);
   E_CONFIG_VAL(D, T, degrees, INT);
   E_CONFIG_VAL(D, T, host, STR);
   E_CONFIG_VAL(D, T, code, STR);
   E_CONFIG_VAL(D, T, lang, STR);
   E_CONFIG_VAL(D, T, label, STR);
   E_CONFIG_VAL(D, T, show_text, INT);
   E_CONFIG_VAL(D, T, popup_on_hover, INT);
 
   conf_edd = E_CONFIG_DD_NEW("Forecasts_Config", Config);
#undef T
#undef D
#define T Config
#define D conf_edd
   E_CONFIG_LIST(D, T, items, conf_item_edd);
   E_CONFIG_VAL(D, T, version, INT);
 
   forecasts_config = e_config_domain_load("module.forecasts", conf_edd);
   
   if (forecasts_config) {
     /* Check config version */
     if (!e_util_module_config_check("Forecasts", forecasts_config->version, MOD_CONFIG_FILE_VERSION))
       _fc_config_free();
   }
   
   if (!forecasts_config)
     {
        Config_Item *ci;
 
        forecasts_config = E_NEW(Config, 1);
        ci = E_NEW(Config_Item, 1);
        ci->id = eina_stringshare_add("0");
        ci->poll_time = 3600.0;
        ci->days = 2.0;
        ci->degrees = DEGREES_C;
        ci->host = eina_stringshare_add("wttr.in");
        ci->code = eina_stringshare_add(DEFAULT_CITY);
        ci->lang = eina_stringshare_add(lang_normalize_str(DEFAULT_LANG));
        ci->label = eina_stringshare_add("");
        ci->show_text = 1;
        ci->popup_on_hover = 1;
        
        forecasts_config->module = m;
        forecasts_config->items = eina_list_append(forecasts_config->items, ci);
        forecasts_config->version = MOD_CONFIG_FILE_VERSION;
        /* save the config to disk */
        e_config_save_queue();
     }
    forecasts_config->module = m; // FIXME: This should not need set twice wtf
    
   _e_forecast_log_dom = eina_log_domain_register("Forecast", EINA_COLOR_ORANGE);
   eina_log_domain_level_set("Forecast", EINA_LOG_LEVEL_DBG);
   ecore_con_init();

   e_gadcon_provider_register(&_gadcon_class);

   return m;
}
 
EAPI int
e_modapi_shutdown(E_Module *m __UNUSED__)
{
   forecasts_config->module = NULL;
   e_gadcon_provider_unregister(&_gadcon_class);
 
   if (forecasts_config->config_dialog)
     e_object_del(E_OBJECT(forecasts_config->config_dialog));
   /*  Segfault issue removal
    * if (forecasts_config->menu)
    *{
    *    e_menu_post_deactivate_callback_set(forecasts_config->menu, NULL, NULL);
    *    e_object_del(E_OBJECT(forecasts_config->menu));
    *    forecasts_config->menu = NULL;
    *} */
 
   _fc_config_free();
 
   E_CONFIG_DD_FREE(conf_item_edd);
   E_CONFIG_DD_FREE(conf_edd);
   
    eina_log_domain_unregister(_e_forecast_log_dom);
   _e_forecast_log_dom = -1;
   return 1;
}
 
EAPI int
e_modapi_save(E_Module *m __UNUSED__)
{
   e_config_domain_save("module.forecasts", conf_edd, forecasts_config);
   return 1;
}
 
static Forecasts *
_fc_new(Evas *evas)
{
   Forecasts *w;
   char buf[4096];
 
   w = E_NEW(Forecasts, 1);
 
   w->forecasts_obj = edje_object_add(evas);
 
   snprintf(buf, sizeof(buf), "%s/forecasts.edj",
            e_module_dir_get(forecasts_config->module));
   if (!e_theme_edje_object_set(w->forecasts_obj, "base/theme/modules/forecasts",
                                "modules/forecasts/main"))
     edje_object_file_set(w->forecasts_obj, buf, "modules/forecasts/main");
   evas_object_show(w->forecasts_obj);
 
   w->icon_obj = edje_object_add(evas);
   if (!e_theme_edje_object_set(w->icon_obj, "base/theme/modules/forecasts/icons",
                                "modules/forecasts/icons/3200"))
     edje_object_file_set(w->icon_obj, buf, "modules/forecasts/icons/3200");
   edje_object_part_swallow(w->forecasts_obj, "icon", w->icon_obj);
 
   return w;
}
 
static void
_fc_free(Forecasts *w)
{
   char name[60];
   int i;
 
   for (i = 0; i < 2; i++)
     {
        Evas_Object *swallow;
 
        snprintf(name, sizeof(name), "e.swallow.day%d.icon", i);
        swallow = edje_object_part_swallow_get(w->forecasts_obj, name);
        if (swallow)
          evas_object_del(swallow);
     }
   evas_object_del(w->forecasts_obj);
   evas_object_del(w->icon_obj);
   free(w);
   w = NULL;
}

static Eina_Bool
_cb_fc_check(void *data)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(data, EINA_FALSE);

   Instance *inst = data;
   Ecore_Con_Url *url_con;
   char url[1114];
   char forecast[1024];
   char lang_buf[256] = "";
   char *temp = NULL;

   if (eina_dbl_exact(ecore_timer_interval_get(inst->check_timer), TIMER_DELAY))
      ecore_timer_interval_set(inst->check_timer, inst->ci->poll_time);
   temp = url_normalize_str(inst->ci->code);

   if ((inst->ci->lang[0]) != '\0') snprintf(lang_buf, 256, "%s.", inst->ci->lang);
   snprintf(forecast, sizeof(forecast), "%s?format=j1", temp);
   snprintf(url, sizeof(url), "http://%s%s/%s",
             lang_buf, inst->ci->host, forecast);
   free(temp);

   url_con = ecore_con_url_new(url);
   if (!url_con) WRN("error when creating ecore con url object.\n");

   ecore_con_url_data_set(url_con, inst);
    if (!ecore_con_url_get(url_con))
     {
        WRN("Could not realize url request.\n");
        goto free_url_con;
     }
   return ECORE_CALLBACK_RENEW;

   free_url_con:
     ecore_con_url_free(url_con);
     return ECORE_CALLBACK_RENEW;
}


static Eina_Bool
_cb_url_data(void *data, int type __UNUSED__, void *event)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(data, ECORE_CALLBACK_PASS_ON);
   
   Ecore_Con_Event_Url_Data *ev = event;
   Instance *inst = data;

   if (data != ecore_con_url_data_get(ev->url_con)) return ECORE_CALLBACK_PASS_ON;

   if (ev->size > 0)
     eina_binbuf_append_length(inst->buffer, ev->data, ev->size);


   return ECORE_CALLBACK_DONE;
}


static Eina_Bool
_cb_url_complete(void *data, int type __UNUSED__, void *event)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(data, ECORE_CALLBACK_PASS_ON);
   
   Ecore_Con_Event_Url_Complete *ev = event;
   Instance *inst = data;
   Eina_Bool ret;
   
   if (data != ecore_con_url_data_get(ev->url_con)) return ECORE_CALLBACK_PASS_ON;
   if (ev->status != 200) return ECORE_CALLBACK_DONE;

   ret = fc_parse_json(inst);
   if (ret)
   {
      _fc_converter(inst);
      _fc_display_set(inst, ret);
   }
   eina_binbuf_reset(inst->buffer);
   
   return ECORE_CALLBACK_DONE;
}

void
_fc_converter(Instance *inst)
{
   EINA_SAFETY_ON_NULL_RETURN(inst);
 
   int i;
 
   if (inst->ci->degrees == DEGREES_C)
     {
        inst->units.temp = 'C';
        snprintf(inst->units.distance, 3, "km");
        snprintf(inst->units.pressure, 3, "mb");
        snprintf(inst->units.speed, 5, "km/h");
       
        inst->condition.temp = inst->condition.temp_c;
        inst->condition.feel = inst->condition.feel_c;
        inst->details.wind.speed = inst->details.wind.speed_km;
        inst->details.wind.chill = inst->details.wind.chill_c;
        inst->details.atmosphere.visibility = inst->details.atmosphere.visibility_km;
        inst->details.atmosphere.pressure = inst->details.atmosphere.pressure_km;
       
        for (i = 0; i <= 2 ; i++)
         {
            inst->forecast[i].low = inst->forecast[i].low_c;
            inst->forecast[i].high = inst->forecast[i].high_c;
          }
     }
    else
     {
        inst->units.temp = 'F';
        snprintf(inst->units.distance, 3, "mi");
        snprintf(inst->units.pressure, 3, "in");
        snprintf(inst->units.speed, 4, "mph");
       
        inst->condition.temp = inst->condition.temp_f;
        inst->condition.feel = inst->condition.feel_f;
        inst->details.wind.speed = inst->details.wind.speed_mi;
        inst->details.wind.chill = inst->details.wind.chill_f;
        inst->details.atmosphere.visibility = inst->details.atmosphere.visibility_mi;
        inst->details.atmosphere.pressure = inst->details.atmosphere.pressure_mi;
        
        for (i = 0; i <= 2; i++)
         {
            inst->forecast[i].low = inst->forecast[i].low_f;
            inst->forecast[i].high = inst->forecast[i].high_f;
          }
     }
}

static void
_right_values_update(Instance *inst)
{
   char buf[4096], name[60];
   int i;
   for (i = 0; i < 2; i++)
     {
        Evas_Object *swallow;
        snprintf(name, sizeof(name), "e.text.day%d.date", i);
        edje_object_part_text_set(inst->forecasts->forecasts_obj, name, inst->forecast[i].date);
        snprintf(name, sizeof(name), "e.text.day%d.description", i);
        edje_object_part_text_set(inst->forecasts->forecasts_obj, name, inst->forecast[i].desc);
        snprintf(name, sizeof(name), "e.text.day%d.high", i);
        snprintf(buf, sizeof(buf), "%d °%c", inst->forecast[i].high, inst->units.temp);
        edje_object_part_text_set(inst->forecasts->forecasts_obj, name, buf);
        snprintf(name, sizeof(name), "e.text.day%d.low", i);
        snprintf(buf, sizeof(buf), "%d °%c", inst->forecast[i].low, inst->units.temp);
        edje_object_part_text_set(inst->forecasts->forecasts_obj, name, buf);
        snprintf(name, sizeof(name), "e.swallow.day%d.icon", i);
        swallow = edje_object_part_swallow_get(inst->forecasts->forecasts_obj, name);
        if (swallow)
          evas_object_del(swallow);
        edje_object_part_swallow(inst->forecasts->forecasts_obj, name,
                                 _fc_popup_icon_create(inst->gcc->gadcon->evas, inst->forecast[i].code));
      }
}
 
 
static void
_fc_display_set(Instance *inst, Eina_Bool ok __UNUSED__)
{
   EINA_SAFETY_ON_NULL_RETURN(inst);
 
   char buf[4096];
   char m[4096];

   snprintf(m, sizeof(m), "%s/forecasts.edj",
            e_module_dir_get(forecasts_config->module));
   snprintf(buf, sizeof(buf), "modules/forecasts/icons/%d", inst->condition.code);
   if (!e_theme_edje_object_set(inst->forecasts->icon_obj,
                                "base/theme/modules/forecasts/icons", buf))
     edje_object_file_set(inst->forecasts->icon_obj, m, buf);
   edje_object_part_swallow(inst->forecasts->forecasts_obj, "icon", inst->forecasts->icon_obj);
 
   if (!inst->ci->show_text)
     edje_object_signal_emit(inst->forecasts_obj, "e,state,description,hide", "e");
   else
     edje_object_signal_emit(inst->forecasts_obj, "e,state,description,show", "e");
 
   if (inst->gcc->gadcon->orient == E_GADCON_ORIENT_FLOAT)
   snprintf(buf, sizeof(buf), "%d °%c", inst->condition.temp, inst->units.temp);
   else 
   snprintf(buf, sizeof(buf), "%d°", inst->condition.temp);
   
   edje_object_part_text_set(inst->forecasts->forecasts_obj, "e.text.temp", buf);
   edje_object_part_text_set(inst->forecasts->forecasts_obj, "e.text.description",
                             inst->condition.desc);

   if (inst->ci->label[0] == '\0')
   { 
     edje_object_part_text_set(inst->forecasts->forecasts_obj, "e.text.location", inst->location);
     edje_object_part_text_set(inst->forecasts->forecasts_obj, "e.text.country", inst->country);
   }
   else
   { 
     edje_object_part_text_set(inst->forecasts->forecasts_obj, "e.text.location", inst->label);
     edje_object_part_text_set(inst->forecasts->forecasts_obj, "e.text.country", "");
   }
   
   if (inst->gcc->gadcon->orient == E_GADCON_ORIENT_FLOAT)
      _right_values_update(inst); //Updating right two icons description
 
   if (inst->popup) _fc_popup_destroy(inst);
   inst->popup = NULL;
}
 
void
_fc_config_updated(Config_Item *ci)
{
   Eina_List *l;
   char buf[4096];
 
   if (!forecasts_config)
     return;
   for (l = forecasts_config->instances; l; l = l->next)
     {
        Instance *inst;
 
        inst = l->data;
        if (inst->ci != ci) continue;
        int area_changed = 0;
        int lang_changed = 0;
 
        if (inst->area && strcmp(inst->area, inst->ci->code))
          area_changed = 1;
        if (inst->area) eina_stringshare_del(inst->area);
        inst->area = eina_stringshare_add(inst->ci->code);
 
        if (inst->language && strcmp(inst->language, inst->ci->lang))
          lang_changed = 1;
        if (inst->language) eina_stringshare_del(inst->language);
        inst->language = eina_stringshare_add(inst->ci->lang);
        
        if (inst->label) eina_stringshare_del(inst->label);
        inst->label = eina_stringshare_add(inst->ci->label);
       
        _fc_converter(inst);
       
        if (inst->popup) _fc_popup_destroy(inst);
        inst->popup = NULL;
 
        snprintf(buf, sizeof(buf), "%d °%c", inst->condition.temp, inst->units.temp);
        edje_object_part_text_set(inst->forecasts->forecasts_obj, "e.text.temp", buf);
 
        if (!inst->ci->show_text)
          edje_object_signal_emit(inst->forecasts_obj, "e,state,description,hide", "e");
        else
          edje_object_signal_emit(inst->forecasts_obj, "e,state,description,show", "e");
 
        _fc_display_set(inst, 1);
        //Updating right two icons description
        _right_values_update(inst);
       
 
        if ((area_changed) || (lang_changed))
          _cb_fc_check(inst);
 
        if (!inst->check_timer)
          inst->check_timer =
            ecore_timer_add(inst->ci->poll_time, _cb_fc_check,
                            inst);
        else
          ecore_timer_interval_set(inst->check_timer,
                                   inst->ci->poll_time);
     }
}
 
static void
_fc_popup_content_create(Instance *inst)
{
   EINA_SAFETY_ON_NULL_RETURN(inst);
 
   Evas_Object *o, *ol, *of, *ob, *oi;
   Evas *evas;
   char buf[4096];
   int row = 0, i;
   Evas_Coord w, h, mw, mh;
 
   if (!inst->location) return;
 
   inst->popup = e_gadcon_popup_new(inst->gcc);
 
   evas = inst->popup->win->evas;
   o = e_widget_list_add(evas, 0, 0);
   if (inst->ci->label[0] == '\0')
     snprintf(buf, sizeof(buf), D_("%s: Current Conditions"), inst->location);
   else
     snprintf(buf, sizeof(buf), D_("%s: Current Conditions"), inst->label);
   of = e_widget_frametable_add(evas, buf, 0);
 
   snprintf(buf, sizeof(buf), "%s: %d °%c", inst->condition.desc, inst->condition.temp, inst->units.temp);
   ob = e_widget_label_add(evas, buf);
   e_widget_frametable_object_append(of, ob, 0, row, 2, 1, 0, 1, 1, 0);
 
   oi = _fc_popup_icon_create(inst->popup->win->evas, inst->condition.code);
   edje_object_size_max_get(oi, &w, &h);
   if (w > 160) w = 160;  /* For now there is a limit to how big the icon should be */
   if (h > 160) h = 160;  /* In the future, the icon should be set from the theme, not part of the table */
   ob = e_widget_image_add_from_object(evas, oi, w, h);
   e_widget_frametable_object_append(of, ob, 2, row, 1, 2, 1, 0, 1, 1);
 
   ob = e_widget_label_add(evas, D_("Feels Like:"));
   e_widget_frametable_object_append(of, ob, 0, ++row, 1, 1, 1, 0, 0, 0);
   snprintf(buf, sizeof(buf), "%d °%c", inst->condition.feel, inst->units.temp);
   ob = e_widget_label_add(evas, buf);
   e_widget_frametable_object_append(of, ob, 1, row, 1, 1, 1, 0, 0, 0); 
 
   ob = e_widget_label_add(evas, D_("Wind Chill:"));
   e_widget_frametable_object_append(of, ob, 0, ++row, 1, 1, 1, 0, 0, 0);
   snprintf(buf, sizeof(buf), "%d °%c", inst->details.wind.chill, inst->units.temp);
   ob = e_widget_label_add(evas, buf);
   e_widget_frametable_object_append(of, ob, 1, row, 1, 1, 1, 0, 0, 0);
 
   ob = e_widget_label_add(evas, D_("Wind Speed:"));
   e_widget_frametable_object_append(of, ob, 0, ++row, 1, 1, 1, 0, 0, 0);
   snprintf(buf, sizeof(buf), "%d %s", inst->details.wind.speed, inst->units.speed);
   ob = e_widget_label_add(evas, buf);
   e_widget_frametable_object_append(of, ob, 1, row, 1, 1, 1, 0, 0, 0);
 
   ob = e_widget_label_add(evas, D_("Humidity:"));
   e_widget_frametable_object_append(of, ob, 0, ++row, 1, 1, 1, 0, 0, 0);
   snprintf(buf, sizeof(buf), "%d %%", inst->details.atmosphere.humidity);
   ob = e_widget_label_add(evas, buf);
   e_widget_frametable_object_append(of, ob, 1, row, 1, 1, 1, 0, 0, 0);
   
   ob = e_widget_label_add(evas, D_("Precipitation:"));
   e_widget_frametable_object_append(of, ob, 0, ++row, 1, 1, 1, 0, 0, 0);
   snprintf(buf, sizeof(buf), "%.1f mm", inst->details.atmosphere.precip);
   ob = e_widget_label_add(evas, buf);
   e_widget_frametable_object_append(of, ob, 1, row, 1, 1, 1, 0, 0, 0);
 
   ob = e_widget_label_add(evas, D_("Visibility:"));
   e_widget_frametable_object_append(of, ob, 0, ++row, 1, 1, 1, 0, 0, 0);
   snprintf(buf, sizeof(buf), "%.2f %s", inst->details.atmosphere.visibility, inst->units.distance);
   ob = e_widget_label_add(evas, buf);
   e_widget_frametable_object_append(of, ob, 1, row, 1, 1, 1, 0, 0, 0);
 
   ob = e_widget_label_add(evas, D_("Pressure:"));
   e_widget_frametable_object_append(of, ob, 0, ++row, 1, 1, 1, 0, 0, 0);
   if (inst->ci->degrees == DEGREES_C)
      snprintf(buf, sizeof(buf), "%.0f %s", inst->details.atmosphere.pressure, inst->units.pressure);
   else
      snprintf(buf, sizeof(buf), "%.2f %s", inst->details.atmosphere.pressure, inst->units.pressure);
   ob = e_widget_label_add(evas, buf);
   e_widget_frametable_object_append(of, ob, 1, row, 1, 1, 1, 0, 0, 0);
 
   ob = e_widget_label_add(evas, D_("Sunrise / Sunset: "));
   e_widget_frametable_object_append(of, ob, 0, ++row, 1, 1, 1, 0, 0, 0);
   snprintf(buf, sizeof(buf), "%s / ", inst->details.astronomy.sunrise);
   ob = e_widget_label_add(evas, buf);
   e_widget_frametable_object_append(of, ob, 1, row, 1, 1, 1, 0, 1, 0);
 
   snprintf(buf, sizeof(buf), "%s", inst->details.astronomy.sunset);
   ob = e_widget_label_add(evas, buf);
   e_widget_frametable_object_append(of, ob, 2, row, 1, 1, 1, 0, 1, 0);
 
   e_widget_list_object_append(o, of, 1, 1, 0.5);
   ol = e_widget_list_add(evas, 1, 1);
   
   //block of code to retrieve a 3rd week day name
   time_t t;
   struct tm *tmp;
   char buf_time[1024]; 
   t = time(NULL) + 48 * 3600;  //48 hours 
   tmp = localtime(&t); 
   strftime(buf_time, 1024, "%A", tmp);
   //---------------------------------------------
   
   for (i = 0; i < inst->ci->days; i++)
     {
        row = 0;
 
        if (!i)
          snprintf(buf, sizeof(buf), D_("Today"));
        else if (i == 1)
          snprintf(buf, sizeof(buf), D_("Tomorrow"));
        else
           snprintf(buf, sizeof(buf), "%s", buf_time);
          //~ snprintf(buf, sizeof(buf), "%s", inst->forecast[i].date);
        of = e_widget_frametable_add(evas, buf, 0);
 
        ob = e_widget_label_add(evas, inst->forecast[i].desc);
        e_widget_frametable_object_append(of, ob, 0, row, 3, 1, 0, 1, 1, 1);
 
        ob = e_widget_label_add(evas, D_("High"));
        e_widget_frametable_object_append(of, ob, 0, ++row, 1, 1, 1, 0, 1, 0);
        snprintf(buf, sizeof(buf), "%d °%c", inst->forecast[i].high, inst->units.temp);
        ob = e_widget_label_add(evas, buf);
        e_widget_frametable_object_append(of, ob, 1, row, 1, 1, 1, 0, 1, 0);
 
        ob = e_widget_image_add_from_object(evas,
                                            _fc_popup_icon_create(inst->popup->win->evas,
                                                                         inst->forecast[i].code), 0, 0);
        e_widget_frametable_object_append(of, ob, 2, row, 1, 2, 1, 1, 0, 0);
 
        ob = e_widget_label_add(evas, D_("Low"));
        e_widget_frametable_object_append(of, ob, 0, ++row, 1, 1, 1, 0, 1, 0);
        snprintf(buf, sizeof(buf), "%d °%c", inst->forecast[i].low, inst->units.temp);
        ob = e_widget_label_add(evas, buf);
        e_widget_frametable_object_append(of, ob, 1, row, 1, 1, 1, 0, 1, 0);
        e_widget_list_object_append(ol, of, 1, 1, 0.5);
     }
 
   e_widget_list_object_append(o, ol, 1, 1, 0.5);
   e_widget_size_min_get(o, &mw, &mh);
   if ((double)mw / mh > GOLDEN_RATIO)
     mh = mw / GOLDEN_RATIO;
   else if ((double)mw / mh < GOLDEN_RATIO - (double)1)
     mw = mh * (GOLDEN_RATIO - (double)1);
   e_widget_size_min_set(o, mw, mh);
 
   e_gadcon_popup_content_set(inst->popup, o);
}
 
static Evas_Object *
_fc_popup_icon_create(Evas *evas, int code)
{
   char buf[4096];
   char m[4096];
   Evas_Object *o;
 
   snprintf(m, sizeof(m), "%s/forecasts.edj",
            e_module_dir_get(forecasts_config->module));
   o = edje_object_add(evas);
   snprintf(buf, sizeof(buf), "modules/forecasts/icons/%d", code);
   if (!e_theme_edje_object_set(o, "base/theme/modules/forecasts/icons", buf))
     edje_object_file_set(o, m, buf);
 
   return o;
}
 
static void
_fc_popup_destroy(Instance *inst)
{
   EINA_SAFETY_ON_NULL_RETURN(inst);
 
   if (!inst->popup) return;
   e_object_del(E_OBJECT(inst->popup));
   inst->popup = NULL;
}
 
static void
_cb_mouse_down(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   EINA_SAFETY_ON_NULL_RETURN(data);
 
   Instance *inst = data;
   Evas_Event_Mouse_Down *ev = event_info;
 
   if ((ev->button == 1) && (ev->flags & EVAS_BUTTON_DOUBLE_CLICK))  
    {
     _cb_fc_check(inst);
    }
    else if (ev->button == 1)
     {
       if (!inst->ci->popup_on_hover)
          {
             if (!inst->popup) _fc_popup_content_create(inst);
             e_gadcon_popup_show(inst->popup);
             return;
          }
      e_gadcon_popup_toggle_pinned(inst->popup);
    }

}
 
static void
_cb_mouse_in(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   EINA_SAFETY_ON_NULL_RETURN(data);
   
   Instance *inst = data;
   if (!inst->ci->popup_on_hover) return;
 
   if (!inst->popup) _fc_popup_content_create(inst);
   e_gadcon_popup_show(inst->popup);
}
 
static void
_cb_mouse_out(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   EINA_SAFETY_ON_NULL_RETURN(data);
   
   Instance *inst = data;
   if (!(inst->popup)) return;
 
   if (inst->popup->pinned) return;
   e_gadcon_popup_hide(inst->popup);
}
 
