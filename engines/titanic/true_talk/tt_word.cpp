/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include "titanic/true_talk/tt_word.h"
#include "titanic/true_talk/tt_string_node.h"
#include "titanic/titanic.h"

namespace Titanic {

TTword::TTword(TTString &str, int mode, int val2) : _string(str),
		_wordMode(mode), _field1C(val2), _pNext(nullptr), _synP(nullptr),
		_field20(0), _field24(0), _field28(0) {
	_status = str.getStatus() == SS_VALID ? SS_VALID : SS_5;
}

TTword::TTword(TTword *src) {
	if (src->getStatus() != SS_VALID) {
		_status = SS_5;
		return;
	}

	_string = src->_string;
	_wordMode = src->_wordMode;
	_field1C = src->_field1C;
	_field20 = src->_field20;
	_synP = nullptr;

	TTsynonym *priorSyn = nullptr;
	for (TTsynonym *synP = _synP; synP && !_status;) {
		TTsynonym *newSyn = new TTsynonym(synP);
		if (!newSyn) {
			_status = SS_7;
		} else {
			newSyn->_pPrior = priorSyn;
			newSyn->_pNext = nullptr;

			if (priorSyn) {
				priorSyn->_pNext = newSyn;
			} else {
				_synP = newSyn;
			}

			priorSyn = newSyn;
		}
	}

	_pNext = src->_pNext;
	_field24 = src->_field24;
	_field28 = src->_field28;
}

void TTword::deleteSiblings() {
	while (_pNext) {
		TTword *next = _pNext;
		_pNext = next->_pNext;
		delete next;
	}
}

int TTword::readSyn(SimpleFile *file) {
	CString str;
	int mode, val1;

	if (!file->scanf("%s %d %d", &str, &mode, &val1))
		return 8;
	if (!testFileHandle(file))
		return 5;

	// Create new synanym node
	TTsynonym *synNode = new TTsynonym(mode, str.c_str(), (FileHandle)val1);

	if (_synP) {
		// A synonym already exists, so add new one as a tail
		// at the end of the linked list of synonyms
		_synP->addNode(synNode);
	} else {
		// Very first synonym, so set it
		_synP = synNode;
	}

	return 0;
}

void TTword::appendNode(TTsynonym *node) {
	if (_synP)
		_synP->addNode(node);
	else
		_synP = node;
}

int TTword::load(SimpleFile *file, int mode) {
	CString str1, str2;
	int val;

	if (file->scanf("%d %s %s", &val, &str1, &str2)) {
		_string = str1;
		_field1C = val;
		_field20 = readNumber(str2.c_str());
		_wordMode = mode;
		return 0;
	} else {
		return 3;
	}
}

uint TTword::readNumber(const char *str) {
	uint numValue = *str;
	if (*str == '0') {
		numValue = MKTAG('Z', 'Z', 'Z', '[');
	} else {
		++str;
		for (int idx = 0; idx < 3; ++idx, ++str)
			numValue = (numValue << 8) + *str;
	}

	return numValue;
}

bool TTword::testFileHandle(FileHandle file) const {
	if (g_vm->_exeResources.is18Equals(3))
		return true;

	// TODO: Figure out why original compares passed file handle against specific values
	return true;
}

TTword *TTword::scanCopy(const TTString &str, TTsynonym *node, int mode) {
	if (_synP) {
		TTsynonym *strNode = TTsynonym::findByName(_synP, str, mode);
		if (strNode) {
			node->copy(strNode);
			node->_pPrior = nullptr;
			node->_pNext = nullptr;
		}
	}

	return nullptr;
}

TTword *TTword::copy() {
	return new TTword(this);
}

FileHandle TTword::getSynFile() const {
	return _synP ? _synP->_file : HANDLE_STDIN;
}

bool TTword::checkSynFile(FileHandle file) const {
	return _synP && _synP->_file == file;
}

void TTword::setSynFile(FileHandle file) {
	if (_synP && testFileHandle(file))
		_synP->_file = file;
}


/*------------------------------------------------------------------------*/

TTword1::TTword1(TTString &str, int val1, int val2, int val3) :
		TTword(str, val1, val2), _field2C(val3) {
}

/*------------------------------------------------------------------------*/

TTword2::TTword2(TTString &str, int val1, int val2, int val3, int val4) :
		TTword1(str, val1, val2, val3), _field30(val4) {
}

int TTword2::load(SimpleFile *file) {
	int val;

	if (!TTword::load(file, 1) && file->scanf("%d", &val)) {
		_field30 = val;
		return 0;
	} else {
		return 8;
	}
}

/*------------------------------------------------------------------------*/

TTword3::TTword3(TTString &str, int val1, int val2, int val3, int val4, int val5, int val6) :
		TTword1(str, val1, val2, val4), _field34(val3), _field30(val5), _field3C(val6),
		_field38(0) {
}

int TTword3::load(SimpleFile *file) {
	CString str;
	int val1, val2;

	if (!TTword::load(file, 2) && file->scanf("%s %d %d", &str, &val1, &val2)) {
		_field34 = readNumber(str.c_str());
		_field30 = val1;
		_field3C = val2;
		return 0;
	} else {
		return 3;
	}
}

/*------------------------------------------------------------------------*/

TTword4::TTword4(TTString &str, int val1, int val2, int val3, int val4) :
		TTword1(str, val1, val2, val3) {
	if (val4 >= 0 && val4 <= 9) {
		_field30 = val4;
	} else {
		_field30 = 0;
		_status = SS_5;
	}
}

int TTword4::load(SimpleFile *file) {
	int val;

	if (!TTword::load(file, 8) && file->scanf("%d", &val)) {
		_field30 = val;
		return 0;
	} else {
		return 8;
	}
}

/*------------------------------------------------------------------------*/

TTword5::TTword5(TTString &str, int val1, int val2, int val3, int val4) :
	TTword1(str, val1, val2, val3), _field30(val4) {
}

int TTword5::load(SimpleFile *file) {
	int val;

	if (!TTword::load(file, 6) && file->scanf("%d", &val)) {
		if (val >= 0 && val <= 12) {
			_field30 = val;
			return 0;
		} else {
			return 5;
		}
	} else {
		return 8;
	}
}

} // End of namespace Titanic
