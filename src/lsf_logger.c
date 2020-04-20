#include "lsf_logger.h"

Logger *
lsf_get_logger (char *logger_name)
{ 
  int i; 

  if (logger_name == NULL)
  {
    logger_name = LSF_LOGGER_DEFAULT_NAME;
  }
  else if ((i = lsf_logger_exist (logger_name)) != -1)
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
  if (!lsf_logger_init (logger[i]))
  {
    return NULL;
  }

  return logger[i];
}

int
lsf_logger_exist (char *logger_name)
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

gboolean
lsf_logger_init (Logger *logger)
{
  GKeyFile *key_file = NULL;
  int       i;
  
  key_file = g_key_file_new ();

  g_key_file_load_from_file (key_file, LSF_LOGGER_CONFIG, G_KEY_FILE_NONE, NULL);

  logger->logger_config[LOGGER_PATH] = g_key_file_get_string (key_file, "LOG", "PATH", NULL);
  logger->logger_config[LOGGER_FMT] = g_key_file_get_string (key_file, "LOG", "FORMAT", NULL);
  logger->logger_config[LOGGER_LEVEL] = g_key_file_get_string (key_file, "LOG", "LEVEL", NULL);
  logger->logger_config[LOGGER_MAX_BYTES] = g_key_file_get_string (key_file, "LOG", "MAX_BYTES", NULL);
  logger->logger_config[LOGGER_BACKUP_COUNT] = g_key_file_get_string (key_file, "LOG", "BACKUP_COUNT", NULL);
  logger->logger_config[LOGGER_FILE_EXTENSION] = g_key_file_get_string (key_file, "LOG", "FILE_EXTENSION", NULL);

  g_key_file_free (key_file);

  for (i = 0; i < LOGGER_CONFIG_NUMS; i++)
  {
    if (logger->logger_config[i] == NULL)
    {
      return FALSE;
    }
  }
  return TRUE;
}

int
lsf_logger_get_log_level_index (char *log_level)
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

void
lsf_logger_free (Logger *logger)
{
  int i;

  if (logger == NULL)
    return;

  for (i = 0; i < LOGGER_CONFIG_NUMS; i++)
  {
    g_free (logger->logger_config[i]);
    logger->logger_config[i] = NULL;
  }
  free (logger);
  logger = NULL;
  logger_cnt--;
}

void
lsf_logger_free_all (void)
{
  int i;

  for (i = 0; i < LOGGER_MAX; i++)
  {
    if (logger_cnt && logger[i] != NULL)
    {
      lsf_logger_free (logger[i]);
    }
  }
}

gchar *
lsf_logger_get_log_format_string (Logger *logger)
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
        case 'T':
          if (!g_strcmp0 (word, "time") || !g_strcmp0 (word, "TIME"))
          {
            arg[a++] = 't';
          }
          break;
        case 'f':
        case 'F':
          if (!g_strcmp0 (word, "file") || !g_strcmp0 (word, "FILE"))
          {
            arg[a++] = 'f';
          }
          else if (!g_strcmp0 (word, "func") || !g_strcmp0 (word, "FUNC"))
          {
            arg[a++] = 'F';
          }
          break;
        case 'l':
        case 'L':
          if (!g_strcmp0 (word, "level") || !g_strcmp0 (word, "LEVEL"))
          {
            arg[a++] = 'l';
          }
          else if (!g_strcmp0 (word, "line") || !g_strcmp0 (word, "LINE"))
          {
            arg[a++] = 'L';
          }
          break;
        case 'm':
        case 'M':
          if (!g_strcmp0 (word, "message") || !g_strcmp0 (word, "MESSAGE"))
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
  va_list args;
  GError *error = NULL;
  gchar  *output_path = NULL;
  gchar  *log_asctime = NULL;
  gchar  *first_log_file = NULL;
  gchar  *log_format_string = NULL;
  gchar  *output_string = NULL;
  GDir   *dir = NULL;
  FILE   *output_fp = NULL;
  char    ch;
  int     log_file_cnt = 0;
  int     backup_cnt;
  int     ret_val = LOGGER_PRINT_SUCCESS;
  int     i;

  backup_cnt = atoi (logger->logger_config[LOGGER_BACKUP_COUNT]);

  if (lsf_logger_get_log_level_index (log_level) < lsf_logger_get_log_level_index (logger->logger_config[LOGGER_LEVEL]))
  {
    return -1;
  }

  local_time = g_date_time_new_now_local ();
  log_asctime = g_date_time_format (local_time, "%F %H:%M:%S ");
  log_asctime = g_strconcat (log_asctime, "(", lsf_logger_util_itoa (g_date_time_get_microsecond (local_time)), ")", NULL);

  dir = g_dir_open (logger->logger_config[LOGGER_PATH], 0, &error);
  if (error != NULL)
  {
    g_error_free (error);
    ret_val = LOGGER_PRINT_FAILURE_CANNOT_OPEN_DIR;
  }

  output_path = lsf_logger_get_log_file_output_path (logger, dir, local_time);

  log_format_string = lsf_logger_get_log_format_string (logger);

  output_fp = fopen (output_path, "a");
  if (output_fp != NULL)
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
            fprintf (output_fp, "%s", log_asctime);
            break;
          case 'l':
            fprintf (output_fp, "%s", log_level);
            break;
          case 'f':
            fprintf (output_fp, "%s", file_name);
            break;
          case 'L':
            fprintf (output_fp, "%d", line_num);
            break;
          case 'F':
            fprintf (output_fp, "%s", func_name);
            break;
          case 'm':
            va_start (args, format);
            output_string = g_strdup_vprintf (format, args);
            fprintf (output_fp, output_string);
            fprintf (output_fp, "\n");
            va_end (args);
            g_free (output_string);
          default:
            break;
        }
      }
      else
      {
        fprintf (output_fp, "%c", ch);
      }
    }
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
  fclose (output_fp);

  log_file_cnt = lsf_logger_get_log_file_cnt (logger, dir);

  while (log_file_cnt > backup_cnt)
  {
    first_log_file = lsf_logger_get_log_file_first (logger, dir);
    g_remove (first_log_file);
    log_file_cnt--;
    g_free (first_log_file);
    first_log_file = NULL;
  }

  g_date_time_unref (local_time);
  g_dir_close (dir);

  return ret_val;
}

