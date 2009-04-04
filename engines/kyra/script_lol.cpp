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

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $URL$
 * $Id$
 *
 */

#ifdef ENABLE_LOL

#include "kyra/lol.h"
#include "kyra/screen_lol.h"
#include "kyra/timer.h"
#include "kyra/resource.h"

#include "common/endian.h"

namespace Kyra {

void LoLEngine::runInitScript(const char *filename, int optionalFunc) {
	_suspendScript = true;
	EMCData scriptData;
	EMCState scriptState;
	memset(&scriptData, 0, sizeof(EMCData));
	_emc->load(filename, &scriptData, &_opcodes);

	_emc->init(&scriptState, &scriptData);
	_emc->start(&scriptState, 0);
	while (_emc->isValid(&scriptState))
		_emc->run(&scriptState);

	if (optionalFunc) {
		_emc->init(&scriptState, &scriptData);
		_emc->start(&scriptState, optionalFunc);
		while (_emc->isValid(&scriptState))
			_emc->run(&scriptState);
	}

	_emc->unload(&scriptData);
	_suspendScript = false;
}

void LoLEngine::runInfScript(const char *filename) {
	_emc->load(filename, &_scriptData, &_opcodes);
	runLevelScript(0x400, -1);
}

void LoLEngine::runLevelScript(int block, int sub) {
	runLevelScriptCustom(block, sub, -1, 0, 0, 0);
}

void LoLEngine::runLevelScriptCustom(int block, int sub, int charNum, int item, int reg3, int reg4) {
	EMCState scriptState;
	memset(&scriptState, 0, sizeof(EMCState));

	if (!_suspendScript) {
		_emc->init(&scriptState, &_scriptData);
		_emc->start(&scriptState, block);

		scriptState.regs[0] = sub;
		scriptState.regs[1] = charNum;
		scriptState.regs[2] = item;
		scriptState.regs[3] = reg3;
		scriptState.regs[4] = reg4;
		scriptState.regs[5] = block;
		scriptState.regs[6] = _scriptDirection;

		if (_emc->isValid(&scriptState)) {
			if (*(scriptState.ip - 1) & sub) {
				while (_emc->isValid(&scriptState))
					_emc->run(&scriptState);
			}
		}
	}

	checkSceneUpdateNeed(block);
}

bool LoLEngine::checkSceneUpdateNeed(int func) {
	if (_sceneUpdateRequired)
		return true;

	for (int i = 0; i < 15; i++) {
		if (_visibleBlockIndex[i] == func) {
			_sceneUpdateRequired = true;
			return true;
		}
	}

	if (_currentBlock == func){
		_sceneUpdateRequired = true;
		return true;
	}

	return false;
}

int LoLEngine::olol_setWallType(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_setWallType(%p) (%d, %d, %d)", (const void *)script, stackPos(0), stackPos(1), stackPos(2));
	if (_wllWallFlags[stackPos(2)] & 4)
		deleteMonstersFromBlock(stackPos(0));
	setWallType(stackPos(0), stackPos(1), stackPos(2));
	return 1;
}

int LoLEngine::olol_getWallType(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_getWallType(%p) (%d, %d)", (const void *)script, stackPos(0), stackPos(1));
	return _levelBlockProperties[stackPos(0)].walls[stackPos(1) & 3];
}

int LoLEngine::olol_drawScene(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_drawScene(%p) (%d)", (const void *)script, stackPos(0));
	drawScene(stackPos(0));
	return 1;
}

int LoLEngine::olol_delay(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_delay(%p) (%d)", (const void *)script, stackPos(0));
	delay(stackPos(0) * _tickLength);
	return 1;
}

int LoLEngine::olol_setGameFlag(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_setGameFlag(%p) (%d, %d)", (const void *)script, stackPos(0), stackPos(1));
	if (stackPos(1))
		_gameFlags[stackPos(0) >> 4] |= (1 << (stackPos(0) & 0x0f));
	else
		_gameFlags[stackPos(0) >> 4] &= (~(1 << (stackPos(0) & 0x0f)));

	return 1;
}

int LoLEngine::olol_testGameFlag(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_testGameFlag(%p) (%d)", (const void *)script, stackPos(0));
	if (stackPos(0) < 0)
		return 0;

	if (_gameFlags[stackPos(0) >> 4] & (1 << (stackPos(0) & 0x0f)))
		return 1;

	return 0;
}

int LoLEngine::olol_loadLevelGraphics(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_loadLevelGraphics(%p) (%s, %d, %d, %d, %d, %d)", (const void *)script, stackPosString(0), stackPos(1), stackPos(2), stackPos(3), stackPos(4), stackPos(5));
	loadLevelGraphics(stackPosString(0), stackPos(1), stackPos(2), stackPos(3), stackPos(4), (stackPos(5) == -1) ? 0 : stackPosString(5));
	return 1;
}

int LoLEngine::olol_loadCmzFile(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_loadCmzFile(%p) (%s)", (const void *)script, stackPosString(0));
	loadCmzFile(stackPosString(0));
	return 1;
}

int LoLEngine::olol_loadMonsterShapes(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_loadMonsterShapes(%p) (%s, %d, %d)", (const void *)script, stackPosString(0), stackPos(1), stackPos(2));
	loadMonsterShapes(stackPosString(0), stackPos(1), stackPos(2));
	return 1;
}

int LoLEngine::olol_deleteHandItem(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_deleteHandItem(%p) ()", (const void *)script);
	int r = _itemInHand;
	deleteItem(_itemInHand);
	setHandItem(0);
	return r;
}

int LoLEngine::olol_allocItemPropertiesBuffer(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_allocItemPropertiesBuffer(%p) (%d)", (const void *)script, stackPos(0));
	delete[] _itemProperties;
	_itemProperties = new ItemProperty[stackPos(0)];
	return 1;
}

int LoLEngine::olol_setItemProperty(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_setItemProperty(%p) (%d, %d, %d, %d, %d, %d, %d, %d, %d, %d)", (const void *)script, stackPos(0), stackPos(1), stackPos(2), stackPos(3), stackPos(4), stackPos(5), stackPos(6), stackPos(7), stackPos(8), stackPos(9));
	ItemProperty *tmp = &_itemProperties[stackPos(0)];

	tmp->nameStringId = stackPos(1);
	tmp->shpIndex = stackPos(2);
	tmp->type = stackPos(3);
	tmp->itemScriptFunc = stackPos(4);
	tmp->might = stackPos(5);
	tmp->skill = stackPos(6);
	tmp->protection = stackPos(7);
	tmp->flags = stackPos(8);
	tmp->unkB = stackPos(9);
	return 1;
}

int LoLEngine::olol_makeItem(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_makeItem(%p) (%d, %d, %d)", (const void *)script, stackPos(0), stackPos(1), stackPos(2));
	return makeItem(stackPos(0), stackPos(1), stackPos(2));
}

int LoLEngine::olol_createLevelItem(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_setItemProperty(%p) (%d, %d, %d, %d, %d, %d, %d, %d)", (const void *)script, stackPos(0), stackPos(1), stackPos(2), stackPos(3), stackPos(4), stackPos(5), stackPos(6), stackPos(7));
	int item = makeItem(stackPos(0), stackPos(1), stackPos(2));
	if (item == -1)
		return item;
	placeMoveLevelItem(item, stackPos(3), stackPos(4), stackPos(5), stackPos(6), stackPos(7));
	return item;
}

int LoLEngine::olol_getItemPara(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_getItemPara(%p) (%d, %d)", (const void *)script, stackPos(0), stackPos(1));
	if (!stackPos(0))
		return 0;

	ItemInPlay *i = &_itemsInPlay[stackPos(0)];
	ItemProperty *p = &_itemProperties[i->itemPropertyIndex];

	switch (stackPos(1)) {
	case 0:
		return i->blockPropertyIndex;
	case 1:
		return i->x;
	case 2:
		return i->y;
	case 3:
		return i->level;
	case 4:
		return i->itemPropertyIndex;
	case 5:
		return i->shpCurFrame_flg;
	case 6:
		return p->nameStringId;
	case 7:
		break;
	case 8:
		return p->shpIndex;
	case 9:
		return p->type;
	case 10:
		return p->itemScriptFunc;
	case 11:
		return p->might;
	case 12:
		return p->skill;
	case 13:
		return p->protection;
	case 14:
		return p->unkB;
	case 15:
		return i->shpCurFrame_flg & 0x1fff;
	case 16:
		return p->flags;
	case 17:
		return (p->skill << 8) | p->might;
	default:
		break;
	}

	return -1;
}

int LoLEngine::olol_getCharacterStat(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_getCharacterStat(%p) (%d, %d, %d)", (const void *)script, stackPos(0), stackPos(1), stackPos(2));
	LoLCharacter *c = &_characters[stackPos(0)];
	int d = stackPos(2);

	switch (stackPos(1)) {
	case 0:
		return c->flags;

	case 1:
		return c->raceClassSex;

	case 5:
		return c->hitPointsCur;

	case 6:
		return c->hitPointsMax;

	case 7:
		return c->magicPointsCur;

	case 8:
		return c->magicPointsMax;

	case 9:
		return c->itemsProtection;

	case 10:
		return c->items[d];

	case 11:
		return c->skillLevels[d] + c->skillModifiers[d];

	case 12:
		return c->field_27[d];

	case 13:
		return (d & 0x80) ? c->itemsMight[7] : c->itemsMight[d];

	case 14:
		return c->skillModifiers[d];

	case 15:
		return c->id;

	default:
		break;
	}

	return 0;
}

int LoLEngine::olol_setCharacterStat(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_setCharacterStat(%p) (%d, %d, %d, %d)", (const void *)script, stackPos(0), stackPos(1), stackPos(2), stackPos(3));
	LoLCharacter *c = &_characters[stackPos(0)];
	int d = stackPos(2);
	int e = stackPos(3);

	switch (stackPos(1)) {
	case 0:
		c->flags = e;
		break;

	case 1:
		c->raceClassSex = e & 0x0f;
		break;

	case 5:
		//// TODO
		break;

	case 6:
		c->hitPointsMax = e;
		break;

	case 7:
		//// TODO
		break;

	case 8:
		c->magicPointsMax = e;
		break;

	case 9:
		c->itemsProtection = e;
		break;

	case 10:
		c->items[d] = 0;
		break;

	case 11:
		c->skillLevels[d] = e;
		break;

	case 12:
		c->field_27[d] = e;
		break;

	case 13:
		if (d & 0x80)
			c->itemsMight[7] = e;
		else
			c->itemsMight[d] = e;
		break;

	case 14:
		c->skillModifiers[d] = e;
		break;

	default:
		break;
	}

	return 0;
}

int LoLEngine::olol_loadLevelShapes(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_loadLevelShapes(%p) (%s, %s)", (const void *)script, stackPosString(0), stackPosString(1));
	loadLevelShpDat(stackPosString(0), stackPosString(1), true);
	return 1;
}

int LoLEngine::olol_closeLevelShapeFile(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_closeLevelShapeFile(%p) ()", (const void *)script);
	delete _lvlShpFileHandle;
	_lvlShpFileHandle = 0;
	return 1;
}

int LoLEngine::olol_loadDoorShapes(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_loadDoorShapes(%p) (%s, %d, %d)", (const void *)script, stackPosString(0), stackPos(1), stackPos(2));
	_screen->loadBitmap(stackPosString(0), 3, 3, 0);
	const uint8 *p = _screen->getCPagePtr(2);
	if (_doorShapes[0])
		delete[] _doorShapes[0];
	_doorShapes[0] = _screen->makeShapeCopy(p, stackPos(1));
	if (_doorShapes[1])
		delete[] _doorShapes[1];
	_doorShapes[1] = _screen->makeShapeCopy(p, stackPos(2));

	for (int i = 0; i < 20; i++) {
		_wllWallFlags[i + 3] |= 7;
		int t = i % 5;
		if (t == 4)
			_wllWallFlags[i + 3] &= 0xf8;
		if (t == 3)
			_wllWallFlags[i + 3] &= 0xfd;
	}

	if (stackPos(3)) {
		for (int i = 3; i < 13; i++)
			_wllWallFlags[i] &= 0xfd;
	}

	if (stackPos(4)) {
		for (int i = 13; i < 23; i++)
			_wllWallFlags[i] &= 0xfd;
	}

	return 1;
}

int LoLEngine::olol_initAnimStruct(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_initAnimStruct(%p) (%s, %d, %d, %d, %d, %d)", (const void *)script, stackPosString(0), stackPos(1), stackPos(2), stackPos(3), stackPos(4), stackPos(5));
	if (_tim->initAnimStruct(stackPos(1), stackPosString(0), stackPos(2), stackPos(3), stackPos(4), 0, stackPos(5)))
		return 1;
	return 0;
}

int LoLEngine::olol_playAnimationPart(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_playAnimationPart(%p) (%d, %d, %d, %d)", (const void *)script, stackPos(0), stackPos(1), stackPos(2), stackPos(3));
	_tim->playAnimationPart(stackPos(0), stackPos(1), stackPos(2), stackPos(3));
	return 1;
}

int LoLEngine::olol_freeAnimStruct(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_freeAnimStruct(%p) (%d)", (const void *)script, stackPos(0));
	if (_tim->freeAnimStruct(stackPos(0)))
		return 1;
	return 0;
}

int LoLEngine::olol_getDirection(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_getDirection(%p)", (const void *)script);
	return _currentDirection;
}

int LoLEngine::olol_setMusicTrack(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_setMusicTrack(%p) (%d)", (const void *)script, stackPos(0));
	_curMusicTheme = stackPos(0);
	return 1;
}

int LoLEngine::olol_setSequenceButtons(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_setSequenceButtons(%p) (%d, %d, %d, %d, %d)", (const void *)script, stackPos(0), stackPos(1), stackPos(2), stackPos(3), stackPos(4));
	setSequenceButtons(stackPos(0), stackPos(1), stackPos(2), stackPos(3), stackPos(4));
	return 1;
}

int LoLEngine::olol_setDefaultButtonState(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_setDefaultButtonState(%p)", (const void *)script);
	setDefaultButtonState();
	return 1;
}

int LoLEngine::olol_checkRectForMousePointer(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_checkRectForMousePointer(%p) (%d, %d, %d, %d)", (const void *)script, stackPos(0), stackPos(1), stackPos(2), stackPos(3));
	return posWithinRect(_mouseX, _mouseY, stackPos(0), stackPos(1), stackPos(2), stackPos(3)) ? 1 : 0;
}

int LoLEngine::olol_clearDialogueField(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_clearDialogueField(%p) (%d)", (const void *)script, stackPos(0));
	if (_currentControlMode && (!textEnabled()))
		return 1;

	_screen->setScreenDim(5);
	const ScreenDim *d = _screen->getScreenDim(5);
	_screen->fillRect(d->sx, d->sy, d->sx + d->w - 2, d->sy + d->h - 2, d->unkA);
	_txt->clearDim(4);
	_txt->resetDimTextPositions(4);

	return 1;
}

int LoLEngine::olol_setupBackgroundAnimationPart(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_setupBackgroundAnimationPart(%p) (%d, %d, %d, %d, %d, %d, %d, %d, %d, %d)", (const void *)script, stackPos(0), stackPos(1), stackPos(2), stackPos(3), stackPos(4), stackPos(5), stackPos(6), stackPos(7), stackPos(8), stackPos(9));
	_tim->setupBackgroundAnimationPart(stackPos(0), stackPos(1), stackPos(2), stackPos(3), stackPos(4), stackPos(5), stackPos(6), stackPos(7), stackPos(8), stackPos(9));
	return 0;
}

int LoLEngine::olol_startBackgroundAnimation(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_startBackgroundAnimation(%p) (%d, %d)", (const void *)script, stackPos(0), stackPos(1));
	_tim->startBackgroundAnimation(stackPos(0), stackPos(1));
	return 1;
}

int LoLEngine::olol_fadeToBlack(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_fadeToBlack(%p) (%d)", (const void *)script, stackPos(0));
	_screen->fadeToBlack(10);
	return 1;
}

int LoLEngine::olol_fadePalette(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_fadePalette(%p)", (const void *)script);
	_screen->fadePalette(_screen->getPalette(3), 10);
	_screen->_fadeFlag = 0;
	return 1;
}

int LoLEngine::olol_loadBitmap(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_clearDialogueField(%p) (%s, %d)", (const void *)script, stackPosString(0), stackPos(1));
	_screen->loadBitmap(stackPosString(0), 3, 3, _screen->getPalette(3));
	if (stackPos(1) != 2)
		_screen->copyPage(3, stackPos(1));
	return 1;
}

int LoLEngine::olol_stopBackgroundAnimation(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_stopBackgroundAnimation(%p) (%d)", (const void *)script, stackPos(0));
	_tim->stopBackgroundAnimation(stackPos(0));
	return 1;
}

int LoLEngine::olol_getGlobalScriptVar(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_getGlobalScriptVar(%p) (%d)", (const void *)script, stackPos(0));
	assert(stackPos(0) < 16);
	return _globalScriptVars[stackPos(0)];
}

int LoLEngine::olol_setGlobalScriptVar(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_setGlobalScriptVar(%p) (%d, %d)", (const void *)script, stackPos(0), stackPos(1));
	assert(stackPos(0) < 16);
	_globalScriptVars[stackPos(0)] = stackPos(1);
	return 1;
}

int LoLEngine::olol_getGlobalVar(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_getGlobalVar(%p) (%d, %d)", (const void *)script, stackPos(0), stackPos(1));

	switch (stackPos(0)) {
	case 0:
		return _currentBlock;
	case 1:
		return _currentDirection;
	case 2:
		return _currentLevel;
	case 3:
		return _itemInHand;
	case 4:
		return _brightness;
	case 5:
		return _credits;
	case 6:
		return _unkWordArraySize8[stackPos(1)];
	case 8:
		return _updateFlags;
	case 9:
		return _lampOilStatus;
	case 10:
		return _sceneDefaultUpdate;
	case 11:
		return _unkBt1;
	case 12:
		return _unkBt2;
	case 13:
		return _speechFlag;
	default:
		break;
	}

	return 0;
}

int LoLEngine::olol_setGlobalVar(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_setGlobalVar(%p) (%d, %d, %d)", (const void *)script, stackPos(0), stackPos(1), stackPos(2));
	uint16 a = stackPos(1);
	uint16 b = stackPos(2);

	switch (stackPos(0)) {
	case 0:
		_currentBlock = b;
		calcCoordinates(_partyPosX, _partyPosY, _currentBlock, 0x80, 0x80);
		updateAutoMap(_currentBlock);
		break;

	case 1:
		_currentDirection = b;
		break;

	case 2:
		_currentLevel = b & 0xff;
		break;

	case 3:
		setHandItem(b);
		break;

	case 4:
		_brightness = b & 0xff;
		break;

	case 5:
		_credits = b;
		break;

	case 6:
		_unkWordArraySize8[a] = b;
		break;

	case 7:
		break;

	case 8:
		_updateFlags = b;
		if (b == 1) {
			if (!textEnabled() || (!(_currentControlMode & 2)))
				timerUpdatePortraitAnimations(1);
			disableSysTimer(2);
		} else {
			enableSysTimer(2);
		}
		break;

	case 9:
		_lampOilStatus = b & 0xff;
		break;

	case 10:
		_sceneDefaultUpdate = b & 0xff;
		gui_toggleButtonDisplayMode(0, 0);
		break;

	case 11:
		_unkBt1 = a & 0xff;
		break;

	case 12:
		_unkBt2 = a & 0xff;
		break;

	default:
		break;
	}

	return 1;
}

int LoLEngine::olol_triggerDoorSwitch(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_triggerDoorSwitch(%p) (%d, %d)", (const void *)script, stackPos(0), stackPos(1));
	processDoorSwitch(stackPos(0)/*, (_wllWallFlags[_levelBlockProperties[stackPos(0)].walls[0]] & 8) ? 0 : 1*/, stackPos(1));
	return 1;
}

int LoLEngine::olol_updateBlockAnimations(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_updateBlockAnimations(%p) (%d, %d, %d, %d)", (const void *)script, stackPos(0), stackPos(1), stackPos(2), stackPos(3));
	int block = stackPos(0);
	int wall = stackPos(1);
	setWallType(block, wall, _levelBlockProperties[block].walls[(wall == -1) ? 0 : wall] == stackPos(2) ? stackPos(3) : stackPos(2));
	return 0;
}

int LoLEngine::olol_mapShapeToBlock(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_mapShapeToBlock(%p) (%d)", (const void *)script, stackPos(0));
	return assignLevelShapes(stackPos(0));
}

int LoLEngine::olol_resetBlockShapeAssignment(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_resetBlockShapeAssignment(%p) (%d)", (const void *)script, stackPos(0));
	uint8 v = stackPos(0) & 0xff;
	memset(_wllShapeMap + 3, v, 5);
	memset(_wllShapeMap + 13, v, 5);
	return 1;
}

int LoLEngine::olol_copyRegion(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_copyRegion(%p) (%d, %d, %d, %d, %d, %d, %d, %d)",
		(const void *)script, stackPos(0), stackPos(1), stackPos(2), stackPos(3), stackPos(4), stackPos(5), stackPos(6), stackPos(7));
	_screen->copyRegion(stackPos(0), stackPos(1), stackPos(2), stackPos(3), stackPos(4), stackPos(5), stackPos(6), stackPos(7), Screen::CR_NO_P_CHECK);
	return 1;
}

