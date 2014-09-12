#include <stdbool.h>
#include <string.h>

#include <smem.h>

#include <utils.h>

char *make_config_path(char *root_directory, char *path) {
  if(root_directory == NULL) {
    return strdup(path);
  }

  size_t config_path_len;
  size_t root_directory_len = strlen(root_directory);
  size_t path_len           = strlen(path);

  bool chop_root_directory = false;
  bool chop_path           = false;
  bool add_slash           = false;

  if(root_directory[root_directory_len - 1] == '/') {
    if(path[0] == '/') {
      config_path_len = root_directory_len + path_len - 1;
      chop_path       = true;
    } else {
      config_path_len = root_directory_len + path_len;
    }
  } else {
    if(path[0] == '/') {
      config_path_len = root_directory_len + path_len;
    } else {
      config_path_len = root_directory_len + path_len + 1;
      add_slash       = true;
    }
  }

  char *config_path = (char*)smalloc(config_path_len + 1);

  size_t to_write =
    chop_root_directory ? root_directory_len - 1 : root_directory_len;
  size_t so_far = 0;
  memcpy(config_path, root_directory, to_write);
  so_far += to_write;

  if(add_slash) {
    config_path[so_far++] = '/';
  }

  if(chop_path) {
    memcpy(config_path + so_far, path + 1, path_len - 1);
  } else {
    memcpy(config_path + so_far, path, path_len);
  }

  config_path[config_path_len] = '\0';

  return config_path;
}
