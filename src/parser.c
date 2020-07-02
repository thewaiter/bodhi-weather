#include <json-c/json.h>
#include "parser.h"

#define KM_TO_MI     1.609344
#define MB_TO_IN     0.029530

#define JSON_JOGS(obj) ((obj)? json_object_get_string(obj): "")
#define JSON_ATOI(obj, key)  atoi(JSON_JOGS(_json_object_object_get(obj, key)))
#define JSON_ATOF(obj, key)  atof(JSON_JOGS(_json_object_object_get(obj, key)))
#define JSON_SET_STR(str, obj, key, n)  strncpy(str, JSON_JOGS(_json_object_object_get(obj, key)), n)
#define JSON_ARRAY_CHECK(obj, n) \
    if (json_object_get_type(obj) != json_type_array || json_object_array_length(obj) != n) \
      goto error;

json_object *
_json_object_object_get(struct json_object *obj, const char *key)
{
    json_object *temp;
    if (json_object_object_get_ex(obj, key, &temp))
        return temp;
    CRI("Error: in json obj or key");
    return NULL;

}

Eina_Bool
fc_parse_json(void *data)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(data, 0);

   Instance *inst = data;
   if (!inst->buffer)
     return EINA_FALSE;

   char *needle;
   char city[256], region[256], country[256], location[513];
   float visibility, pressure;
   json_object *root, *main, *submain, *sub, *temp;
   Eina_Bool have_lang =  (inst->ci->lang[0] != '\0') && strcmp(inst->ci->lang, "en");

   needle = (char *) eina_binbuf_string_get(inst->buffer);
   if (needle[0] == '\0') return EINA_FALSE;

   root = json_tokener_parse(needle);

    // Parse Json array "current_condition"
    json_object *current_condition = _json_object_object_get(root,  "current_condition");
    JSON_ARRAY_CHECK(current_condition, 1);

    main = json_object_array_get_idx(current_condition, 0);

    inst->condition.feel_c = JSON_ATOI(main, "FeelsLikeC");
    inst->condition.feel_f = JSON_ATOI(main, "FeelsLikeF");
    inst->details.atmosphere.humidity = JSON_ATOI(main, "humidity");
    inst->details.atmosphere.precip = JSON_ATOF(main, "precipMM");
    inst->details.atmosphere.pressure_km = JSON_ATOF(main, "pressure");
    inst->details.atmosphere.pressure_mi = inst->details.atmosphere.pressure_km * MB_TO_IN;
    inst->condition.temp_c = JSON_ATOI(main, "temp_C");
    inst->condition.temp_f = JSON_ATOI(main, "temp_F");
    inst->details.atmosphere.visibility_km = JSON_ATOF(main, "visibility");
    inst->details.atmosphere.visibility_mi = inst->details.atmosphere.visibility_km * KM_TO_MI;
    inst->condition.code = JSON_ATOI(main, "weatherCode");
       inst->details.wind.speed_km = JSON_ATOI(main, "windspeedKmph");
    inst->details.wind.speed_mi = JSON_ATOI(main, "windspeedMiles");

    if (have_lang)
    {
      char lang_key[8];
      snprintf(lang_key, 8, "lang_%s",inst->ci->lang);
      json_object *lang = _json_object_object_get(main, lang_key);
      JSON_ARRAY_CHECK(lang, 1);

      submain = json_object_array_get_idx(lang, 0);
      JSON_SET_STR(inst->condition.desc, submain, "value", 255);
    }
    else
    {
        json_object *weather_desc = _json_object_object_get(main, "weatherDesc");
        JSON_ARRAY_CHECK(weather_desc, 1);

        submain = json_object_array_get_idx(weather_desc, 0);
        JSON_SET_STR(inst->condition.desc, submain, "value", 255);
    }
    // Parse Json array "nearest_area"
    json_object *nearest_area = _json_object_object_get(root, "nearest_area");
    JSON_ARRAY_CHECK(nearest_area, 1);

    main = json_object_array_get_idx(nearest_area, 0);

    json_object *area_name = _json_object_object_get(main, "areaName");
    submain = json_object_array_get_idx(area_name, 0);
    JSON_SET_STR(city, submain, "value", 255);

    json_object *obj_country = _json_object_object_get(main, "country");
    submain = json_object_array_get_idx(obj_country, 0);
    JSON_SET_STR(country, submain, "value", 255);

    json_object *obj_region = _json_object_object_get(main, "region");
    submain = json_object_array_get_idx(obj_region, 0);
    JSON_SET_STR(region, submain, "value", 255);

    if (strcmp(city, region))
       snprintf(location, 513, "%s, %s", city, region);
    else
       snprintf(location, 513, "%s", city);

    eina_stringshare_replace(&inst->location, location);
    eina_stringshare_replace(&inst->country, country);

    // Parse Json array "weather" for days
    json_object *weather = _json_object_object_get(root, "weather");
    JSON_ARRAY_CHECK(weather, FORECASTS);

    for (int i = 0; i < FORECASTS; i++)
    {
        main = json_object_array_get_idx(weather, i);
        if (i == 0)
        {
            json_object *astronomy = _json_object_object_get(main, "astronomy");
            submain = json_object_array_get_idx(astronomy, 0);
            JSON_SET_STR(inst->details.astronomy.sunrise, submain, "sunrise", 8);
            JSON_SET_STR(inst->details.astronomy.sunset, submain, "sunset", 8);
        }
        JSON_SET_STR(inst->forecast[i].date, main, "date", 12);
        inst->forecast[i].high_c = JSON_ATOI(main, "maxtempC");
        inst->forecast[i].high_f = JSON_ATOI(main, "maxtempF");
        inst->forecast[i].low_c  = JSON_ATOI(main, "mintempC");
        inst->forecast[i].low_f  = JSON_ATOI(main, "mintempF");
        //      hourly subarray
        json_object *hourly = _json_object_object_get(main, "hourly");
        JSON_ARRAY_CHECK(hourly, 8);

        submain = json_object_array_get_idx(hourly, 0);
        if (i == 0)
        {
            inst->details.wind.chill_c = JSON_ATOI(submain, "WindChillC");
            inst->details.wind.chill_f = JSON_ATOI(submain, "WindChillF");
        }

        if (have_lang)
        {
            char lang_key[8];
            snprintf(lang_key, 8, "lang_%s",inst->ci->lang);
            json_object *lang = _json_object_object_get(submain, lang_key);
            JSON_ARRAY_CHECK(lang, 1);

            sub = json_object_array_get_idx(lang, 0);
            JSON_SET_STR(inst->forecast[i].desc, sub, "value", 255);
        }
        else
        {
            json_object *weather_desc = _json_object_object_get(submain, "weatherDesc");
            JSON_ARRAY_CHECK(weather_desc, 1);

            sub = json_object_array_get_idx(weather_desc, 0);
            JSON_SET_STR(inst->forecast[i].desc, sub, "value", 255);
        }
        inst->forecast[i].code = JSON_ATOI(submain, "weatherCode");
   }
   json_object_put(root);

   return EINA_TRUE;

error:
   WRN("**: Couldn't parse info from %s\n", inst->ci->host);
   return EINA_FALSE;
}