int LoLEngine::olol_initMonster(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_initMonster(%p) (%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d)", (const void *)script,
		stackPos(0), stackPos(1), stackPos(2), stackPos(3), stackPos(4), stackPos(5), stackPos(6), stackPos(7), stackPos(8), stackPos(9), stackPos(10));
	uint16 x = 0;
	uint16 y = 0;
	calcCoordinates(x, y, stackPos(0), stackPos(1), stackPos(2));
	uint16 w = _monsterProperties[stackPos(4)].maxWidth;

	if (checkBlockBeforeObjectPlacement(x, y, w, 7, 7))
		return -1;

	for (uint8 i = 0; i < 30; i++) {
		MonsterInPlay *l = &_monsters[i];
		if (l->might || l->mode == 13)
			continue;

		memset(l, 0, sizeof(MonsterInPlay));		
		l->id = i;
		l->x = x;
		l->y = y;
		l->facing = stackPos(3);
		l->type = stackPos(4);
		l->properties = &_monsterProperties[l->type];
		l->direction = l->facing << 1;
		l->might = (l->properties->might * _monsterModifiers[_monsterDifficulty]) >> 8;

		if (_currentLevel == 12 && l->type == 2)
			l->might = (l->might * (_rnd.getRandomNumberRng(1, 128) + 192)) >> 8;

		l->field_25 = l->properties->unk6[0];
		l->field_27 = _rnd.getRandomNumberRng(1, calcMonsterSkillLevel(l->id | 0x8000, 8)) - 1;
		l->flyingHeight = 2;
		l->flags = stackPos(5);
		l->assignedItems = 0;

		setMonsterMode(l, stackPos(6));
		placeMonster(l, l->x, l->y);

		l->destX = l->x;
		l->destY = l->y;
		l->destDirection = l->direction;

		for (int ii = 0; ii < 4; ii++)
			l->field_2A[ii] = stackPos(7 + ii);

		checkSceneUpdateNeed(l->blockPropertyIndex);
		return i;
	}

	return -1;
}