int
lsf_logger_get_log_file_cnt (Logger *logger,
                             GDir   *dir)
{
  const char *dir_file;
  int         log_file_cnt = 0;

  while ((dir_file = g_dir_read_name (dir)) != NULL)
  {
    if (g_str_has_prefix (dir_file, logger->logger_config[LOGGER_NAME]))
    {
      log_file_cnt++;
    }
  }
  g_dir_rewind (dir);

  return log_file_cnt;
}

gchar *
lsf_logger_get_log_file_first (Logger *logger,
                               GDir   *dir)
{
  struct stat file_info;
  const char *dir_file;
  time_t      mtime = -1;
  gchar      *first_log_file = NULL;
  gchar      *log_file = NULL;
  int         log_file_cnt = 0;

  while ((dir_file = g_dir_read_name (dir)) != NULL)
  {
    if (g_str_has_prefix (dir_file, logger->logger_config[LOGGER_NAME]))
    {
      log_file = g_strconcat (logger->logger_config[LOGGER_PATH], dir_file, NULL);
      stat (log_file, &file_info);
      if (mtime == -1)
      {
        mtime = file_info.st_mtime;
        first_log_file = g_strdup (dir_file);
      }
      else
      {
        if (mtime > file_info.st_mtime)
        {
          mtime = file_info.st_mtime;
          g_free (first_log_file);
          first_log_file = g_strdup (dir_file);
        } 
      }
      g_free (log_file);
    }
  }
  g_dir_rewind (dir);

  return first_log_file = g_strconcat (logger->logger_config[LOGGER_PATH], first_log_file, NULL);
}

gchar *
lsf_logger_util_itoa (int num)
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

gchar *
lsf_logger_get_log_file_output_path (Logger    *logger,
                                     GDir      *dir,
                                     GDateTime *local_time)
{
  struct stat file_info;
  const char *dir_file;
  GError     *error = NULL;
  gchar      *output_file_identifier = NULL;
  gchar      *output_file_name = NULL;
  gchar      *first_log_file = NULL;
  gchar      *output_path = NULL;
  gchar      *ext_num_str = NULL;
  gchar      *renamed = NULL;
  int         backup_cnt;
  int         max_bytes;
  int         i;

  max_bytes = atoi (logger->logger_config[LOGGER_MAX_BYTES]);
  backup_cnt = atoi (logger->logger_config[LOGGER_BACKUP_COUNT]);

  output_file_identifier = g_date_time_format (local_time, "%F");
  output_file_name = g_strconcat (logger->logger_config[LOGGER_NAME], "-", output_file_identifier, ".", logger->logger_config[LOGGER_FILE_EXTENSION], NULL);
  output_path = g_strconcat (logger->logger_config[LOGGER_PATH], output_file_name, NULL);

  while ((dir_file = g_dir_read_name (dir)) != NULL)
  {
    if (!g_strcmp0 (output_file_name, dir_file))
    { 
      stat (output_path, &file_info);
      if (max_bytes <= file_info.st_size)
      {
        for (i = 1; i <= backup_cnt; i++)
        {
          ext_num_str = lsf_logger_util_itoa (i);
          renamed = g_strconcat (output_path, ".", ext_num_str, NULL);
          if (!g_file_test (renamed, G_FILE_TEST_EXISTS)) 
          {
            g_rename (output_path, renamed);
          }
          g_free (ext_num_str);
          g_free (renamed);
        }
      }
    }
  }
  g_dir_rewind (dir);

  g_free (output_file_identifier);
  output_file_identifier = NULL;
  g_free (output_file_name);
  output_file_name = NULL;

  return output_path;
}
