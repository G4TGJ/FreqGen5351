/*
 * stringfun.c
 *
 * Created: 03/07/2020 13:16:40
 * Author : Richard Tomlinson G4TGJ
 */ 

 #include "config.h"
 #include "stringfun.h"

// Convert n characters into a uint32
// Expects leading zeroes
// An incorrect format returns zero
uint32_t convertToUint32( char *num, uint8_t n )
{
    uint8_t i;
    uint32_t result = 0;
    
    for( i = 0 ; i < n ; i++ )
    {
        // First check the character is a decimal digit
        if( isdigit(num[i]) )
        {
            // Convert from ASCII to decimal and shift into the running total
            result = (num[i]-'0') + (result * 10);
        }
        else
        {
            // Not a digit so return 0
            result = 0;
            break;
        }
    }

    return result;
}

// Convert a uint32 into a string
// This is for the display and will be right justified
// This is intended for a frequency with a maximum of
// 200MHz.
//
// If bShort is true then only output 4 characters
// otherwise convert the whole number
//
// len is the length of the buffer and we won't write beyond it
//
// If there are only 4 characters available then
// can only handle 3 digits plus an M or K
// (or 4 digits for the lowest frequencies)
void convertFromUint32( char *buf, uint8_t len, uint32_t number, bool bShort )
{
    uint32_t divider;
    uint8_t pos;
    
    // Set to true once we have started converting digits
    bool bStarted = false;

    // Start by writing out leading spaces
    for( pos = 0 ; pos < (len-9) ; pos++ )
    {
        buf[pos] = ' ';
    }
    
    // Maximum number is 200 000 000
    // We want to convert digits starting at the left
    for( divider = 100000000 ; (divider > 0) && (pos < len) ; divider /= 10 )
    {
        // Get the current digit
        uint8_t digit = number / divider;
        
        // If we have started converting or this is a digit
        if( bStarted || digit )
        {
            // Convert this digit
            buf[pos] = digit + '0';
            pos++;
            bStarted = true;
        }
        else if( !bShort )
        {
            // Insert a leading space if doing a full conversion
            // This right justifies the number
            buf[pos] = ' ';
            pos++;
        }
        
        // If we only have space for SHORT_WIDTH digits then stop after 4
        if( bShort && pos == SHORT_WIDTH )
        {
            // Need to decide whether to use M or K etc as a decimal point
            // If so, either insert it at the end, or in the middle
            // in which case we need to shift the digits to make space
            //
            // 123456789 becomes 123M
            //  12345678 becomes 12M3
            //   1234567 becomes 1M23
            //    123456 becomes 123K
            //     12345 becomes 12K3
            //      1234 becomes 1234
            //
            if( divider == 100000 )
            {
                buf[3] = 'M';
            }
            else if( divider == 10000 )
            {
                buf[3] = buf[2];
                buf[2] = 'M';
            }
            else if( divider == 1000 )
            {
                buf[3] = buf[2];
                buf[2] = buf[1];
                buf[1] = 'M';
            }
            else if( divider == 100 )
            {
                buf[3] = 'K';
            }
            else if( divider == 10 )
            {
                buf[3] = buf[2];
                buf[2] = 'K';
            }
            
            // Stop converting
            break;
        }
        
        // Now process the remainder
        number %= divider;
    }
    
    // If we haven't converted any digits then it must be zero
    if( !bStarted && (pos == len) )
    {
        buf[len - 1] = '0';
    }
}