int LoLEngine::olol_fadeClearSceneWindow(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_fadeClearSceneWindow(%p)", (const void *)script);
	_screen->fadeClearSceneWindow(10);
	return 1;
}

int LoLEngine::olol_fadeSequencePalette(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_fadeSequencePalette(%p)", (const void *)script);
	memcpy(_screen->getPalette(3) + 0x180, _screen->_currentPalette + 0x180, 0x180);
	_screen->loadSpecialColours(_screen->getPalette(3));
	_screen->fadePalette(_screen->getPalette(3), 10);
	_screen->_fadeFlag = 0;
	return 1;
}

int LoLEngine::olol_redrawPlayfield(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_redrawPlayfield(%p)", (const void *)script);
	if (_screen->_fadeFlag != 2)
		_screen->fadeClearSceneWindow(10);
	gui_drawPlayField();
	setPaletteBrightness(_screen->_currentPalette, _brightness, _lampEffect);
	_screen->_fadeFlag = 0;
	return 1;
}

int LoLEngine::olol_loadNewLevel(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_loadNewLevel(%p) (%d, %d, %d)", (const void *)script, stackPos(0), stackPos(1), stackPos(2));
	_screen->fadeClearSceneWindow(10);
	_screen->fillRect(112, 0, 288, 120, 0);
	disableSysTimer(2);
	
	for (int i = 0; i < 8; i++) {
		if (!_flyingObjects[i].enable || _flyingObjects[i].a)
			continue;
		endObjectFlight(&_flyingObjects[i], _flyingObjects[i].x, _flyingObjects[i].y, 1);
	}

	completeDoorOperations();

	generateTempData();

	_currentBlock = stackPos(1);
	_currentDirection = stackPos(2);
	calcCoordinates(_partyPosX, _partyPosY, _currentBlock, 0x80, 0x80);

	loadLevel(stackPos(0));

	enableSysTimer(2);

	script->ip = 0;
	return 1;
}

