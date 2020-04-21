#include "lsf-logger.h"

static int
logger_exist (char *logger_name)
{
  int i;

  for (i = 0; i < LOGGER_MAX; i++)
  {
    if (logger[i] != NULL && !g_strcmp0 (logger_name, logger[i]->logger_config[LOGGER_NAME]))
    {
      return i;
    }
  }
  return -1;
}

static int
logger_get_log_level_index (char *log_level)
{
  int ret_val = 0;

  if (!g_strcmp0 (log_level, "CRITICAL"))
  {
    ret_val = LOGGER_LEVEL_CRITICAL;
  }
  else if (!g_strcmp0 (log_level, "ERROR"))
  {
    ret_val = LOGGER_LEVEL_ERROR;
  }
  else if (!g_strcmp0 (log_level, "WARNING"))
  {
    ret_val = LOGGER_LEVEL_WARNING;
  }
  else if (!g_strcmp0 (log_level, "INFO"))
  {
    ret_val = LOGGER_LEVEL_INFO;
  }
  else
  {
    ret_val = LOGGER_LEVEL_DEBUG;
  }
  return ret_val;
}

static gboolean
logger_init (Logger *logger)
{
  int i;
  
  lsf_get_confer (NULL, CONFER_RELOAD_FALSE);

  logger->logger_config[LOGGER_PATH] = lsf_confer_get_str ("LOG", "PATH");
  logger->logger_config[LOGGER_FMT] = lsf_confer_get_str ("LOG", "FORMAT");
  logger->logger_config[LOGGER_LEVEL] = lsf_confer_get_str ("LOG", "LEVEL");
  logger->logger_config[LOGGER_FILE_EXTENSION] = lsf_confer_get_str ("LOG", "FILE_EXTENSION");
  logger->max_bytes = lsf_confer_get_integer ("LOG", "MAX_BYTES");
  logger->backup_count = lsf_confer_get_integer ("LOG", "BACKUP_COUNT");
  logger->output_fp = NULL;

  for (i = 0; i < LOGGER_CONFIG_NUMS; i++)
  {
    if (logger->logger_config[i] == NULL)
    {
      return FALSE;
    }
  }

  logger->log_level_index = logger_get_log_level_index (logger->logger_config[LOGGER_LEVEL]);

  return TRUE;
}

Logger *
lsf_get_logger (char *logger_name)
{ 
  int i; 
    
  if (logger_name == NULL)
  {
    logger_name = LSF_LOGGER_DEFAULT_NAME;
  }
  else if ((i = logger_exist (logger_name)) != -1)
  { 
    return logger[i];
  }

  for (i = 0; i < LOGGER_MAX; i++)
  {
    if (logger[i] == NULL)
    {
      break;
    }
  }
  if (i == LOGGER_MAX)
  {
    return NULL;
  }

  logger[i] = (Logger *) malloc (sizeof (Logger));
  logger_cnt++;

  logger[i]->logger_config[LOGGER_NAME] = g_strdup (logger_name);
  if (!logger_init (logger[i]))
  {
    return NULL;
  }

  return logger[i];
}

void
lsf_logger_free (Logger *logger)
{
  int i;

  if (logger == NULL)
    return;

  if (logger->output_fp != NULL)
  {
    fclose (logger->output_fp);
    logger->output_fp = NULL;
  }

  for (i = 0; i < LOGGER_CONFIG_NUMS; i++)
  {
    g_free (logger->logger_config[i]);
    logger->logger_config[i] = NULL;
  }
  free (logger);
  logger = NULL;
  logger_cnt--;
}

