#ifndef BRICK_APP_SETTINGS_H_
#define BRICK_APP_SETTINGS_H_
#pragma once

#include <set>
#include "brick/account.h"
#include <include/internal/cef_types_wrappers.h>

struct AppSettings {
  typedef std::map<std::string, std::string> client_scripts_map;

  std::string app_token;
  std::string profile_path;
  std::string cache_path;
  std::string log_file;
  std::string proxy_server;
  std::string resource_dir;
  bool ignore_certificate_errors;
  cef_log_severity_t log_severity;
  bool start_minimized;
  bool auto_away;
  bool external_api;
  bool hide_on_delete;
  bool extended_status;
  client_scripts_map client_scripts;

  static AppSettings InitByJson(std::string json);

  AppSettings();
  ~AppSettings();
  void UpdateByJson(std::string json);
  std::string DumpJson();
};

#endif // BRICK_APP_SETTINGS_H_