int LoLEngine::olol_dummy0(EMCState *script) {
	return 0;
}

int LoLEngine::olol_loadMonsterProperties(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_loadMonsterProperties(%p) (%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d)",
		(const void *)script, stackPos(0), stackPos(1), stackPos(2), stackPos(3), stackPos(4), stackPos(5),
		stackPos(6), stackPos(7), stackPos(8), stackPos(9), stackPos(10), stackPos(11), stackPos(12), stackPos(13),
		stackPos(14), stackPos(15), stackPos(16), stackPos(17), stackPos(18), stackPos(19), stackPos(20),
		stackPos(21), stackPos(22), stackPos(23), stackPos(24), stackPos(25), stackPos(26),	stackPos(27),
		stackPos(28), stackPos(29), stackPos(30), stackPos(31), stackPos(32), stackPos(33), stackPos(34),
		stackPos(35), stackPos(36), stackPos(37), stackPos(38), stackPos(39), stackPos(40), stackPos(41));

	MonsterProperty *l = &_monsterProperties[stackPos(0)];
	l->shapeIndex = stackPos(1) & 0xff;

	int shpWidthMax = 0;

	for (int i = 0; i < 16; i++) {
		uint8 m = _monsterShapes[(l->shapeIndex << 4) + i][3];
		if (m > shpWidthMax)
			shpWidthMax = m;
	}

	l->maxWidth = shpWidthMax;

	l->fightingStats[0] = (stackPos(2) << 8) / 100;		// hit chance
	l->fightingStats[1] = 256;							//
	l->fightingStats[2] = (stackPos(3) << 8) / 100;		// protection
	l->fightingStats[3] = stackPos(4);					// evade chance
	l->fightingStats[4] = (stackPos(5) << 8) / 100;		// speed
	l->fightingStats[5] = (stackPos(6) << 8) / 100;		//
	l->fightingStats[6] = (stackPos(7) << 8) / 100;		//
	l->fightingStats[7] = (stackPos(8) << 8) / 100;		//
	l->fightingStats[8] = 0;
	l->fightingStats[9] = 0;

	for (int i = 0; i < 8; i++) {
		l->unk2[i] = stackPos(9 + i);
		l->unk3[i] = (stackPos(17 + i) << 8) / 100;
	}

	l->itemProtection = stackPos(25);
	l->might = stackPos(26);
	l->speedTotalWaitTicks = 1;
	l->flags = stackPos(27);
	l->unk5 = stackPos(28);
	// FIXME???
	l->unk5 = stackPos(29);
	//

	for (int i = 0; i < 5; i++)
		l->unk6[i] = stackPos(30 + i);

	for (int i = 0; i < 2; i++) {
		l->unk7[i] = stackPos(35 + i);
		l->unk7[i + 2] = stackPos(37 + i);
	}

	for (int i = 0; i < 3; i++)
		l->sounds[i] = stackPos(39 + i);

	return 1;
}

int LoLEngine::olol_battleHitSkillTest(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_battleHitSkillTest(%p) (%d, %d, %d)", (const void *)script, stackPos(0), stackPos(1), stackPos(2));
	return battleHitSkillTest(stackPos(0), stackPos(1), stackPos(2));
}

int LoLEngine::olol_moveMonster(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_moveMonster(%p) (%d, %d, %d, %d)", (const void *)script, stackPos(0), stackPos(1), stackPos(2), stackPos(3));
	MonsterInPlay *m = &_monsters[stackPos(0)];

	if (m->mode == 1 || m->mode == 2) {
		calcCoordinates(m->destX, m->destY, stackPos(1), stackPos(2), stackPos(3));
		m->destDirection = stackPos(4) << 1;
		if (m->x != m->destX || m->y != m->destY)
			setMonsterDirection(m, calcMonsterDirection(m->x, m->y, m->destX, m->destY));
	}

	return 1;
}

int LoLEngine::olol_dialogueBox(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_dialogueBox(%p) (%d, %d, %d, %d)", (const void *)script, stackPos(0), stackPos(1), stackPos(2), stackPos(3));

	_tim->drawDialogueBox(stackPos(0), getLangString(stackPos(1)), getLangString(stackPos(2)), getLangString(stackPos(3)));
	return 1;
}

int LoLEngine::olol_giveTakeMoney(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_giveTakeMoney(%p) (%d)", (const void *)script, stackPos(0));
	int c = stackPos(0);
	if (c >= 0)
		giveCredits(c, 1);
	else
		takeCredits(-c, 1);

	return 1;
}

int LoLEngine::olol_checkMoney(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_checkMoney(%p) (%d)", (const void *)script, stackPos(0));
	return (stackPos(0) > _credits) ? 0 : 1;
}

int LoLEngine::olol_setScriptTimer(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_setScriptTimer(%p) (%d, %d)", (const void *)script, stackPos(0), stackPos(1));
	uint8 id = 0x50 + stackPos(0);

	if (stackPos(1)) {
		_timer->enable(id);
		_timer->setCountdown(id, stackPos(1));

	} else {
		_timer->disable(id);
	}

	return 1;
}

int LoLEngine::olol_createHandItem(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_createHandItem(%p) (%d, %d, %d)", (const void *)script, stackPos(0), stackPos(1), stackPos(2));
	if (_itemInHand)
		return 0;
	setHandItem(makeItem(stackPos(0), stackPos(1), stackPos(2)));
	return 1;
}

int LoLEngine::olol_characterJoinsParty(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_characterJoinsParty(%p) (%d, %d, %d)", (const void *)script, stackPos(0), stackPos(1), stackPos(2));

	int16 id = stackPos(0);
	if (id < 0)
		id = -id;

	for (int i = 0; i < 4; i++) {
		if (!(_characters[i].flags & 1) || _characters[i].id != id)
			continue;

		_characters[i].flags &= 0xfffe;
		calcCharPortraitXpos();
		
		if (!_updateFlags) {
			gui_enableDefaultPlayfieldButtons();
			gui_drawPlayField();
		}

		if (_selectedCharacter == i)
			_selectedCharacter = 0;

		return 1;
	}

	addCharacter(id);

	if (!_updateFlags) {
		gui_enableDefaultPlayfieldButtons();
		gui_drawPlayField();
	}

	return 1;
}

int LoLEngine::olol_loadTimScript(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_loadTimScript(%p) (%d, %s)", (const void *)script, stackPos(0), stackPosString(1));
	if (_activeTim[stackPos(0)])
		return 1;
	char file[13];
	snprintf(file, sizeof(file), "%s.TIM", stackPosString(1));
	_activeTim[stackPos(0)] = _tim->load(file, &_timIngameOpcodes);
	return 1;
}

int LoLEngine::olol_runTimScript(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_runTimScript(%p) (%d, %d)", (const void *)script, stackPos(0), stackPos(1));
	return _tim->exec(_activeTim[stackPos(0)], stackPos(1));
}

int LoLEngine::olol_releaseTimScript(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_releaseTimScript(%p) (%d)", (const void *)script, stackPos(0));
	_tim->unload(_activeTim[stackPos(0)]);
	return 1;
}

int LoLEngine::olol_initSceneWindowDialogue(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_initSceneWindowDialogue(%p) (%d)", (const void *)script, stackPos(0));
	initSceneWindowDialogue(stackPos(0));
	return 1;
}