static gchar *
logger_get_log_format_string (Logger *logger)
{
  char arg[LOGGER_FMT_STRING_BUF] = { 0, };
  char word[LOGGER_FMT_WORD_BUF] = { 0, };
  char ch;
  int  a = 0;
  int  i = 0;
  int  j = 0;

  while (ch = logger->logger_config[LOGGER_FMT][i++])
  {
    if (ch == '%')
    {
      arg[a++] = ch;
      i++;
      j = 0;
      while (logger->logger_config[LOGGER_FMT][i] != ')')
      {
        word[j++] = logger->logger_config[LOGGER_FMT][i++];
      }
      i++;
      word[j] = '\0';
      switch (word[0])
      {
        case 't':
          if (!g_strcmp0 (word, "time"))
          {
            arg[a++] = 't';
          }
          break;
        case 'T':
          if (!g_strcmp0 (word, "TIME"))
          {
            arg[a++] = 't';
          }
          break;
        case 'f':
          if (!g_strcmp0 (word, "file"))
          {
            arg[a++] = 'f';
          }
          else if (!g_strcmp0 (word, "func"))
          {
            arg[a++] = 'F';
          }
          break;
        case 'F':
          if (!g_strcmp0 (word, "FILE"))
          {
            arg[a++] = 'f';
          }
          else if (!g_strcmp0 (word, "FUNC"))
          {
            arg[a++] = 'F';
          }
          break;
        case 'l':
          if (!g_strcmp0 (word, "level"))
          {
            arg[a++] = 'l';
          }
          else if (!g_strcmp0 (word, "line"))
          {
            arg[a++] = 'L';
          }
          break;
        case 'L':
          if (!g_strcmp0 (word, "LEVEL"))
          {
            arg[a++] = 'l';
          }
          else if (!g_strcmp0 (word, "LINE"))
          {
            arg[a++] = 'L';
          }
          break;
        case 'm':
          if (!g_strcmp0 (word, "message"))
          {
            arg[a++] = 'm';
          }
          break;
        case 'M':
          if (!g_strcmp0 (word, "MESSAGE"))
          {
            arg[a++] = 'm';
          }
          break;
      }
      for (j = 0; j < LOGGER_FMT_WORD_BUF; j++)
      {
        word[j] = '\0';
      }
    }
    else
    {
      arg[a++] = ch;
    }
  }
  arg[a] = '\0';

  return g_strdup (arg);
}

static gchar *
logger_util_itoa (int num)
{
  char buf[2][10] = { { 0, },
                      { 0, } };
  int  i = 0;
  int  j = 0;

  while (num)
  {
    buf[0][i++] = (num % 10) + '0';
    num /= 10;
  }
  i--;

  while (i >= 0)
  {
    buf[1][j++] = buf[0][i--];
  }
  buf[1][j] = '\0';

  return g_strdup (buf[1]);
}

static gchar *
logger_get_log_file_output_path (Logger    *logger,
                                     GDir      *dir,
                                     GDateTime *local_time)
{
  struct stat file_info;
  const char *dir_file;
  gboolean    full = FALSE;
  GError     *error = NULL;
  gchar      *output_file_identifier = NULL;
  gchar      *output_file_name = NULL;
  gchar      *target_file = NULL;
  gchar      *output_path = NULL;
  gchar      *renamed_file = NULL;
  gchar      *ext_num_str = NULL;
  int         i;

  output_file_identifier = g_date_time_format (local_time, "%F");
  output_file_name = g_strconcat (logger->logger_config[LOGGER_NAME], "-", output_file_identifier, ".", logger->logger_config[LOGGER_FILE_EXTENSION], NULL);
  output_path = g_strconcat (logger->logger_config[LOGGER_PATH], output_file_name, NULL);

  while ((dir_file = g_dir_read_name (dir)) != NULL)
  {
    if (!g_strcmp0 (output_file_name, dir_file))
    { 
      stat (output_path, &file_info);
      if (logger->max_bytes <= file_info.st_size)
      {
        full = TRUE;
        break;
      }
    }
  }
  g_dir_rewind (dir);

  if (full)
  {
    fclose (logger->output_fp);
    logger->output_fp = NULL;
    ext_num_str = logger_util_itoa (logger->backup_count);
    target_file = g_strconcat (output_path, ".", ext_num_str, NULL);
    if (g_file_test (target_file, G_FILE_TEST_EXISTS))
    {
      g_remove (target_file);
    }
    g_free (ext_num_str);
    g_free (target_file);

    for (i = logger->backup_count - 1; i > 0; i--)
    {
      ext_num_str = logger_util_itoa (i);
      target_file = g_strconcat (output_path, ".", ext_num_str, NULL);
      g_free (ext_num_str);
      ext_num_str = logger_util_itoa (i + 1);
      renamed_file = g_strconcat (output_path, ".", ext_num_str, NULL);
      g_free (ext_num_str);
      if (g_file_test (target_file, G_FILE_TEST_EXISTS)) 
      {
        g_rename (target_file, renamed_file);
      }
      g_free (target_file);
      g_free (renamed_file);
    }
    renamed_file = g_strconcat (output_path, ".1", NULL);
    g_rename (output_path, renamed_file);
    g_free (renamed_file);
  }

  g_free (output_file_identifier);
  output_file_identifier = NULL;
  g_free (output_file_name);
  output_file_name = NULL;

  return output_path;
}

