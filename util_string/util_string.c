/***************************************************************************//**
 * @file util_string.c
 * @brief String (char array) conversion functionality.
 * @version 1.1
 * @author Brecht Van Eeckhoudt
 *
 * ******************************************************************************
 *
 * @section Versions
 *
 *   @li v1.0: Implemented `charASCII_to_charHex`, `uint32_to_charDec`,
 *             `stringStartsWith` and `stringLength`.
 *   @li v1.1: Implemented `uint32_to_charHex`.
 *
 * ******************************************************************************
 *
 * @section License
 *
 *   **Copyright (C) 2019 - Brecht Van Eeckhoudt**
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the **GNU General Public License** as published by
 *   the Free Software Foundation, either **version 3** of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   *A copy of the GNU General Public License can be found in the `LICENSE`
 *   file along with this source code.*
 *
 *   @n
 *
 *   Some methods use code obtained from examples from [Silicon Labs' GitHub](https://github.com/SiliconLabs/peripheral_examples).
 *   These sections are licensed under the Silabs License Agreement. See the file
 *   "Silabs_License_Agreement.txt" for details. Before using this software for
 *   any purpose, you must agree to the terms of that agreement.
 *
 ******************************************************************************/


#include <stdint.h>      /* (u)intXX_t */
#include <stdbool.h>     /* "bool", "true", "false" */
#include "util_string.h" /* Corresponding header file */


/**************************************************************************//**
 * @brief
 *   Convert an ASCII char array (string) to a hexadecimal char array.
 *
 * @note
 *   If the input is not a string (ex.: `"Hello world!"`) but a char array,
 *   the input message (array) needs to end with NULL (`"\0"`)!
 *
 * @param[in] inbuf
 *   The given ASCII string to convert to a hexadecimal string.
 *
 * @param[out] outbuf
 *   The buffer to put the resulting string in.@n
 *   **This needs to have twice the length of the input message + 1!**
 *   (two HEX characters for one ASCII character + NULL termination)
 *****************************************************************************/
void charASCII_to_charHex (char *inbuf, char *outbuf)
{
	/* Lookup table array */
	char hex_lut[] = "0123456789abcdef";

	/* Calculate length of the given string */
	uint8_t length = stringLength(inbuf);

	/* Convert each character */
	for (uint16_t i = 0; i < length; i++)
	{
		/* Only get necessary nibble by ANDing with a mask and
		 * shifting one nibble (4 bits) per position */
		outbuf[(i*2)]   = hex_lut[(inbuf[i] & 0xF0) >> 4 ];
		outbuf[(i*2)+1] = hex_lut[(inbuf[i] & 0x0F)      ];
	}

	outbuf[length*2] = '\0'; /* NULL termination character */
}


/**************************************************************************//**
 * @brief
 *   Convert a `uint32_t` value to a decimal char array (string).
 *
 * @param[in] value
 *   The `uint32_t` value to convert to a string.
 *
 * @param[out] outbuf
 *   The buffer to put the resulting string in.@n
 *   **This needs to have a length of 10: `char buf[10];`!**
 *****************************************************************************/
void uint32_to_charDec (uint32_t value, char *outbuf)
{
	if (value == 0)
	{
		outbuf[0] = '0';
		outbuf[1] = '\0'; /* NULL termination character */
	}
	else
	{
		/* MAX uint32_t value = FFFFFFFFh = 4294967295d (10 decimal chars) */
		char backwardsBuf[10];

		uint32_t calcval = value;
		uint8_t length = 0;
		uint8_t lengthCounter = 0;


		/* Loop until the value is zero (separate characters 0-9) and calculate length */
		while (calcval)
		{
			uint32_t rem = calcval % 10;
			backwardsBuf[length] = '0' + rem; /* Convert to ASCII character */
			length++;

			calcval = calcval - rem;
			calcval = calcval / 10;
		}

		/* Backwards counter */
		lengthCounter = length;

		/* Reverse the characters in the buffer for the final string */
		for (uint8_t i = 0; i < length; i++)
		{
			outbuf[i] = backwardsBuf[lengthCounter-1];
			lengthCounter--;
		}

		/* Add NULL termination character */
		outbuf[length] = '\0';
	}
}


/**************************************************************************//**
 * @brief
 *   Convert a `uint32_t` value to a hexadecimal char array (string).
 *
 * @param[in] value
 *   The `uint32_t` value to convert to a string.
 *
 * @param[out] outbuf
 *   The buffer to put the resulting string in.@n
 *   **This needs to have a length of 9: `char buf[9];`!**
 *****************************************************************************/
void uint32_to_charHex (uint32_t value, char *outbuf)
{
	/* Lookup table array */
	char hex_lut[] = "0123456789abcdef";

	/* 4 nibble HEX representation */
	if (value <= 0xFFFF)
	{
		/* Only get necessary nibble by ANDing with a mask and
		 * shifting one nibble (4 bits) per position */
		outbuf[0] = hex_lut[(value & 0xF000) >> 12];
		outbuf[1] = hex_lut[(value & 0x0F00) >> 8 ];
		outbuf[2] = hex_lut[(value & 0x00F0) >> 4 ];
		outbuf[3] = hex_lut[(value & 0x000F)      ];
		outbuf[4] = '\0'; /* NULL termination character */
	}

	/* 8 nibble HEX representation */
	else
	{
		/* Only get necessary nibble by ANDing with a mask and
		 * shifting one nibble (4 bits) per position */
		outbuf[0] = hex_lut[(value & 0xF0000000) >> 28];
		outbuf[1] = hex_lut[(value & 0x0F000000) >> 24];
		outbuf[2] = hex_lut[(value & 0x00F00000) >> 20];
		outbuf[3] = hex_lut[(value & 0x000F0000) >> 16];
		outbuf[4] = hex_lut[(value & 0x0000F000) >> 12];
		outbuf[5] = hex_lut[(value & 0x00000F00) >> 8 ];
		outbuf[6] = hex_lut[(value & 0x000000F0) >> 4 ];
		outbuf[7] = hex_lut[(value & 0x0000000F)      ];
		outbuf[8] = '\0'; /* NULL termination character */
	}
}


/**************************************************************************//**
 * @brief
 *   Check if a given char array (string) starts with a given sequence.
 *
 * @note
 *   If the input is not a string (ex.: `"Hello world!"`) but a char array,
 *   the input message (array) needs to end with NULL (`"\0"`)!
 *
 * @param[in] inbuf
 *   The given char array (string) to check a sequence in.
 *
 * @param[in] sequence
 *   The sequence to check in the given char array.
 *
 * @return
 *   @li `true` - The given sequence is found in the char array.
 *   @li `false` -The given sequence is not found in the char array.
 *****************************************************************************/
bool stringStartsWith (char *inbuf, char *sequence)
{
	/* Go through each character of "sequence" */
	for (uint8_t i = 0; i < stringLength(sequence); i++ )
	{
		/* Return false if the value at the memory location (pointer, '*') of
		 * (inbuf+i) isn't equal to the value at the memory location of (sequence+i) */
		if (*(inbuf+i) != *(sequence+i)) return (false);
	}

	return (true);
}


/**************************************************************************//**
 * @brief
 *   Returns the length of the given null-terminated byte string.
 *
 * @param[in] inbuf
 *   The given char array (string).
 *****************************************************************************/
uint8_t stringLength (char *inbuf)
{
	uint8_t length = 0;

	/* Count up until we reach a NULL */
	while (inbuf[length] != '\0') length++;

	return (length);
}