int LoLEngine::olol_restoreAfterSceneWindowDialogue(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_restoreAfterSceneWindowDialogue(%p) (%d)", (const void *)script, stackPos(0));
	restoreAfterSceneWindowDialogue(stackPos(0));
	return 1;
}

int LoLEngine::olol_getItemInHand(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_getItemInHand(%p))", (const void *)script);
	return _itemInHand;
}

int LoLEngine::olol_giveItemToMonster(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_giveItemToMonster(%p) (%d, %d)", (const void *)script, stackPos(0), stackPos(1));
	if (stackPos(0) == -1)
		return 0;
	giveItemToMonster(&_monsters[stackPos(0)], stackPos(1));
	return 1;
}

int LoLEngine::olol_loadLangFile(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_loadLangFile(%p) (%s)", (const void *)script, stackPosString(0));
	char filename[13];
	snprintf(filename, sizeof(filename), "%s.%s", stackPosString(0), _languageExt[_lang]);
	if (_levelLangFile)
		delete[] _levelLangFile;
	_levelLangFile = _res->fileData(filename, 0);
	return 1;
}

int LoLEngine::olol_playSoundEffect(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_playSoundEffect(%p) (%d)", (const void *)script, stackPos(0));
	snd_playSoundEffect(stackPos(0), -1);
	return 1;
}

int LoLEngine::olol_processDialogue(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_processDialogue(%p)", (const void *)script);
	return _tim->processDialogue();
}

int LoLEngine::olol_stopTimScript(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_stopTimScript(%p) (%d)", (const void *)script, stackPos(0));
	_tim->stopAllFuncs(_activeTim[stackPos(0)]);
	return 1;
}

int LoLEngine::olol_getWallFlags(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_getWallFlags(%p) (%d, %d)", (const void *)script, stackPos(0), stackPos(1));
	return _wllWallFlags[_levelBlockProperties[stackPos(0)].walls[stackPos(1) & 3]];
}

int LoLEngine::olol_changeMonsterSettings(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_changeMonsterSettings(%p) (%d, %d, %d)", (const void *)script, stackPos(0), stackPos(1), stackPos(2));
	if (stackPos(0) == -1)
		return 1;

	MonsterInPlay *m = &_monsters[stackPos(0) & 0x7fff];

	int16 d = stackPos(2);
	uint16 x = 0;
	uint16 y = 0;

	switch (stackPos(1)) {
		case 0:
			setMonsterMode(m, d);
			break;

		case 1:
			m->might = d;
			break;

		case 2:
			calcCoordinates(x, y, d, m->x & 0xff, m->y & 0xff);
			if (!walkMonsterCheckDest(x, y, m, 7))
				placeMonster(m, x, y);
			break;

		case 3:
			setMonsterDirection(m, d << 1);
			break;

		case 6:
			m->flags |= d;
			break;

		default:
			break;
	}

	return 1;
}


int LoLEngine::olol_playCharacterScriptChat(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_playCharacterScriptChat(%p) (%d, %d, %d)", (const void *)script, stackPos(0), stackPos(1), stackPos(2));
	snd_stopSpeech(1);
	updatePortraits();
	return playCharacterScriptChat(stackPos(0), stackPos(1), 1, getLangString(stackPos(2)), script, 0, 3);
}

int LoLEngine::olol_update(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_update(%p)", (const void *)script);
	update();
	return 1;
}

int LoLEngine::olol_drawExitButton(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_drawExitButton(%p) (%d, %d)", (const void *)script, stackPos(0), stackPos(1));
	
	static const uint8 printPara[] = { 0x90, 0x78, 0x0C, 0x9F, 0x80, 0x1E };

	int cp = _screen->setCurPage(0);
	Screen::FontId cf = _screen->setFont(Screen::FID_6_FNT);
	int x = printPara[3 * stackPos(0)] << 1;
	int y = printPara[3 * stackPos(0) + 1];
	int offs = printPara[3 * stackPos(0) + 2];

	char *str = getLangString(0x4033);
	int w = _screen->getTextWidth(str);
	
	gui_drawBox(x - offs - w, y - 9, w + offs, 9, 136, 251, 252);
	_screen->printText(str, x - (offs >> 1) - w, y - 7, 144, 0);

	if (stackPos(1))
		_screen->drawGridBox(x - offs - w + 1, y - 8, w + offs - 2, 7, 1);

	_screen->setFont(cf);
	_screen->setCurPage(cp);	
	return 1;
}

int LoLEngine::olol_loadSoundFile(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_loadSoundFile(%p) (%d)", (const void *)script, stackPos(0));
	snd_loadSoundFile(stackPos(0));
	return 1;
}

int LoLEngine::olol_playMusicTrack(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_playMusicTrack(%p) (%d)", (const void *)script, stackPos(0));
	return snd_playTrack(stackPos(0));
}

int LoLEngine::olol_countBlockItems(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_countBlockItems(%p) (%d)", (const void *)script, stackPos(0));
	uint16 o = _levelBlockProperties[stackPos(0)].assignedObjects;
	int res = 0;

	while (o) {
		if (!(o & 0x8000))
			res++;
		o = findObject(o)->nextAssignedObject;
	}

	return res;
}

int LoLEngine::olol_stopCharacterSpeech(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_stopCharacterSpeech(%p)", (const void *)script);
	snd_stopSpeech(1);
	updatePortraits();
	return 1;
}

int LoLEngine::olol_setPaletteBrightness(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_setPaletteBrightness(%p) (%d, %d)", (const void *)script, stackPos(0), stackPos(1));
	uint16 old = _brightness;
	_brightness = stackPos(0);
	if (stackPos(1) == 1)
		setPaletteBrightness(_screen->_currentPalette, stackPos(0), _lampEffect);
	return old;
}

int LoLEngine::olol_printMessage(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_printMessage(%p) (%d, %d, %d, %d, %d, %d, %d, %d, %d, %d)", (const void *)script, stackPos(0), stackPos(1), stackPos(2), stackPos(3), stackPos(4), stackPos(5), stackPos(6), stackPos(7), stackPos(8), stackPos(9));
	int snd = stackPos(2);
	_txt->printMessage(stackPos(0), getLangString(stackPos(1)), stackPos(3), stackPos(4), stackPos(5), stackPos(6), stackPos(7), stackPos(8), stackPos(9));

	if (snd >= 0)
		snd_playSoundEffect(snd, -1);

	return 1;
}

int LoLEngine::olol_deleteLevelItem(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_deleteLevelItem(%p) (%d)", (const void *)script, stackPos(0));
	if (_itemsInPlay[stackPos(0)].blockPropertyIndex)
		removeLevelItem(stackPos(0), _itemsInPlay[stackPos(0)].blockPropertyIndex);

	deleteItem(stackPos(0));

	return 1;
}

int LoLEngine::olol_playDialogueTalkText(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_playDialogueTalkText(%p) (%d)", (const void *)script, stackPos(0));
	int track = stackPos(0);

	if (!snd_playCharacterSpeech(track, 0, 0) || textEnabled()) {
		char *s = getLangString(track);
		_txt->printDialogueText(4, s, script, 0, 1);
	}

	return 1;
}

int LoLEngine::olol_checkMonsterTypeHostility(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_checkMonsterTypeHostility(%p) (%d)", (const void *)script, stackPos(0));
	for (int i = 0; i < 30; i++) {
		if (stackPos(0) != _monsters[i].type && stackPos(0) != -1)
			continue;
		return (_monsters[i].mode == 1) ? 0 : 1;
	}
	return 1;
}

int LoLEngine::olol_setNextFunc(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_setNextFunc(%p) (%d)", (const void *)script, stackPos(0));
	_nextScriptFunc = stackPos(0);
	return 1;
}

int LoLEngine::olol_dummy1(EMCState *script) {
	return 1;
}

int LoLEngine::olol_suspendMonster(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_suspendMonster(%p) (%d)", (const void *)script, stackPos(0));
	MonsterInPlay *m = &_monsters[stackPos(0) & 0x7fff];
	setMonsterMode(m, 14);
	checkSceneUpdateNeed(m->blockPropertyIndex);
	placeMonster(m, 0, 0);
	return 1;
}

int LoLEngine::olol_setDoorState(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_setDoorState(%p) (%d)", (const void *)script, stackPos(0));
	_emcDoorState = stackPos(0);
	return _emcDoorState;
}

