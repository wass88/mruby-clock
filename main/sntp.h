#ifndef L_SNTP_H
#define L_SNTP_H
#include <time.h>
struct timenum {
  int tm_usec;
  int tm_sec;
  int tm_min;
  int tm_hour;
  int tm_mday;
  int tm_mon;
  int tm_year;
  int tm_wday;
  int tm_yday;
};

void get_time(time_t *now, struct tm *timeinfo); 
void get_timenum(time_t *now, struct tm *timeinfo, struct timenum *num); 
void sync_sntp(void *pvParameter);
#endif