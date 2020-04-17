#include "lsf_logger.h"

Logger *
lsf_get_logger (char *logger_name)
{ 
  int i; if ((i = lsf_logger_exist (logger_name)) != -1)
  { return logger[i];
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
  lsf_logger_init (logger[i]);

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
  GKeyFile *key_file = g_key_file_new ();
  
  g_key_file_load_from_file (key_file, CONFIG_FILE_PATH, G_KEY_FILE_NONE, NULL);

  logger->logger_config[LOGGER_PATH] = g_key_file_get_string (key_file, "LOG", "PATH", NULL);
  logger->logger_config[LOGGER_FMT] = g_key_file_get_string (key_file, "LOG", "FORMAT", NULL);
  logger->logger_config[LOGGER_LEVEL] = g_key_file_get_string (key_file, "LOG", "LEVEL", NULL);
  logger->logger_config[LOGGER_MAX_BYTES] = g_key_file_get_string (key_file, "LOG", "MAX_BYTES", NULL);
  logger->logger_config[LOGGER_BACKUP_COUNT] = g_key_file_get_string (key_file, "LOG", "BACKUP_COUNT", NULL);
  logger->logger_config[LOGGER_FILE_EXTENSION] = g_key_file_get_string (key_file, "LOG", "FILE_EXTENSION", NULL);

  g_key_file_free (key_file);
}

int
lsf_logger_get_log_level_index (char *log_level)
{
  if (!g_strcmp0 (log_level, "CRITICAL"))
  {
    return LOGGER_LEVEL_CRITICAL;
  }
  else if (!g_strcmp0 (log_level, "ERROR"))
  {
    return LOGGER_LEVEL_ERROR;
  }
  else if (!g_strcmp0 (log_level, "WARNING"))
  {
    return LOGGER_LEVEL_WARNING;
  }
  else if (!g_strcmp0 (log_level, "INFO"))
  {
    return LOGGER_LEVEL_INFO;
  }
  else if (!g_strcmp0 (log_level, "DEBUG"))
  {
    return LOGGER_LEVEL_DEBUG;
  }
  else
  {
    return LOGGER_LEVEL_ALL;
  }
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
lsf_logger_free_all ()
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
  int a, i, j;

  a = i = 0;
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
lsf_logger_print (Logger *logger, char *log_level, const char *file_name, const int line_num, const char *func_name, char *format, ...)
{
  GError *error = NULL;
  FILE *output_fp;
  gchar *output_path;
  GDateTime *local_time;
  gchar *log_asctime;
  gchar *first_log_file;
  gchar *log_format_string;
  gchar *output_string;
  GDir *dir;
  int ret_val = 0;
  int i;
  char ch;
  va_list args;
  int level_index;
  int log_file_cnt = 0;

  if (lsf_logger_get_log_level_index (log_level) < lsf_logger_get_log_level_index (logger->logger_config[LOGGER_LEVEL]))
  {
    return -1;
  }

  local_time = g_date_time_new_now_local ();
  log_asctime = g_date_time_format (local_time, "%F %H:%M:%S");

  dir = g_dir_open (logger->logger_config[LOGGER_PATH], 0, &error);
  if (error != NULL)
  {
    g_error_free (error);
    ret_val = -1;
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
    ret_val = -1;
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

  while (log_file_cnt > atoi (logger->logger_config[LOGGER_BACKUP_COUNT]))
  {
    first_log_file = lsf_logger_get_log_file_first (logger, dir);
    g_print ("First log file: %s\n", first_log_file);
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
lsf_logger_get_log_file_cnt (Logger *logger, GDir *dir)
{
  const char *dir_file;
  int log_file_cnt = 0;

  while ((dir_file = g_dir_read_name (dir)) != NULL)
  {
    if (g_str_has_prefix (dir_file, logger->logger_config[LOGGER_NAME]) &&
        g_str_has_suffix (dir_file, logger->logger_config[LOGGER_FILE_EXTENSION]))
    {
      log_file_cnt++;
    }
  }
  g_dir_rewind (dir);

  return log_file_cnt;
}

gchar *
lsf_logger_get_log_file_first (Logger *logger, GDir *dir)
{
  const char *dir_file;
  int log_file_cnt = 0;
  time_t mtime = -1;
  gchar *first_log_file;
  struct stat file_info;

  while ((dir_file = g_dir_read_name (dir)) != NULL)
  {
    if (g_str_has_prefix (dir_file, logger->logger_config[LOGGER_NAME]))
        //&& g_str_has_suffix (dir_file, logger->logger_config[LOGGER_FILE_EXTENSION]))
    {
      stat (dir_file, &file_info);
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
    }
  }
  g_dir_rewind (dir);
  g_print ("%s\n", first_log_file);

  return first_log_file = g_strconcat (logger->logger_config[LOGGER_PATH], first_log_file, NULL);
}

gchar *
lsf_logger_itoa (int num)
{
  char buf[2][10];
  int i = 0;
  int j = 0;

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
lsf_logger_get_log_file_output_path (Logger *logger, GDir *dir, GDateTime *local_time)
{
  GError *error = NULL;
  gchar *output_file_identifier;
  gchar *output_file_name;
  gchar *output_path;
  gchar *renamed;
  gchar *ext_num_str;
  struct stat file_info;
  const char *dir_file;
  int i;

  output_file_identifier = g_date_time_format (local_time, "%F");
  output_file_name = g_strconcat (logger->logger_config[LOGGER_NAME], "-", output_file_identifier, ".", logger->logger_config[LOGGER_FILE_EXTENSION], NULL);
  output_path = g_strconcat (logger->logger_config[LOGGER_PATH], output_file_name, NULL);

  while ((dir_file = g_dir_read_name (dir)) != NULL)
  {
    if (!g_strcmp0 (output_file_name, dir_file))
    { 
      stat (output_path, &file_info);
      if (atoi (logger->logger_config[LOGGER_MAX_BYTES]) <= file_info.st_size)
      {
        for (i = 1; i < atoi (logger->logger_config[LOGGER_BACKUP_COUNT]); i++)
        {
          ext_num_str = lsf_logger_itoa (i);
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
