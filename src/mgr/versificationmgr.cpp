/******************************************************************************
 *
 *  versificationmgr.cpp -	implementation of class VersificationMgr used
 *				for managing versification systems
 *
 * $Id$
 *
 * Copyright 2008-2013 CrossWire Bible Society (http://www.crosswire.org)
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

#include <versificationmgr.h>
#include <vector>
#include <map>
#include <treekey.h>
#include <canon.h>		// KJV internal versification system
#include <swlog.h>
#include <algorithm>

#include <canon_null.h>		// null v11n system

#include <canon_leningrad.h>	// Leningrad Codex (WLC) v11n system
#include <canon_mt.h>		// Masoretic Text (MT) v11n system
#include <canon_kjva.h>		// KJV + Apocrypha v11n system
#include <canon_nrsv.h>		// NRSV v11n system
#include <canon_nrsva.h>	// NRSV + Apocrypha v11n system
#include <canon_synodal.h>	// Russian Synodal v11n system
#include <canon_vulg.h>		// Vulgate v11n system
#include <canon_german.h>	// German v11n system
#include <canon_luther.h>	// Luther v11n system
#include <canon_catholic.h>	// Catholic v11n system (10 chapter Esther)
#include <canon_catholic2.h>	// Catholic2 v11n system (16 chapter Esther)
#include <canon_rahlfs.h>	// Rahlfs LXX v11n system
//#include <canon_lxx_nt.h>	// Rahlfs LXX + NTv11n system
#include <canon_lxx.h>		// General LXX v11n system (includes GNT, as used in Orthodox Bibles)
#include <canon_orthodox.h>	// Orthodox v11n system (includes GNT, as used in Orthodox Bibles)

using std::vector;
using std::map;
using std::distance;
using std::lower_bound;

SWORD_NAMESPACE_START


VersificationMgr *VersificationMgr::getSystemVersificationMgr() {
	if (!systemVersificationMgr) {
		systemVersificationMgr = new VersificationMgr();
		systemVersificationMgr->registerVersificationSystem("KJV", otbooks, ntbooks, vm);
		systemVersificationMgr->registerVersificationSystem("Leningrad", otbooks_leningrad, ntbooks_null, vm_leningrad);
		systemVersificationMgr->registerVersificationSystem("MT", otbooks_mt, ntbooks_null, vm_mt);
		systemVersificationMgr->registerVersificationSystem("KJVA", otbooks_kjva, ntbooks, vm_kjva);
		systemVersificationMgr->registerVersificationSystem("NRSV", otbooks, ntbooks, vm_nrsv);
		systemVersificationMgr->registerVersificationSystem("NRSVA", otbooks_nrsva, ntbooks, vm_nrsva);
		systemVersificationMgr->registerVersificationSystem("Synodal", otbooks_synodal, ntbooks_synodal, vm_synodal);
		systemVersificationMgr->registerVersificationSystem("Vulg", otbooks_vulg, ntbooks_vulg, vm_vulg);
		systemVersificationMgr->registerVersificationSystem("German", otbooks_german, ntbooks, vm_german);
		systemVersificationMgr->registerVersificationSystem("Luther", otbooks_luther, ntbooks_luther, vm_luther);
		systemVersificationMgr->registerVersificationSystem("Catholic", otbooks_catholic, ntbooks, vm_catholic);
		systemVersificationMgr->registerVersificationSystem("Catholic2", otbooks_catholic2, ntbooks, vm_catholic2);
		systemVersificationMgr->registerVersificationSystem("Rahlfs", otbooks_rahlfs, ntbooks_null, vm_rahlfs);
		//systemVersificationMgr->registerVersificationSystem("LXX_NT", otbooks_rahlfs, ntbooks, vm_lxx_nt);
		systemVersificationMgr->registerVersificationSystem("LXX", otbooks_lxx, ntbooks, vm_lxx);
		systemVersificationMgr->registerVersificationSystem("Orthodox", otbooks_orthodox, ntbooks, vm_orthodox);
	}
	return systemVersificationMgr;
}


class VersificationMgr::System::Private {
public:
	/** Array[chapmax] of maximum verses in chapters */
	vector<Book> books;
	map<SWBuf, int> osisLookup;

	Private() {
	}
	Private(const VersificationMgr::System::Private &other) {
		books = other.books;
		osisLookup = other.osisLookup;
	}
	VersificationMgr::System::Private &operator =(const VersificationMgr::System::Private &other) {
		books = other.books;
		osisLookup = other.osisLookup;
		return *this;
	}
};