int LoLEngine::olol_processButtonClick(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_processButtonClick(%p) (%d)", (const void *)script, stackPos(0));
	_tim->forceDialogue(_activeTim[stackPos(0)]);
	return 1;
}

int LoLEngine::olol_savePage5(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_savePage5(%p)", (const void *)script);
	savePage5();
	return 1;
}

int LoLEngine::olol_restorePage5(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_restorePage5(%p)", (const void *)script);
	restorePage5();
	return 1;
}

int LoLEngine::olol_initDialogueSequence(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_initDialogueSequence(%p) (%d, %d)", (const void *)script, stackPos(0), stackPos(1));
	initDialogueSequence(stackPos(0), stackPos(1));
	return 1;
}

int LoLEngine::olol_restoreAfterDialogueSequence(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_restoreAfterDialogueSequence(%p) (%d)", (const void *)script, stackPos(0));
	restoreAfterDialogueSequence(stackPos(0));
	return 1;
}

int LoLEngine::olol_setSpecialSceneButtons(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_setSpecialSceneButtons(%p) (%d, %d, %d, %d, %d)", (const void *)script, stackPos(0), stackPos(1), stackPos(2), stackPos(3), stackPos(4));
	setSpecialSceneButtons(stackPos(0), stackPos(1), stackPos(2), stackPos(3), stackPos(4));
	return 1;
}

int LoLEngine::olol_prepareSpecialScene(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_prepareSpecialScene(%p) (%d, %d, %d, %d, %d, %d)", (const void *)script, stackPos(0), stackPos(1), stackPos(2), stackPos(3), stackPos(4), stackPos(5));
	prepareSpecialScene(stackPos(0), stackPos(1), stackPos(2), stackPos(3), stackPos(4), stackPos(5));
	return 1;
}

int LoLEngine::olol_restoreAfterSpecialScene(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_restoreAfterSpecialScene(%p) (%d, %d, %d, %d)", (const void *)script, stackPos(0), stackPos(1), stackPos(2), stackPos(3));
	return restoreAfterSpecialScene(stackPos(0), stackPos(1), stackPos(2), stackPos(3));
}

int LoLEngine::olol_assignCustomSfx(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_assignCustomSfx(%p) (%s, %d)", (const void *)script, stackPosString(0), stackPos(1));
	const char *c = stackPosString(0);
	int i = stackPos(1);

	if (!c || i > 250)
		return 0;

	uint16 t = READ_LE_UINT16(&_ingameSoundIndex[i << 1]);
	if (t == 0xffff)
		return 0;

	strcpy(_ingameSoundList[t], c);

	return 0;
}

int LoLEngine::olol_resetPortraitsAndDisableSysTimer(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_resetPortraitsAndDisableSysTimer(%p)", (const void *)script);
	resetPortraitsAndDisableSysTimer();
	return 1;
}

int LoLEngine::olol_enableSysTimer(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_enableSysTimer(%p)", (const void *)script);
	_needSceneRestore = 0;
	enableSysTimer(2);
	return 1;
}

int LoLEngine::olol_disableControls(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_disableControls(%p) (%d)", (const void *)script, stackPos(0));
	return gui_disableControls(stackPos(0));
}

int LoLEngine::olol_enableControls(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_enableControls(%p)", (const void *)script);
	return gui_enableControls();
}

int LoLEngine::olol_characterSays(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_characterSays(%p)  (%d, %d, %d)", (const void *)script, stackPos(0), stackPos(1), stackPos(2));
	if (stackPos(0) == -1) {
		snd_stopSpeech(true);
		return 1;
	}

	if (stackPos(0) != -1)
		return characterSays(stackPos(0), stackPos(1), stackPos(2));
	else
		return snd_characterSpeaking();
}

int LoLEngine::olol_queueSpeech(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_queueSpeech(%p) (%d, %d)", (const void *)script, stackPos(0), stackPos(1));
	if (stackPos(0) && stackPos(1)) {
		_nextSpeechId = stackPos(0) + 1000;
		_nextSpeaker = stackPos(1);
	}
	return 1;
}

int LoLEngine::olol_getItemPrice(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_getItemPrice(%p) (%d)", (const void *)script, stackPos(0));
	int c = stackPos(0);
	if (c < 0) {
		c = -c;
		if (c < 50)
			return 50;
		c = (c + 99) / 100;
		return c * 100;

	} else {
		for (int i = 0; i < 46; i++) {
			if (_itemCost[i] >= c)
				return _itemCost[i];
		}
	}

	return 0;
}

int LoLEngine::olol_getLanguage(EMCState *script) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::olol_getLanguage(%p)", (const void *)script);
	return _lang;
}

#pragma mark -

int LoLEngine::tlol_setupPaletteFade(const TIM *tim, const uint16 *param) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::t2_playSoundEffect(%p, %p) (%d)", (const void*)tim, (const void*)param, param[0]);
	_screen->getFadeParams(_screen->getPalette(0), param[0], _tim->_palDelayInc, _tim->_palDiff);
	_tim->_palDelayAcc = 0;
	return 1;
}

int LoLEngine::tlol_loadPalette(const TIM *tim, const uint16 *param) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::tlol_loadPalette(%p, %p) (%d)", (const void*)tim, (const void*)param, param[0]);
	const char *palFile = (const char *)(tim->text + READ_LE_UINT16(tim->text + (param[0]<<1)));
	_res->loadFileToBuf(palFile, _screen->getPalette(0), 768);
	return 1;
}

int LoLEngine::tlol_setupPaletteFadeEx(const TIM *tim, const uint16 *param) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::tlol_setupPaletteFadeEx(%p, %p) (%d)", (const void*)tim, (const void*)param, param[0]);
	memcpy(_screen->getPalette(0), _screen->getPalette(1), 768);

	_screen->getFadeParams(_screen->getPalette(0), param[0], _tim->_palDelayInc, _tim->_palDiff);
	_tim->_palDelayAcc = 0;
	return 1;
}

int LoLEngine::tlol_processWsaFrame(const TIM *tim, const uint16 *param) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::tlol_processWsaFrame(%p, %p) (%d, %d, %d, %d, %d)",
		(const void*)tim, (const void*)param, param[0], param[1], param[2], param[3], param[4]);
	TIMInterpreter::Animation *anim = (TIMInterpreter::Animation *)tim->wsa[param[0]].anim;
	const int frame = param[1];
	const int x2 = param[2];
	const int y2 = param[3];
	const int factor = MAX<int>(0, (int16)param[4]);

	const int x1 = anim->x;
	const int y1 = anim->y;

	int w1 = anim->wsa->width();
	int h1 = anim->wsa->height();
	int w2 = (w1 * factor) / 100;
	int h2 = (h1 * factor) / 100;

	anim->wsa->setDrawPage(2);
	anim->wsa->setX(x1);
	anim->wsa->setY(y1);
	anim->wsa->displayFrame(frame, anim->wsaCopyParams & 0xF0FF, 0, 0);
	_screen->wsaFrameAnimationStep(x1, y1, x2, y2, w1, h1, w2, h2, 2, 8, 0);
	_screen->checkedPageUpdate(8, 4);
	_screen->updateScreen();

	return 1;
}

int LoLEngine::tlol_displayText(const TIM *tim, const uint16 *param) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::tlol_displayText(%p, %p) (%d, %d)", (const void*)tim, (const void*)param, param[0], (int16)param[1]);
	_tim->displayText(param[0], param[1]);
	return 1;
}

int LoLEngine::tlol_initSceneWindowDialogue(const TIM *tim, const uint16 *param) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::tlol_initSceneWindowDialogue(%p, %p) (%d)", (const void*)tim, (const void*)param, param[0]);
	initSceneWindowDialogue(param[0]);
	return 1;
}

int LoLEngine::tlol_restoreAfterSceneWindowDialogue(const TIM *tim, const uint16 *param) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::tlol_restoreAfterSceneWindowDialogue(%p, %p) (%d)", (const void*)tim, (const void*)param, param[0]);
	restoreAfterSceneWindowDialogue(param[0]);
	return 1;
}

int LoLEngine::tlol_giveItem(const TIM *tim, const uint16 *param) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::tlol_giveItem(%p, %p) (%d)", (const void*)tim, (const void*)param, param[0]);
	int item = makeItem(param[0], param[1], param[2]);
	if (addItemToInventory(item))
		return 1;

	deleteItem(item);
	return 0;
}

