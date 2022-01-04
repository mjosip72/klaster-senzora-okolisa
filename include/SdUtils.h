
#pragma once

#include <FS.h>

namespace SdUtils {

void printLvl(uint8_t lvl) {
  for(uint8_t i = 0; i < lvl * 3; i++) {
    Serial.write(' ');
  }
}

void listDir(FS &fs, const char* dirPath, uint8_t levels, uint8_t lvl = 0) {

  printLvl(lvl);
  Serial.printf("Listing directory: %s\n", dirPath);

  File dir = fs.open(dirPath);

  if(!dir) {
    printLvl(lvl);
    Serial.println("Failed to open directory");
    return;
  }

  if(!dir.isDirectory()) {
    printLvl(lvl);
    Serial.println("Not a directory");
    return;
  }

  File file;

  while(true) {

    file = dir.openNextFile();
    if(!file) break;

    if(file.isDirectory()) {
      printLvl(lvl);
      Serial.printf("Dir: %s\n", file.name());
      if(levels) listDir(fs, file.name(), levels - 1, lvl + 1);
    }else{
      printLvl(lvl);
      Serial.printf("File: %s\n", file.name());
    }


  }

}


void listDir_ORIG(fs::FS &fs, const char * dirname, uint8_t levels){
    Serial.printf("Listing directory: %s\n", dirname);

    File root = fs.open(dirname);
    if(!root){
        Serial.println("Failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println("Not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if(levels){
                listDir(fs, file.name(), levels -1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("  SIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}



}