class VersificationMgr::Book::Private {
friend struct BookOffsetLess;
public:
	/** Array[chapmax] of maximum verses in chapters */
	vector<int> verseMax;
	vector<long> offsetPrecomputed;

	Private() {
		verseMax.clear();
	}
	Private(const VersificationMgr::Book::Private &other) {
		verseMax.clear();
		verseMax = other.verseMax;
		offsetPrecomputed = other.offsetPrecomputed;
	}
	VersificationMgr::Book::Private &operator =(const VersificationMgr::Book::Private &other) {
		verseMax.clear();
		verseMax = other.verseMax;
		offsetPrecomputed = other.offsetPrecomputed;
		return *this;
	}
};

struct BookOffsetLess {
	bool operator() (const VersificationMgr::Book &o1, const VersificationMgr::Book &o2) const { return o1.p->offsetPrecomputed[0] < o2.p->offsetPrecomputed[0]; }
	bool operator() (const long &o1, const VersificationMgr::Book &o2) const { return o1 < o2.p->offsetPrecomputed[0]; }
	bool operator() (const VersificationMgr::Book &o1, const long &o2) const { return o1.p->offsetPrecomputed[0] < o2; }
	bool operator() (const long &o1, const long &o2) const { return o1 < o2; }
};

void VersificationMgr::Book::init() {
	p = new Private();
}

void VersificationMgr::System::init() {
	p = new Private();
	BMAX[0] = 0;
	BMAX[1] = 0;
	ntStartOffset = 0;
}


VersificationMgr::System::System(const System &other) {
	init();
	name = other.name;
	BMAX[0] = other.BMAX[0];
	BMAX[1] = other.BMAX[1];
	(*p) = *(other.p);
	ntStartOffset = other.ntStartOffset;
}

VersificationMgr::System &VersificationMgr::System::operator =(const System &other) {
	name = other.name;
	BMAX[0] = other.BMAX[0];
	BMAX[1] = other.BMAX[1];
	(*p) = *(other.p);
	ntStartOffset = other.ntStartOffset;
	return *this;
}


VersificationMgr::System::~System() {
	delete p;
}

const VersificationMgr::Book *VersificationMgr::System::getBook(int number) const {
	return (number < (signed int)p->books.size()) ? &(p->books[number]) : 0;
}


int VersificationMgr::System::getBookNumberByOSISName(const char *bookName) const {
	map<SWBuf, int>::const_iterator it = p->osisLookup.find(bookName);
	return (it != p->osisLookup.end()) ? it->second : -1;
}


void VersificationMgr::System::loadFromSBook(const sbook *ot, const sbook *nt, int *chMax) {
	int chap = 0;
	int book = 0;
	long offset = 0;	// module heading
	offset++;			// testament heading
	while (ot->chapmax) {
		p->books.push_back(Book(ot->name, ot->osis, ot->prefAbbrev, ot->chapmax));
		offset++;		// book heading
		Book &b = p->books[p->books.size()-1];
		p->osisLookup[b.getOSISName()] = p->books.size();
		for (int i = 0; i < ot->chapmax; i++) {
			b.p->verseMax.push_back(chMax[chap]);
			offset++;		// chapter heading
			b.p->offsetPrecomputed.push_back(offset);
			offset += chMax[chap++];
		}
		ot++;
		book++;
	}
	BMAX[0] = book;
	book = 0;
	ntStartOffset = offset;
	offset++;			// testament heading
	while (nt->chapmax) {
		p->books.push_back(Book(nt->name, nt->osis, nt->prefAbbrev, nt->chapmax));
		offset++;		// book heading
		Book &b = p->books[p->books.size()-1];
		p->osisLookup[b.getOSISName()] = p->books.size();
		for (int i = 0; i < nt->chapmax; i++) {
			b.p->verseMax.push_back(chMax[chap]);
			offset++;		// chapter heading
			b.p->offsetPrecomputed.push_back(offset);
			offset += chMax[chap++];
		}
		nt++;
		book++;
	}
	BMAX[1] = book;

	// TODO: build offset speed array
}


VersificationMgr::Book::Book(const Book &other) {
	longName = other.longName;
	osisName = other.osisName;
	prefAbbrev = other.prefAbbrev;
	chapMax = other.chapMax;
	init();
	(*p) = *(other.p);
}

VersificationMgr::Book& VersificationMgr::Book::operator =(const Book &other) {
	longName = other.longName;
	osisName = other.osisName;
	prefAbbrev = other.prefAbbrev;
	chapMax = other.chapMax;
	init();
	(*p) = *(other.p);
	return *this;
}


VersificationMgr::Book::~Book() {
	delete p;
}


