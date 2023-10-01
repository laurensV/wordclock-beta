#include "filesystem.h"

String load_from_file(String file_name) {
  String result = "";

  File this_file = LittleFS.open(file_name, "r");
  if (!this_file) { // failed to open the file, return empty result
    return result;
  }
  while (this_file.available()) {
    result += (char)this_file.read();
  }

  this_file.close();
  return result;
}