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

#include "sherlock/sherlock.h"
#include "sherlock/fixed_text.h"

namespace Sherlock {

static const Common::String fixedTextEN[] = {
	// Window buttons
	"Exit",
	"Up",
	"Down"
};

// sharp-s       : 0xE1 / octal 341
// small a-umlaut: 0x84 / octal 204
// small o-umlaut: 0x94 / octal 224
// small u-umlaut: 0x81 / octal 201
static const Common::String fixedTextDE[] = {
	// Window buttons
	"Zur\201ck",
	"Hoch",
	"Runter"
};

static const Common::String fixedTextES[] = {
	// Window buttons
	"Exit",
	"Subir",
	"Bajar"
};

static const SherlockFixedTextLanguageEntry fixedTextLanguages[] = {
	{ Common::DE_DEU,   fixedTextDE },
	{ Common::ES_ESP,   fixedTextES },
	{ Common::EN_ANY,   fixedTextEN },
	{ Common::UNK_LANG, fixedTextEN }
};


FixedText::FixedText(SherlockEngine *vm) : _vm(vm) {
	// Figure out which fixed texts to use
	Common::Language curLanguage = _vm->getLanguage();

	const SherlockFixedTextLanguageEntry *curLanguageEntry = fixedTextLanguages;

	while (curLanguageEntry->language != Common::UNK_LANG) {
		if (curLanguageEntry->language == curLanguage)
			break; // found current language
		curLanguageEntry++;
	}
	_fixedText = curLanguageEntry->fixedTextArray;
}

const Common::String FixedText::getText(FixedTextId fixedTextId) {
	return _fixedText[fixedTextId];
}


} // End of namespace Sherlock
