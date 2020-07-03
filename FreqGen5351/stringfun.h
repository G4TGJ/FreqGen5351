/*
 * stringfun.h
 *
 * Created: 03/07/2020 13:16:25
 * Author : Richard Tomlinson G4TGJ
 */ 

#ifndef STRINGFUN_H_
#define STRINGFUN_H_

#include <inttypes.h>
#include <ctype.h>

uint32_t convertToUint32( char *num, uint8_t n );
void convertFromUint32( char *buf, uint8_t len, uint32_t number, bool bShort );

#endif /* STRINGFUN_H_ */