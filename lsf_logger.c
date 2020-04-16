#include "lsf_logger.h"

Logger *
lsf_get_logger (char *logger_name, char *config_path)
{
  int i;
  for (i = 0; i < LOGGER_MAX; i++)
  {
    if (logger[i] == NULL)
    {
      break;
    }
  }
  if ((logger[i] = lsf_logger_exist (logger_name)) != NULL)
  {
    return logger[i];
  }
  logger[i] = (Logger *) malloc (sizeof (Logger));
  logger_cnt++;

  logger[i]->logger_config[LOGGER_NAME] = g_strdup (logger_name);
  lsf_logger_init (logger[i], config_path);

  return logger[i];
}

Logger *
lsf_logger_exist (char *logger_name)
{
  int i;

  for (i = 0; i < LOGGER_MAX; i++)
  {
    if (logger[i] != NULL && !g_strcmp0 (logger_name, logger[i]->logger_config[LOGGER_NAME]))
    {
      return logger[i];
    }
  }
  return NULL;
}

gboolean
lsf_logger_init (Logger *logger, char *config_path)
{
  confer = lsf_get_confer (config_path);
  logger->logger_config[LOGGER_PATH] = lsf_confer_get_str ("LOG", "PATH");
  logger->logger_config[LOGGER_FMT] = lsf_confer_get_str ("LOG", "FORMAT");
  logger->logger_config[LOGGER_LEVEL] = lsf_confer_get_str ("LOG", "LEVEL");
  logger->logger_config[LOGGER_MAX_BYTES] = lsf_confer_get_str ("LOG", "MAX_BYTES");
  logger->logger_config[LOGGER_BACKUP_COUNT] = lsf_confer_get_str ("LOG", "BACKUP_COUNT");
  logger->logger_config[LOGGER_FILE_EXTENSION] = lsf_confer_get_str ("LOG", "FILE_EXTENSION");
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
  }
  free (logger);
  logger_cnt--;
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
      for (j = 0; j < LOGGER_FMT_WORD_NUM; j++)
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
  GDir *dir;
  int ret_val = 0;
  int i;
  char ch;

  local_time = g_date_time_new_now_local ();
  log_asctime = g_date_time_format (local_time, "%a %b %d %H:%M:%S %Y");

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
            fprintf (output_fp, format);
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

  g_free (log_asctime);
  g_free (output_path);
  g_free (log_format_string);

  if (lsf_logger_get_log_file_cnt (logger, dir) > atoi (logger->logger_config[LOGGER_BACKUP_COUNT]))
  {
    first_log_file = lsf_logger_get_log_file_first (logger, dir);
    g_remove (first_log_file);
    g_free (first_log_file);
  }

  fclose (output_fp);
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
  time_t mtime = 0;
  gchar *first_log_file;
  struct stat file_info;

  while ((dir_file = g_dir_read_name (dir)) != NULL)
  {
    if (g_str_has_prefix (dir_file, logger->logger_config[LOGGER_NAME]) &&
        g_str_has_suffix (dir_file, logger->logger_config[LOGGER_FILE_EXTENSION]))
    {
      stat (dir_file, &file_info);
      if (!mtime)
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

  return first_log_file;
}

gchar *
lsf_logger_get_log_file_output_path (Logger *logger, GDir *dir, GDateTime *local_time)
{
  GError *error = NULL;
  gchar *output_file_identifier;
  gchar *output_file_name;
  gchar *output_path;
  struct stat file_info;
  const char *dir_file;

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
        output_file_identifier = g_strconcat (output_file_identifier, "+", NULL);
        g_free (output_file_name);
        g_free (output_path);
        output_file_name = g_strconcat (logger->logger_config[LOGGER_NAME], "-", output_file_identifier, ".", logger->logger_config[LOGGER_FILE_EXTENSION], NULL);
        output_path = g_strconcat (logger->logger_config[LOGGER_PATH], output_file_name, NULL);
        g_dir_rewind (dir);
      }
    }
  }
  g_dir_rewind (dir);

  g_free (output_file_identifier);
  g_free (output_file_name);

  return output_path;
}