int
lsf_logger_print (Logger     *logger,
                  char       *log_level,
                  const char *file_name,
                  const int   line_num,
                  const char *func_name,
                  char       *format,
                  ...)
{
  GDateTime *local_time;
  va_list    args;
  GError    *error = NULL;
  gchar     *output_path = NULL;
  gchar     *log_asctime = NULL;
  gchar     *log_format_string = NULL;
  gchar     *output_string = NULL;
  GDir      *dir = NULL;
  char       ch;
  char       tmp[2];
  int        log_file_cnt = 0;
  int        ret_val = LOGGER_PRINT_SUCCESS;
  int        i;
  mode_t     mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH;

  if (logger == NULL)
  {
    return LOGGER_PRINT_FAILURE_LOGGER_IS_NULL;
  }

  if (logger_get_log_level_index (log_level) < logger->log_level_index)
  {
    return -1;
  }

  local_time = g_date_time_new_now_local ();
  log_asctime = g_date_time_format (local_time, "%F %H:%M:%S ");
  log_asctime = g_strconcat (log_asctime, "(", logger_util_itoa (g_date_time_get_microsecond (local_time)), ")", NULL);

  dir = g_dir_open (logger->logger_config[LOGGER_PATH], 0, &error);
  if (error != NULL)
  {
    g_error_free (error);
    ret_val = LOGGER_PRINT_FAILURE_CANNOT_OPEN_DIR;
  }

  output_path = logger_get_log_file_output_path (logger, dir, local_time);

  log_format_string = logger_get_log_format_string (logger);

  if (logger->output_fp == NULL)
  {
    logger->output_fp = fopen (output_path, "a");
  }

  if (logger->output_fp != NULL)
  {
    i = 0;
    while (ch = log_format_string[i++])
    {
      if (ch == '%')
      {
        ch = log_format_string[i++];
        switch (ch)
        {
          case 't':
            fputs (log_asctime, logger->output_fp);
            break;
          case 'l':
            fputs (log_level, logger->output_fp);
            break;
          case 'f':
            fputs (file_name, logger->output_fp);
            break;
          case 'L':
            fprintf (logger->output_fp, "%d", line_num);
            break;
          case 'F':
            fputs (func_name, logger->output_fp);
            break;
          case 'm':
            va_start (args, format);
            output_string = g_strdup_vprintf (format, args);
            fputs (output_string, logger->output_fp);
            fputc ('\n', logger->output_fp);
            va_end (args);
            g_free (output_string);
          default:
            break;
        }
      }
      else
      {
        fputc (ch, logger->output_fp);
      }
    }
    fflush (logger->output_fp);
  }
  else
  {
    ret_val = LOGGER_PRINT_FAILURE_CANNOT_OPEN_FILE;
  }
  utime (output_path, NULL);

  g_free (log_asctime);
  log_asctime = NULL;
  g_free (output_path);
  output_path = NULL;
  g_free (log_format_string);
  log_format_string = NULL;

  g_date_time_unref (local_time);
  g_dir_close (dir);

  return ret_val;
}
