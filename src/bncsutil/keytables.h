
/**
 * BNCSutil
 * Battle.Net Utility Library
 *
 * Copyright (C) 2004-2006 Eric Naeseth
 *
 * WarCraft II/Diablo II and WarCraft III Decoding Tables
 * October 2, 2004
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License is included in the BNCSutil
 * distribution in the file COPYING.  If you did not receive this copy,
 * write to the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA  02111-1307  USA
 */

#ifndef BNCSUTIL_KEYTABLES_H
#define BNCSUTIL_KEYTABLES_H

#ifdef __cplusplus
extern "C" {
#endif

	const unsigned char w2Map[] = {
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0x00, 0xFF, 0x01, 0xFF, 0x02, 0x03, 0x04, 0x05, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B,
		0x0C, 0xFF, 0x0D, 0x0E, 0xFF, 0x0F, 0x10, 0xFF, 0x11, 0xFF, 0x12, 0xFF,
		0x13, 0xFF, 0x14, 0x15, 0x16, 0xFF, 0x17, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0xFF, 0x0D, 0x0E,
		0xFF, 0x0F, 0x10, 0xFF, 0x11, 0xFF, 0x12, 0xFF, 0x13, 0xFF, 0x14, 0x15,
		0x16, 0xFF, 0x17, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF
	};

	const unsigned char w3KeyMap[] = {
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0x00, 0xFF, 0x01, 0xFF, 0x02, 0x03, 0x04, 0x05, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x06, 0x07, 0x08, 0x09, 0x0A,
		0x0B, 0x0C, 0xFF, 0x0D, 0x0E, 0xFF, 0x0F, 0x10, 0xFF, 0x11, 0xFF, 0x12,
		0xFF, 0x13, 0xFF, 0x14, 0x15, 0x16, 0x17, 0x18, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0xFF, 0x0D,
		0x0E, 0xFF, 0x0F, 0x10, 0xFF, 0x11, 0xFF, 0x12, 0xFF, 0x13, 0xFF, 0x14,
		0x15, 0x16, 0x17, 0x18, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF
	};

	const unsigned char w3TranslateMap[] = {
		0x09, 0x04, 0x07, 0x0F, 0x0D, 0x0A, 0x03, 0x0B, 0x01, 0x02, 0x0C, 0x08,
		0x06, 0x0E, 0x05, 0x00, 0x09, 0x0B, 0x05, 0x04, 0x08, 0x0F, 0x01, 0x0E,
		0x07, 0x00, 0x03, 0x02, 0x0A, 0x06, 0x0D, 0x0C, 0x0C, 0x0E, 0x01, 0x04,
		0x09, 0x0F, 0x0A, 0x0B, 0x0D, 0x06, 0x00, 0x08, 0x07, 0x02, 0x05, 0x03,
		0x0B, 0x02, 0x05, 0x0E, 0x0D, 0x03, 0x09, 0x00, 0x01, 0x0F, 0x07, 0x0C,
		0x0A, 0x06, 0x04, 0x08, 0x06, 0x02, 0x04, 0x05, 0x0B, 0x08, 0x0C, 0x0E,
		0x0D, 0x0F, 0x07, 0x01, 0x0A, 0x00, 0x03, 0x09, 0x05, 0x04, 0x0E, 0x0C,
		0x07, 0x06, 0x0D, 0x0A, 0x0F, 0x02, 0x09, 0x01, 0x00, 0x0B, 0x08, 0x03,
		0x0C, 0x07, 0x08, 0x0F, 0x0B, 0x00, 0x05, 0x09, 0x0D, 0x0A, 0x06, 0x0E,
		0x02, 0x04, 0x03, 0x01, 0x03, 0x0A, 0x0E, 0x08, 0x01, 0x0B, 0x05, 0x04,
		0x02, 0x0F, 0x0D, 0x0C, 0x06, 0x07, 0x09, 0x00, 0x0C, 0x0D, 0x01, 0x0F,
		0x08, 0x0E, 0x05, 0x0B, 0x03, 0x0A, 0x09, 0x00, 0x07, 0x02, 0x04, 0x06,
		0x0D, 0x0A, 0x07, 0x0E, 0x01, 0x06, 0x0B, 0x08, 0x0F, 0x0C, 0x05, 0x02,
		0x03, 0x00, 0x04, 0x09, 0x03, 0x0E, 0x07, 0x05, 0x0B, 0x0F, 0x08, 0x0C,
		0x01, 0x0A, 0x04, 0x0D, 0x00, 0x06, 0x09, 0x02, 0x0B, 0x06, 0x09, 0x04,
		0x01, 0x08, 0x0A, 0x0D, 0x07, 0x0E, 0x00, 0x0C, 0x0F, 0x02, 0x03, 0x05,
		0x0C, 0x07, 0x08, 0x0D, 0x03, 0x0B, 0x00, 0x0E, 0x06, 0x0F, 0x09, 0x04,
		0x0A, 0x01, 0x05, 0x02, 0x0C, 0x06, 0x0D, 0x09, 0x0B, 0x00, 0x01, 0x02,
		0x0F, 0x07, 0x03, 0x04, 0x0A, 0x0E, 0x08, 0x05, 0x03, 0x06, 0x01, 0x05,
		0x0B, 0x0C, 0x08, 0x00, 0x0F, 0x0E, 0x09, 0x04, 0x07, 0x0A, 0x0D, 0x02,
		0x0A, 0x07, 0x0B, 0x0F, 0x02, 0x08, 0x00, 0x0D, 0x0E, 0x0C, 0x01, 0x06,
		0x09, 0x03, 0x05, 0x04, 0x0A, 0x0B, 0x0D, 0x04, 0x03, 0x08, 0x05, 0x09,
		0x01, 0x00, 0x0F, 0x0C, 0x07, 0x0E, 0x02, 0x06, 0x0B, 0x04, 0x0D, 0x0F,
		0x01, 0x06, 0x03, 0x0E, 0x07, 0x0A, 0x0C, 0x08, 0x09, 0x02, 0x05, 0x00,
		0x09, 0x06, 0x07, 0x00, 0x01, 0x0A, 0x0D, 0x02, 0x03, 0x0E, 0x0F, 0x0C,
		0x05, 0x0B, 0x04, 0x08, 0x0D, 0x0E, 0x05, 0x06, 0x01, 0x09, 0x08, 0x0C,
		0x02, 0x0F, 0x03, 0x07, 0x0B, 0x04, 0x00, 0x0A, 0x09, 0x0F, 0x04, 0x00,
		0x01, 0x06, 0x0A, 0x0E, 0x02, 0x03, 0x07, 0x0D, 0x05, 0x0B, 0x08, 0x0C,
		0x03, 0x0E, 0x01, 0x0A, 0x02, 0x0C, 0x08, 0x04, 0x0B, 0x07, 0x0D, 0x00,
		0x0F, 0x06, 0x09, 0x05, 0x07, 0x02, 0x0C, 0x06, 0x0A, 0x08, 0x0B, 0x00,
		0x0F, 0x04, 0x03, 0x0E, 0x09, 0x01, 0x0D, 0x05, 0x0C, 0x04, 0x05, 0x09,
		0x0A, 0x02, 0x08, 0x0D, 0x03, 0x0F, 0x01, 0x0E, 0x06, 0x07, 0x0B, 0x00,
		0x0A, 0x08, 0x0E, 0x0D, 0x09, 0x0F, 0x03, 0x00, 0x04, 0x06, 0x01, 0x0C,
		0x07, 0x0B, 0x02, 0x05, 0x03, 0x0C, 0x04, 0x0A, 0x02, 0x0F, 0x0D, 0x0E,
		0x07, 0x00, 0x05, 0x08, 0x01, 0x06, 0x0B, 0x09, 0x0A, 0x0C, 0x01, 0x00,
		0x09, 0x0E, 0x0D, 0x0B, 0x03, 0x07, 0x0F, 0x08, 0x05, 0x02, 0x04, 0x06,
		0x0E, 0x0A, 0x01, 0x08, 0x07, 0x06, 0x05, 0x0C, 0x02, 0x0F, 0x00, 0x0D,
		0x03, 0x0B, 0x04, 0x09, 0x03, 0x08, 0x0E, 0x00, 0x07, 0x09, 0x0F, 0x0C,
		0x01, 0x06, 0x0D, 0x02, 0x05, 0x0A, 0x0B, 0x04, 0x03, 0x0A, 0x0C, 0x04,
		0x0D, 0x0B, 0x09, 0x0E, 0x0F, 0x06, 0x01, 0x07, 0x02, 0x00, 0x05, 0x08
	};

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* BNCSUTIL_KEYTABLES_H */