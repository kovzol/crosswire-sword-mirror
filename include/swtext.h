/******************************************************************************
 *  swtext.h   - code for base class 'SWText'.  SWText is the basis for all
 *		 types of text modules
 *
 * $Id: swtext.h,v 1.14 2003/02/28 13:12:43 mgruner Exp $
 *
 * Copyright 1998 CrossWire Bible Society (http://www.crosswire.org)
 *	CrossWire Bible Society
 *	P. O. Box 2528
 *	Tempe, AZ  85280-2528
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation version 2.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 */

#ifndef SWTEXT_H
#define SWTEXT_H

#include <swmodule.h>
#include <versekey.h>
#include <listkey.h>

#include <defs.h>

SWORD_NAMESPACE_START

/** The basis for all text modules
 */
class SWDLLEXPORT SWText : public SWModule {
public:
	/** Initializes data for instance of SWText
	*/
	SWText(const char *imodname = 0, const char *imoddesc = 0,
			SWDisplay * idisp = 0,
			SWTextEncoding encoding = ENC_UNKNOWN,
			SWTextDirection dir = DIRECTION_LTR,
			SWTextMarkup markup = FMT_UNKNOWN, const char* ilang = 0);

	virtual ~SWText();
	/** Create the correct key (VerseKey) for use with SWText
	*/
	virtual SWKey *CreateKey();

	virtual long Index() const;
	virtual long Index(long iindex);

	// OPERATORS -----------------------------------------------------------------
	
	SWMODULE_OPERATORS

};

SWORD_NAMESPACE_END

#endif