int LoLEngine::tlol_setPartyPosition(const TIM *tim, const uint16 *param) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::tlol_setPartyPosition(%p, %p) (%d, %d)", (const void*)tim, (const void*)param, param[0], param[1]);
	if (param[0] == 1) {
		_currentDirection = param[1];
	} else if (param[0] == 0) {
		_currentBlock = param[1];
		calcCoordinates(_partyPosX, _partyPosY, _currentBlock, 0x80, 0x80);
	}

	return 1;
}

int LoLEngine::tlol_fadeClearWindow(const TIM *tim, const uint16 *param) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::tlol_fadeClearWindow(%p, %p) (%d)", (const void*)tim, (const void*)param, param[0]);
	uint8 *tmp = 0;

	switch (param[0]) {
		case 0:
			_screen->fadeClearSceneWindow(10);
			break;

		case 1:
			tmp = _screen->getPalette(3);
			memcpy(tmp + 0x180, _screen->_currentPalette + 0x180, 0x180);
			_screen->loadSpecialColours(tmp);
			_screen->fadePalette(tmp, 10);
			_screen->_fadeFlag = 0;
			break;

		case 2:
			_screen->fadeToBlack(10);
			break;

		case 3:
			tmp = _screen->getPalette(3);
			_screen->loadSpecialColours(tmp);
			_screen->fadePalette(tmp, 10);
			_screen->_fadeFlag = 0;
			break;

		case 4:
			if (_screen->_fadeFlag != 2)
				_screen->fadeClearSceneWindow(10);
			gui_drawPlayField();
			setPaletteBrightness(_screen->_currentPalette, _brightness, _lampEffect);
			_screen->_fadeFlag = 0;
			break;

		case 5:
			tmp = _screen->getPalette(3);
			_screen->loadSpecialColours(tmp);
			_screen->fadePalette(_screen->getPalette(1), 10);
			_screen->_fadeFlag = 0;
			break;

		default:
			break;
	}

	return 1;
}

int LoLEngine::tlol_copyRegion(const TIM *tim, const uint16 *param) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::tlol_copyRegion(%p, %p) (%d, %d, %d, %d, %d, %d, %d, %d)", (const void*)tim, (const void*)param, param[0], param[1], param[2], param[3], param[4], param[5], param[6], param[7]);
	_screen->copyRegion(param[0], param[1], param[2], param[3], param[4], param[5], param[6], param[7], Screen::CR_NO_P_CHECK);
	return 1;
}

int LoLEngine::tlol_characterChat(const TIM *tim, const uint16 *param) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::tlol_characterChat(%p, %p) (%d, %d, %d)", (const void*)tim, (const void*)param, param[0], param[1], param[2]);
	playCharacterScriptChat(param[0], param[1], 1, getLangString(param[2]), 0, param, 3);
	return 1;
}

int LoLEngine::tlol_drawScene(const TIM *tim, const uint16 *param) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::tlol_drawScene(%p, %p) (%d)", (const void*)tim, (const void*)param, param[0]);
	gui_drawScene(param[0]);
	if (_sceneDrawPage2 != 2 && param[0] == 2)
		_screen->copyRegion(112, 0, 112, 0, 176, 120, _sceneDrawPage2, 2, Screen::CR_NO_P_CHECK);
	return 1;
}

int LoLEngine::tlol_update(const TIM *tim, const uint16 *param) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::tlol_update(%p, %p)", (const void*)tim, (const void*)param);
	update();
	return 1;
}

int LoLEngine::tlol_loadSoundFile(const TIM *tim, const uint16 *param) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::tlol_loadSoundFile(%p, %p) (%d)", (const void*)tim, (const void*)param, param[0]);
	snd_loadSoundFile(param[0]);
	return 1;
}

int LoLEngine::tlol_playMusicTrack(const TIM *tim, const uint16 *param) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::tlol_playMusicTrack(%p, %p) (%d)", (const void*)tim, (const void*)param, param[0]);
	snd_playTrack(param[0]);
	return 1;
}

int LoLEngine::tlol_playDialogueTalkText(const TIM *tim, const uint16 *param) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::tlol_playDialogueTalkText(%p, %p) (%d)", (const void*)tim, (const void*)param, param[0]);
	if (!snd_playCharacterSpeech(param[0], 0, 0) || textEnabled())
		_txt->printDialogueText(4, getLangString(param[0]), 0, param, 1);
	return 1;
}

int LoLEngine::tlol_playSoundEffect(const TIM *tim, const uint16 *param) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::tlol_playSoundEffect(%p, %p) (%d)", (const void*)tim, (const void*)param, param[0]);
	snd_playSoundEffect(param[0], -1);
	return 1;
}

int LoLEngine::tlol_startBackgroundAnimation(const TIM *tim, const uint16 *param) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::tlol_startBackgroundAnimation(%p, %p) (%d, %d)", (const void*)tim, (const void*)param, param[0], param[1]);
	_tim->startBackgroundAnimation(param[0], param[1]);
	return 1;
}

int LoLEngine::tlol_stopBackgroundAnimation(const TIM *tim, const uint16 *param) {
	debugC(3, kDebugLevelScriptFuncs, "LoLEngine::tlol_stopBackgroundAnimation(%p, %p) (%d)", (const void*)tim, (const void*)param, param[0]);
	_tim->stopBackgroundAnimation(param[0]);
	return 1;
}

#pragma mark -

typedef Common::Functor1Mem<EMCState*, int, LoLEngine> OpcodeV2;
#define SetOpcodeTable(x) table = &x;
#define Opcode(x) table->push_back(new OpcodeV2(this, &LoLEngine::x))
#define OpcodeUnImpl() table->push_back(new OpcodeV2(this, 0))

typedef Common::Functor2Mem<const TIM *, const uint16 *, int, LoLEngine> TIMOpcodeLoL;
#define SetTimOpcodeTable(x) timTable = &x;
#define OpcodeTim(x) timTable->push_back(new TIMOpcodeLoL(this, &LoLEngine::x))
#define OpcodeTimUnImpl() timTable->push_back(new TIMOpcodeLoL(this, 0))

