#include "parser.h"

#define KM_TO_MI     1.609344
#define MB_TO_IN     0.029530

#define PARSER_TEST(val)         \
  do                             \
    {                            \
       if (!needle)              \
       {                         \
          ERR("Parse: %s", val); \
          goto error;             \
       }                         \
    }                            \
  while(0)

static char *
seek_text(char * string, const char * value, int jump)
{
  if (string) string = strstr(string, value);
  if (!string)
   {
      goto error;
  }
  string += jump;
  return string;
 
  error:
   WRN("**: Couldn't parse info from data file\n");
   return NULL;
 
}
 
Eina_Bool
fc_parse_json(void *data)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(data, 0);
   
   Instance *inst = data;
 
   char *needle;
   char city[256];
   char region[256];
   char country[256];
   char location[513];
   float visibility;
   float pressure;
   int have_lang = 0;
 
   if (!inst->buffer)
     return EINA_FALSE;
   
   needle = (char *) eina_binbuf_string_get(inst->buffer);
   if (needle[0] == '\0') return EINA_FALSE;
   
   needle = seek_text(needle, "FeelsLikeC", 0);
   needle = seek_text(needle, ":", 3);
   PARSER_TEST("FeelsLikeC");
   sscanf(needle, "%d\"", &inst->condition.feel_c);
   
   needle = seek_text(needle, "FeelsLikeF", 0);
   needle = seek_text(needle, ":", 3);
   PARSER_TEST("FeelsLikeF");
   sscanf(needle, "%d\"", &inst->condition.feel_f);
   
   needle = seek_text(needle, "humidity", 0);
   needle = seek_text(needle, ":", 3);
   PARSER_TEST("humidity");
   sscanf(needle, "%d\"", &inst->details.atmosphere.humidity);
   
   if (inst->ci->lang[0] != '\0')
   {
     needle = seek_text(needle, "lang", 0);
     needle = seek_text(needle, "value",0);
     needle = seek_text(needle, ":", 3);
     if (needle)
     {
        sscanf(needle, "%255[^\"]\"", inst->condition.desc);
        have_lang = 1;
      }
      else 
      {
         have_lang = 0;
         needle = (char *) eina_binbuf_string_get(inst->buffer);
      }
   }
   
   needle = seek_text(needle, "localObsDateTime", 0);
   needle = seek_text(needle, ":", 3);
   PARSER_TEST("localObsDateTime");
   sscanf(needle, "%51[^\"]\"", inst->condition.update);
   
   needle = seek_text(needle, "precipMM", 0);
   needle = seek_text(needle, ":", 3);
   PARSER_TEST("precipMM");
   sscanf(needle, "%f\"", &inst->details.atmosphere.precip);
   
   needle = seek_text(needle, "pressure", 0);
   needle = seek_text(needle, ":", 3);
   PARSER_TEST("pressure");
   sscanf(needle, "%f\"", &pressure);
   
   inst->details.atmosphere.pressure_km = pressure;
   inst->details.atmosphere.pressure_mi = pressure * MB_TO_IN;
   
   needle = seek_text(needle, "temp_C", 0);
   needle = seek_text(needle, ":", 3);
   PARSER_TEST("temp_C");
   sscanf(needle, "%d\"", &inst->condition.temp_c);
 
   needle = seek_text(needle, "temp_F", 0);
   needle = seek_text(needle, ":", 3);
   PARSER_TEST("temp_F");
   sscanf(needle, "%d\"", &inst->condition.temp_f);
 
   needle = seek_text(needle, "visibility", 0);
   needle = seek_text(needle, ":", 3);
   PARSER_TEST("visibility");
   sscanf(needle, "%f\"", &visibility);
   inst->details.atmosphere.visibility_km = visibility;
   inst->details.atmosphere.visibility_mi = visibility * KM_TO_MI;
   
   needle = seek_text(needle, "weatherCode", 0);
   needle = seek_text(needle, ":", 3);
   PARSER_TEST("weatherCode");
   sscanf(needle, "%d\"", &inst->condition.code);
 
   if (!have_lang)
   {
     needle = seek_text(needle, "weatherDesc", 0);
     needle = seek_text(needle, "value", 0);
     needle = seek_text(needle, ":", 3);
     PARSER_TEST("weatherDesc"); 
     sscanf(needle, "%255[^\"]\"", inst->condition.desc);
   }
   
   needle = seek_text(needle, "windspeedKmph", 0);
   needle = seek_text(needle, ":", 3);
   PARSER_TEST("windspeedKmph"); 
   sscanf(needle, "%d\"", &inst->details.wind.speed_km);
   
   needle = seek_text(needle, "windspeedMiles", 0);
   needle = seek_text(needle, ":", 3);
   PARSER_TEST("windspeedMiles");  
   sscanf(needle, "%d\"", &inst->details.wind.speed_mi);
   
   needle = seek_text(needle, "areaName", 0);
   needle = seek_text(needle, "value", 0);
   needle = seek_text(needle, ":", 3);
   PARSER_TEST("areaName"); 
   sscanf(needle, "%255[^\"]\"", city);
   
   needle = seek_text(needle, "country", 0);
   needle = seek_text(needle, "value", 0);
   needle = seek_text(needle, ":", 3);
   PARSER_TEST("country");
   sscanf(needle, "%255[^\"]\"", country);
   
   needle = seek_text(needle, "region", 0);
   needle = seek_text(needle, "value", 0);
   needle = seek_text(needle, ":", 3);
   PARSER_TEST("region");
   sscanf(needle, "%255[^\"]\"", region);
   
   if (strcmp(city, region) == 0)
       snprintf(location, 513, "%s", city); 
   else
       snprintf(location, 513, "%s, %s", city, region); 
       
   eina_stringshare_replace(&inst->location, location);
   eina_stringshare_replace(&inst->country, country);
   
   needle = seek_text(needle, "sunrise", 0);
   needle = seek_text(needle, ":", 3);
   PARSER_TEST("sunrise"); 
   sscanf(needle, "%8[^\"]\"", inst->details.astronomy.sunrise);
   
   needle = seek_text(needle, "sunset", 0);
   needle = seek_text(needle, ":", 3);
   PARSER_TEST("sunset");
   sscanf(needle, "%8[^\"]\"", inst->details.astronomy.sunset);
   
   // day 0
   needle = seek_text(needle, "date", 0);
   needle = seek_text(needle, ":", 3);
   PARSER_TEST("date");
   sscanf(needle, "%12[^\"]\"", inst->forecast[0].date);
   
   needle = seek_text(needle, "900", 0);
   
   needle = seek_text(needle, "WindChillC", 0);
   needle = seek_text(needle, ":", 3);
   PARSER_TEST("WindChillC");
   sscanf(needle, "%d\"", &inst->details.wind.chill_c);
   
   needle = seek_text(needle, "WindChillF", 0);
   needle = seek_text(needle, ":", 3);
   PARSER_TEST("WindChillF");
   sscanf(needle, "%d\"", &inst->details.wind.chill_f);
   
   if (have_lang)
   {
   needle = seek_text(needle, "lang", 0);
   needle = seek_text(needle, "value", 0);
   needle = seek_text(needle, ":", 3);
   PARSER_TEST("inst->forecast[0].desc");
   sscanf(needle, "%255[^\"]\"", inst->forecast[0].desc);
   }
   
   //~ needle = seek_text(needle, "1200", 0);
   needle = seek_text(needle, "weatherCode", 0);
   needle = seek_text(needle, ":", 3);
   PARSER_TEST("weatherCode");
   sscanf(needle, "%d\"", &inst->forecast[0].code);
   
   if (!have_lang)
   {
     needle = seek_text(needle, "value", 0);
     needle = seek_text(needle, ":", 3);
     PARSER_TEST("forecast[0].desc");
     sscanf(needle, "%255[^\"]\"", inst->forecast[0].desc);
   }
   
   needle = seek_text(needle, "maxtempC", 0);
   needle = seek_text(needle, "maxtempC", 0); //TODO: are 2 tests needed here ?
   needle = seek_text(needle, ":", 3);
   PARSER_TEST("maxtempC"); 
   sscanf(needle, "%d\"", &inst->forecast[0].high_c);
   
   needle = seek_text(needle, "maxtempF", 0);
   needle = seek_text(needle, ":", 3);
   PARSER_TEST("maxtempF"); 
   sscanf(needle, "%d\"", &inst->forecast[0].high_f);
   
   needle = seek_text(needle, "mintempC", 0);
   needle = seek_text(needle, ":", 3);
   PARSER_TEST("mintempC");
   sscanf(needle, "%d\"", &inst->forecast[0].low_c);
   
   needle = seek_text(needle, "mintempF", 0);
   needle = seek_text(needle, ":", 3);
   PARSER_TEST("mintempF");
   sscanf(needle, "%d\"", &inst->forecast[0].low_f);
   
   //day 1
   needle = seek_text(needle, "date", 0);
   needle = seek_text(needle, ":", 3);
   PARSER_TEST("inst->forecast[1].date");
   sscanf(needle, "%12[^\"]\"", inst->forecast[1].date);
   
   if (have_lang)
   {
     needle = seek_text(needle, "900", 0);
     needle = seek_text(needle, "lang", 0);
     needle = seek_text(needle, "value", 0);
     needle = seek_text(needle, ":", 3);
     PARSER_TEST("inst->forecast[1].desc"); 
     sscanf(needle, "%255[^\"]\"", inst->forecast[1].desc);
   }
   
   //~ needle = seek_text(needle, "1200", 0);
   needle = seek_text(needle, "weatherCode", 0);
   needle = seek_text(needle, ":", 3);
   PARSER_TEST("weatherCode");
   sscanf(needle, "%d\"", &inst->forecast[1].code);
 
    if (!have_lang)
   {
     needle = seek_text(needle, "value", 0);
     needle = seek_text(needle, ":", 3);
     PARSER_TEST("inst->forecast[1].desc");
     sscanf(needle, "%255[^\"]\"", inst->forecast[1].desc);
   }
   
   needle = seek_text(needle, "maxtempC", 0);
   needle = seek_text(needle, ":", 3);
   PARSER_TEST("maxtempC");
   sscanf(needle, "%d\"", &inst->forecast[1].high_c);
   
   needle = seek_text(needle, "maxtempF", 0);
   needle = seek_text(needle, ":", 3);
   PARSER_TEST("maxtempF");
   sscanf(needle, "%d\"", &inst->forecast[1].high_f);
   
   needle = seek_text(needle, "mintempC", 0);
   needle = seek_text(needle, ":", 3);
   PARSER_TEST("mintempC");
   sscanf(needle, "%d\"", &inst->forecast[1].low_c);
   
   needle = seek_text(needle, "mintempF", 0);
   needle = seek_text(needle, ":", 3);
   PARSER_TEST("mintempF");
   sscanf(needle, "%d\"", &inst->forecast[1].low_f);
   
   //day 2
   needle = seek_text(needle, "date", 0);
   needle = seek_text(needle, ":", 3);
   PARSER_TEST("date");
   sscanf(needle, "%12[^\"]\"", inst->forecast[2].date);
   
   if (have_lang)
   {
     needle = seek_text(needle, "900", 0);
     needle = seek_text(needle, "lang", 0);
     needle = seek_text(needle, "value", 0);
     needle = seek_text(needle, ":", 3);
     PARSER_TEST("inst->forecast[2].desc");
     sscanf(needle, "%255[^\"]\"", inst->forecast[2].desc);
   }
   
   //~ needle = seek_text(needle, "1200", 0);
   needle = seek_text(needle, "weatherCode", 0);
   needle = seek_text(needle, ":", 3);
   PARSER_TEST("weatherCode");
   sscanf(needle, "%d\"", &inst->forecast[2].code);
 
    if (!have_lang)
   {
     needle = seek_text(needle, "value", 0);
     needle = seek_text(needle, ":", 3);
     PARSER_TEST("inst->forecast[2].desc");
     sscanf(needle, "%255[^\"]\"", inst->forecast[2].desc);
   }
   
   needle = seek_text(needle, "maxtempC", 0);
   needle = seek_text(needle, ":", 3);
   PARSER_TEST("maxtempC");
   sscanf(needle, "%d\"", &inst->forecast[2].high_c);
   
   needle = seek_text(needle, "maxtempF", 0);
   needle = seek_text(needle, ":", 3);
   PARSER_TEST("maxtempF");
   sscanf(needle, "%d\"", &inst->forecast[2].high_f);
   
   needle = seek_text(needle, "mintempC", 0);
   needle = seek_text(needle, ":", 3);
   PARSER_TEST("mintempC");
   sscanf(needle, "%d\"", &inst->forecast[2].low_c);
   
   needle = seek_text(needle, "mintempF", 0);
   needle = seek_text(needle, ":", 3);
   PARSER_TEST("mintempF"); 
   sscanf(needle, "%d\"", &inst->forecast[2].low_f);
   
   return EINA_TRUE;

error:
   WRN("**: Couldn't parse info from %s\n", inst->ci->host);
   return EINA_FALSE;
}
 
