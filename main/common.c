/*
 * common.c
 *
 *  Created on: Jun 21, 2022
 *      Author: Thanh Vu
 */

#include "common.h"
#include "./jsonUser/json_user.h"

void get_device_infor(Device * _device, char * brokerInfor)
{
	char buff[513];
	readfromfile("deviceinfor", buff);
	JSON_analyze_post(buff, _device->id, _device->token, brokerInfor);
	if (strlen(brokerInfor) == 0) {
		memcpy(brokerInfor, BROKER, strlen(BROKER) + 1);
	}
}

inline int8_t hexValue(char c) {
  if ((c >= '0') && (c <= '9')) {
    return c - '0';
  }
  if ((c >= 'A') && (c <= 'F')) {
    return 10 + c - 'A';
  }
  if ((c >= 'a') && (c <= 'f')) {
    return 10 + c - 'a';
  }
  return -1;
}

uint32_t mParseHex(char *data, size_t max_len) {
  uint32_t ret = 0;
  for (uint8_t i = 0; i < max_len; i++) {
    int8_t v = hexValue(data[i]);
    if (v < 0) { // non hex digit, we stop parsing
      continue;
    }
    ret = (ret << 4) | v;

  }
  return ret;
}