void LoLEngine::setupOpcodeTable() {
	Common::Array<const Opcode*> *table = 0;

	SetOpcodeTable(_opcodes);
	// 0x00
	Opcode(olol_setWallType);
	Opcode(olol_getWallType);
	Opcode(olol_drawScene);
	Opcode(o1_getRand);

	// 0x04
	OpcodeUnImpl();
	OpcodeUnImpl();
	Opcode(olol_delay);
	Opcode(olol_setGameFlag);

	// 0x08
	Opcode(olol_testGameFlag);
	Opcode(olol_loadLevelGraphics);
	Opcode(olol_loadCmzFile);
	Opcode(olol_loadMonsterShapes);

	// 0x0C
	Opcode(olol_deleteHandItem);
	Opcode(olol_allocItemPropertiesBuffer);
	Opcode(olol_setItemProperty);
	Opcode(olol_makeItem);

	// 0x10
	OpcodeUnImpl();
	Opcode(olol_createLevelItem);
	Opcode(olol_getItemPara);
	Opcode(olol_getCharacterStat);

	// 0x14
	Opcode(olol_setCharacterStat);
	Opcode(olol_loadLevelShapes);
	Opcode(olol_closeLevelShapeFile);
	OpcodeUnImpl();

	// 0x18
	Opcode(olol_loadDoorShapes);
	Opcode(olol_initAnimStruct);
	Opcode(olol_playAnimationPart);
	Opcode(olol_freeAnimStruct);

	// 0x1C
	Opcode(olol_getDirection);
	OpcodeUnImpl();
	Opcode(olol_setMusicTrack);
	Opcode(olol_setSequenceButtons);

	// 0x20
	Opcode(olol_setDefaultButtonState);
	Opcode(olol_checkRectForMousePointer);
	Opcode(olol_clearDialogueField);
	Opcode(olol_setupBackgroundAnimationPart);

	// 0x24
	Opcode(olol_startBackgroundAnimation);
	Opcode(o1_hideMouse);
	Opcode(o1_showMouse);
	Opcode(olol_fadeToBlack);

	// 0x28
	Opcode(olol_fadePalette);
	Opcode(olol_loadBitmap);
	Opcode(olol_stopBackgroundAnimation);
	OpcodeUnImpl();

	// 0x2C
	OpcodeUnImpl();
	Opcode(olol_getGlobalScriptVar);
	Opcode(olol_setGlobalScriptVar);
	Opcode(olol_getGlobalVar);

	// 0x30
	Opcode(olol_setGlobalVar);
	Opcode(olol_triggerDoorSwitch);
	OpcodeUnImpl();
	OpcodeUnImpl();

	// 0x34
	Opcode(olol_updateBlockAnimations);
	Opcode(olol_mapShapeToBlock);
	Opcode(olol_resetBlockShapeAssignment);
	Opcode(olol_copyRegion);

	// 0x38
	Opcode(olol_initMonster);
	Opcode(olol_fadeClearSceneWindow);
	Opcode(olol_fadeSequencePalette);
	Opcode(olol_redrawPlayfield);

	// 0x3C
	Opcode(olol_loadNewLevel);
	OpcodeUnImpl();
	Opcode(olol_dummy0);
	Opcode(olol_loadMonsterProperties);

	// 0x40
	Opcode(olol_battleHitSkillTest);
	OpcodeUnImpl();
	OpcodeUnImpl();
	OpcodeUnImpl();

	// 0x44
	Opcode(olol_moveMonster);
	Opcode(olol_dialogueBox);
	Opcode(olol_giveTakeMoney);
	Opcode(olol_checkMoney);

	// 0x48
	Opcode(olol_setScriptTimer);
	Opcode(olol_createHandItem);
	OpcodeUnImpl();
	Opcode(olol_characterJoinsParty);

	// 0x4C
	OpcodeUnImpl();
	OpcodeUnImpl();
	Opcode(olol_loadTimScript);
	Opcode(olol_runTimScript);

	// 0x50
	Opcode(olol_releaseTimScript);
	Opcode(olol_initSceneWindowDialogue);
	Opcode(olol_restoreAfterSceneWindowDialogue);
	Opcode(olol_getItemInHand);

	// 0x54
	OpcodeUnImpl();
	Opcode(olol_giveItemToMonster);
	Opcode(olol_loadLangFile);
	Opcode(olol_playSoundEffect);

	// 0x58
	Opcode(olol_processDialogue);
	Opcode(olol_stopTimScript);
	Opcode(olol_getWallFlags);
	Opcode(olol_changeMonsterSettings);

	// 0x5C
	OpcodeUnImpl();
	OpcodeUnImpl();
	Opcode(olol_playCharacterScriptChat);
	Opcode(olol_update);

	// 0x60
	OpcodeUnImpl();
	OpcodeUnImpl();
	Opcode(olol_drawExitButton);
	Opcode(olol_loadSoundFile);

	// 0x64
	Opcode(olol_playMusicTrack);
	OpcodeUnImpl();
	Opcode(olol_countBlockItems);
	OpcodeUnImpl();

	// 0x68
	OpcodeUnImpl();
	OpcodeUnImpl();
	Opcode(olol_stopCharacterSpeech);
	Opcode(olol_setPaletteBrightness);

	// 0x6C
	OpcodeUnImpl();
	OpcodeUnImpl();
	OpcodeUnImpl();
	Opcode(olol_printMessage);

	// 0x70
	Opcode(olol_deleteLevelItem);
	OpcodeUnImpl();
	OpcodeUnImpl();
	OpcodeUnImpl();

	// 0x74
	OpcodeUnImpl();
	OpcodeUnImpl();
	OpcodeUnImpl();
	OpcodeUnImpl();

	// 0x78
	OpcodeUnImpl();
	OpcodeUnImpl();
	Opcode(olol_playDialogueTalkText);
	Opcode(olol_checkMonsterTypeHostility);

	// 0x7C
	Opcode(olol_setNextFunc);
	Opcode(olol_dummy1);
	OpcodeUnImpl();
	Opcode(olol_suspendMonster);

	// 0x80
	OpcodeUnImpl();
	OpcodeUnImpl();
	OpcodeUnImpl();
	OpcodeUnImpl();

	// 0x84
	OpcodeUnImpl();
	OpcodeUnImpl();
	Opcode(olol_setDoorState);
	Opcode(olol_processButtonClick);

	// 0x88
	OpcodeUnImpl();
	OpcodeUnImpl();
	Opcode(olol_savePage5);
	Opcode(olol_restorePage5);

	// 0x8C
	Opcode(olol_initDialogueSequence);
	Opcode(olol_restoreAfterDialogueSequence);
	Opcode(olol_setSpecialSceneButtons);
	OpcodeUnImpl();

	// 0x90
	OpcodeUnImpl();
	OpcodeUnImpl();
	Opcode(olol_prepareSpecialScene);
	Opcode(olol_restoreAfterSpecialScene);

	// 0x94
	Opcode(olol_assignCustomSfx);
	OpcodeUnImpl();
	OpcodeUnImpl();
	OpcodeUnImpl();

	// 0x98
	OpcodeUnImpl();
	OpcodeUnImpl();
	Opcode(olol_resetPortraitsAndDisableSysTimer);
	Opcode(olol_enableSysTimer);

	// 0x9C
	OpcodeUnImpl();
	OpcodeUnImpl();
	OpcodeUnImpl();
	OpcodeUnImpl();

	// 0xA0
	OpcodeUnImpl();
	OpcodeUnImpl();
	OpcodeUnImpl();
	OpcodeUnImpl();

	// 0xA4
	OpcodeUnImpl();
	OpcodeUnImpl();
	OpcodeUnImpl();
	OpcodeUnImpl();

	// 0xA8
	OpcodeUnImpl();
	OpcodeUnImpl();
	OpcodeUnImpl();
	OpcodeUnImpl();

	// 0xAC
	OpcodeUnImpl();
	OpcodeUnImpl();
	OpcodeUnImpl();
	OpcodeUnImpl();

	// 0xB0
	OpcodeUnImpl();
	OpcodeUnImpl();
	Opcode(olol_dummy1); // anim buffer select?
	Opcode(olol_disableControls);

	// 0xB4
	Opcode(olol_enableControls);
	OpcodeUnImpl();
	OpcodeUnImpl();
	OpcodeUnImpl();

	// 0xB8
	OpcodeUnImpl();
	OpcodeUnImpl();
	OpcodeUnImpl();
	Opcode(olol_characterSays);

	// 0xBC
	Opcode(olol_queueSpeech);
	Opcode(olol_getItemPrice);
	Opcode(olol_getLanguage);
	OpcodeUnImpl();

	Common::Array<const TIMOpcode*> *timTable = 0;
	SetTimOpcodeTable(_timIntroOpcodes);

	// 0x00
	OpcodeTim(tlol_setupPaletteFade);
	OpcodeTimUnImpl();
	OpcodeTim(tlol_loadPalette);
	OpcodeTim(tlol_setupPaletteFadeEx);

	// 0x04
	OpcodeTim(tlol_processWsaFrame);
	OpcodeTim(tlol_displayText);
	OpcodeTimUnImpl();
	OpcodeTimUnImpl();

	SetTimOpcodeTable(_timIngameOpcodes);

	// 0x00
	OpcodeTim(tlol_initSceneWindowDialogue);
	OpcodeTim(tlol_restoreAfterSceneWindowDialogue);
	OpcodeTimUnImpl();
	OpcodeTim(tlol_giveItem);

	// 0x04
	OpcodeTim(tlol_setPartyPosition);
	OpcodeTim(tlol_fadeClearWindow);
	OpcodeTim(tlol_copyRegion);
	OpcodeTim(tlol_characterChat);

	// 0x08
	OpcodeTim(tlol_drawScene);
	OpcodeTim(tlol_update);
	OpcodeTimUnImpl();
	OpcodeTim(tlol_loadSoundFile);

	// 0x0C
	OpcodeTim(tlol_playMusicTrack);
	OpcodeTim(tlol_playDialogueTalkText);
	OpcodeTim(tlol_playSoundEffect);
	OpcodeTim(tlol_startBackgroundAnimation);

	// 0x10
	OpcodeTim(tlol_stopBackgroundAnimation);
}

} // end of namespace Kyra

#endif // ENABLE_LOL

