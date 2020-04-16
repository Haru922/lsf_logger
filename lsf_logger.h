#ifndef __LSF_LOGGER_H__
#define __LSF_LOGGER_H__

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <glib.h>
#include <glib/gprintf.h>
#include <glib/gstdio.h>

#define LOGGER_MAX            10
#define LOGGER_FMT_STRING_BUF 4096
#define LOGGER_FMT_WORD_BUF   10

enum {
  LOGGER_NAME,
  LOGGER_PATH,
  LOGGER_FMT,
  LOGGER_LEVEL,
  LOGGER_MAX_BYTES,
  LOGGER_BACKUP_COUNT,
  LOGGER_FILE_EXTENSION,
  LOGGER_CONFIG_NUMS
};

enum {
  LOGGER_FMT_NONE,
  LOGGER_FMT_ASCTIME,
  LOGGER_FMT_LOG_LEVEL,
  LOGGER_FMT_FILE_NAME,
  LOGGER_FMT_LINE_NUM,
  LOGGER_FMT_FUNC_NAME
};

typedef struct {
  gchar *logger_config[LOGGER_CONFIG_NUMS];
  int    log_file_cnt;
} Logger;

Logger *logger[LOGGER_MAX];
int logger_cnt;

#define GLOGC(logger, format, ...) lsf_logger_print(logger, "CRITICAL", __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)
#define GLOGE(logger, format, ...) lsf_logger_print(logger, "ERROR", __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)
#define GLOGW(logger, format, ...) lsf_logger_print(logger, "WARNING", __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)
#define GLOGI(logger, format, ...) lsf_logger_print(logger, "INFO", __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)
#define GLOGD(logger, format, ...) lsf_logger_print(logger, "DEBUG", __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)

extern Logger  *lsf_get_logger                      (char *logger_name, char *config_path);
extern Logger  *lsf_logger_exist                    (char *logger_name);
extern gboolean lsf_logger_init                     (Logger *logger, char *config_path);
extern void     lsf_logger_free                     (Logger *logger);
extern int      lsf_logger_print                    (Logger *logger, char *log_level, const char *file_name, const int line_num, const char *func_name, char *format, ...);
extern gchar   *lsf_logger_get_log_format_string    (Logger *logger);
extern int      lsf_logger_get_log_file_cnt         (Logger *logger, GDir *dir);
extern gchar   *lsf_logger_get_log_file_first       (Logger *logger, GDir *dir);
extern gchar   *lsf_logger_get_log_file_output_path (Logger *logger, GDir *dir, GDateTime *local_time);

#endif
