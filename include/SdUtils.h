
#pragma once

#include <SD.h>
#include <FS.h>
#include "mbedtls/md.h"

namespace HmacUtils {

const uint16_t keyLen = 4;
const uint8_t key[keyLen] = {0x40, 0x84, 0xA4, 0x03};
#define HMAC_SIZE 20

void hmac_sha1(uint8_t* payload, uint16_t len, uint8_t* hmacResult) {

  mbedtls_md_context_t ctx;
  mbedtls_md_type_t md_type = MBEDTLS_MD_SHA1;

  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 1);
  mbedtls_md_hmac_starts(&ctx, key, keyLen);
  mbedtls_md_hmac_update(&ctx, payload, len);
  mbedtls_md_hmac_finish(&ctx, hmacResult);
  mbedtls_md_free(&ctx);

}

}

namespace SdUtils {

bool readFileSecure(const char* path, uint8_t* dest, uint16_t len) {

  File file = SD.open(path, FILE_READ);
  if(!file) return false;

  uint16_t fileSize = file.size();
  if(fileSize != len + HMAC_SIZE) return false;

  uint8_t* payload = new uint8_t[len];
  uint8_t hmacExpected[HMAC_SIZE];
  uint8_t hmacActual[HMAC_SIZE];

  uint16_t result = 0;
  result += file.readBytes((char*)payload, len);
  result += file.readBytes((char*)hmacExpected, HMAC_SIZE);
  file.close();

  bool ok = result == fileSize;
  if(ok) {
    HmacUtils::hmac_sha1(payload, len, hmacActual);
    ok = memcmp(hmacExpected, hmacActual, HMAC_SIZE) == 0;

    if(ok) memcpy(dest, payload, len);

  }

  delete[] payload;
  return ok;

}

bool writeFileSecure(const char* path, uint8_t* payload, uint16_t len) {

  uint8_t hmac[HMAC_SIZE];
  HmacUtils::hmac_sha1(payload, len, hmac);

  File file = SD.open(path, FILE_WRITE);
  if(!file) return false;

  uint16_t result = 0;
  result += file.write(payload, len);
  result += file.write(hmac, HMAC_SIZE);
  file.close();

  if(result != len + HMAC_SIZE) return false;

  ///////////////////////////////////////////////////////////

  file = SD.open(path, FILE_READ);
  if(!file) return false;

  uint16_t fileSize = file.size();
  if(fileSize != len + HMAC_SIZE) return false;

  char* payloadActual = new char[len];
  char hmacActual[HMAC_SIZE];

  result = 0;
  result += file.readBytes(payloadActual, len);
  result += file.readBytes(hmacActual, HMAC_SIZE);
  file.close();

  bool ok = result == fileSize;
  if(ok) {
    ok = memcmp(payload, payloadActual, len) == 0;
    if(ok) ok = memcmp(hmac, hmacActual, HMAC_SIZE) == 0;
  }

  delete[] payloadActual;
  return ok;

}

















bool writeFile(const char* path, uint8_t* payload, uint16_t len) {

  File file = SD.open(path, FILE_WRITE);
  if(!file) return false;

  uint16_t result = file.write(payload, len);
  file.close();

  return result == len;
}

/*
void writeFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if(!file){
    Serial.println("Failed to open file for writing");
    return;
  }
  if(file.print(message)){
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  file.close();
}
*/

/*

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
}*/

}