int VersificationMgr::Book::getVerseMax(int chapter) const {
	chapter--;
	return (p && (chapter < (signed int)p->verseMax.size()) && (chapter > -1)) ? p->verseMax[chapter] : -1;
}


int VersificationMgr::System::getBookCount() const {
	return (p ? p->books.size() : 0);
}


long VersificationMgr::System::getOffsetFromVerse(int book, int chapter, int verse) const {
	long  offset = -1;
	chapter--;

	const Book *b = getBook(book);

	if (!b)                                        return -1;	// assert we have a valid book
	if ((chapter > -1) && (chapter >= (signed int)b->p->offsetPrecomputed.size())) return -1;	// assert we have a valid chapter

	offset = b->p->offsetPrecomputed[(chapter > -1)?chapter:0];
	if (chapter < 0) offset--;

/* old code
 *
	offset = offsets[testament-1][0][book];
	offset = offsets[testament-1][1][(int)offset + chapter];
	if (!(offset|verse)) // if we have a testament but nothing else.
		offset = 1;

*/

	return (offset + verse);
}


char VersificationMgr::System::getVerseFromOffset(long offset, int *book, int *chapter, int *verse) const {

	if (offset < 1) {	// just handle the module heading corner case up front (and error case)
		(*book) = -1;
		(*chapter) = 0;
		(*verse) = 0;
		return offset;	// < 0 = error
	}

	// binary search for book
	vector<Book>::iterator b = lower_bound(p->books.begin(), p->books.end(), offset, BookOffsetLess());
	if (b == p->books.end()) b--;
	(*book)    = distance(p->books.begin(), b)+1;
	if (offset < (*(b->p->offsetPrecomputed.begin()))-((((!(*book)) || (*book)==BMAX[0]+1))?2:1)) { // -1 for chapter headings
		(*book)--;
		if (b != p->books.begin()) {
			b--;	
		}
	}
	vector<long>::iterator c = lower_bound(b->p->offsetPrecomputed.begin(), b->p->offsetPrecomputed.end(), offset);

	// if we're a book heading, we are lessthan chapter precomputes, but greater book.  This catches corner case.
	if (c == b->p->offsetPrecomputed.end()) {
		c--;
	}
	if ((offset < *c) && (c == b->p->offsetPrecomputed.begin())) {
		(*chapter) = (offset - *c)+1;	// should be 0 or -1 (for testament heading)
		(*verse) = 0;
	}
	else {
		if (offset < *c) c--;
		(*chapter) = distance(b->p->offsetPrecomputed.begin(), c)+1;
		(*verse)   = (offset - *c);
	}
	return ((*chapter > 0) && (*verse > b->getVerseMax(*chapter))) ? KEYERR_OUTOFBOUNDS : 0;
}


/***************************************************
 * VersificationMgr
 */

class VersificationMgr::Private {
public:
	Private() {
	}
	Private(const VersificationMgr::Private &other) {
		systems = other.systems;
	}
	VersificationMgr::Private &operator =(const VersificationMgr::Private &other) {
		systems = other.systems;
		return *this;
	}
	map<SWBuf, System> systems;
};
// ---------------- statics -----------------
VersificationMgr *VersificationMgr::systemVersificationMgr = 0;

class __staticsystemVersificationMgr {
public:
	__staticsystemVersificationMgr() { }
	~__staticsystemVersificationMgr() { delete VersificationMgr::systemVersificationMgr; }
} _staticsystemVersificationMgr;


void VersificationMgr::init() {
	p = new Private();
}


VersificationMgr::~VersificationMgr() {
	delete p;
}


void VersificationMgr::setSystemVersificationMgr(VersificationMgr *newVersificationMgr) {
	if (systemVersificationMgr)
		delete systemVersificationMgr;
	systemVersificationMgr = newVersificationMgr;
}


const VersificationMgr::System *VersificationMgr::getVersificationSystem(const char *name) const {
	map<SWBuf, System>::const_iterator it = p->systems.find(name);
	return (it != p->systems.end()) ? &(it->second) : 0;
}


void VersificationMgr::registerVersificationSystem(const char *name, const sbook *ot, const sbook *nt, int *chMax) {
	p->systems[name] = name;
	System &s = p->systems[name];
	s.loadFromSBook(ot, nt, chMax);
}


void VersificationMgr::registerVersificationSystem(const char *name, const TreeKey *tk) {
}


const StringList VersificationMgr::getVersificationSystems() const {
	StringList retVal;
	for (map<SWBuf, System>::const_iterator it = p->systems.begin(); it != p->systems.end(); it++) {
		retVal.push_back(it->first);
	}
	return retVal;
}


SWORD_NAMESPACE_END
