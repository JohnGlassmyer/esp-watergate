#ifndef SERVE_H
#define SERVE_H

void serve_notFound();

void serve_get_log();

void serve_get_time();

void serve_post_setTime();

void serve_get_temperature();

void serve_get_wateringSeconds();

void serve_post_startWatering();

void serve_post_stopWatering();

void serve_get_schedule();

void serve_post_setSchedule();

void serve_get_mapping();

void serve_post_setMapping();

void serve_post_setWifiConfig();

void serve_get_temperatureHistory();

void serve_post_deleteLogs();

void serve_get_ls();

#endif
