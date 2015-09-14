/* xoreos-tools - Tools to help with xoreos development
 *
 * xoreos-tools is the legal property of its developers, whose names
 * can be found in the AUTHORS file distributed with this source
 * distribution.
 *
 * xoreos-tools is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * xoreos-tools is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with xoreos-tools. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file
 *  Handling BioWare's TLK talk tables.
 */

/* See BioWare's own specs released for Neverwinter Nights modding
 * (<https://github.com/xoreos/xoreos-docs/tree/master/specs/bioware>)
 */

#include <cassert>

#include "src/common/util.h"
#include "src/common/strutil.h"
#include "src/common/memreadstream.h"
#include "src/common/readfile.h"
#include "src/common/error.h"

#include "src/aurora/talktable_tlk.h"
#include "src/aurora/language.h"

static const uint32 kTLKID    = MKTAG('T', 'L', 'K', ' ');
static const uint32 kVersion3 = MKTAG('V', '3', '.', '0');
static const uint32 kVersion4 = MKTAG('V', '4', '.', '0');

namespace Aurora {

TalkTable_TLK::Entry::Entry() : offset(0xFFFFFFFF), length(0xFFFFFFFF),
	flags(0), volumeVariance(0), pitchVariance(0), soundLength(-1.0f),
	soundID(0xFFFFFFFF) {

}


TalkTable_TLK::TalkTable_TLK(Common::SeekableReadStream *tlk, Common::Encoding encoding) :
	TalkTable(encoding), _tlk(tlk) {

	load();
}

TalkTable_TLK::~TalkTable_TLK() {
	delete _tlk;
}

void TalkTable_TLK::load() {
	assert(_tlk);

	try {
		readHeader(*_tlk);

		if (_id != kTLKID)
			throw Common::Exception("Not a TLK file (%s)", Common::debugTag(_id).c_str());

		if (_version != kVersion3 && _version != kVersion4)
			throw Common::Exception("Unsupported TLK file version %s", Common::debugTag(_version).c_str());

		_languageID = _tlk->readUint32LE();

		uint32 stringCount = _tlk->readUint32LE();
		_entries.resize(stringCount);

		// V4 added this field; it's right after the header in V3
		uint32 tableOffset = 20;
		if (_version == kVersion4)
			tableOffset = _tlk->readUint32LE();

		_stringsOffset = _tlk->readUint32LE();

		// Go to the table
		_tlk->seek(tableOffset);

		// Read in all the table data
		if (_version == kVersion3)
			readEntryTableV3();
		else
			readEntryTableV4();

	} catch (Common::Exception &e) {
		delete _tlk;

		e.add("Failed reading TLK file");
		throw;
	}
}

void TalkTable_TLK::readEntryTableV3() {
	for (size_t i = 0; i < _entries.size(); i++) {
		Entry &entry = _entries[i];

		entry.flags          = _tlk->readUint32LE();
		entry.soundResRef    = Common::readStringFixed(*_tlk, Common::kEncodingASCII, 16);
		entry.volumeVariance = _tlk->readUint32LE();
		entry.pitchVariance  = _tlk->readUint32LE();
		entry.offset         = _tlk->readUint32LE() + _stringsOffset;
		entry.length         = _tlk->readUint32LE();
		entry.soundLength    = _tlk->readIEEEFloatLE();

		if ((entry.length > 0) && (entry.flags & kFlagTextPresent))
			_strRefs.push_back(i);
	}
}

void TalkTable_TLK::readEntryTableV4() {
	for (size_t i = 0; i < _entries.size(); i++) {
		Entry &entry = _entries[i];

		entry.soundID = _tlk->readUint32LE();
		entry.offset  = _tlk->readUint32LE();
		entry.length  = _tlk->readUint16LE();
		entry.flags   = kFlagTextPresent;

		if ((entry.length > 0) && (entry.flags & kFlagTextPresent))
			_strRefs.push_back(i);
	}
}

Common::UString TalkTable_TLK::readString(const Entry &entry) const {
	if (!entry.text.empty())
		return entry.text;

	if ((entry.length == 0) || !(entry.flags & kFlagTextPresent))
		return "";

	if (_encoding == Common::kEncodingInvalid)
		return "";

	_tlk->seek(entry.offset);

	size_t length = MIN<size_t>(entry.length, _tlk->size() - _tlk->pos());
	if (length == 0)
		return "";

	Common::MemoryReadStream *data   = _tlk->readStream(length);
	Common::MemoryReadStream *parsed = LangMan.preParseColorCodes(*data);

	Common::UString str = Common::readString(*parsed, _encoding);

	delete parsed;
	delete data;

	return str;
}

uint32 TalkTable_TLK::getLanguageID() const {
	return _languageID;
}

void TalkTable_TLK::setLanguageID(uint32 id) {
	_languageID = id;
}

const std::list<uint32> &TalkTable_TLK::getStrRefs() const {
	return _strRefs;
}

bool TalkTable_TLK::getString(uint32 strRef, Common::UString &string, Common::UString &soundResRef) const {
	if (strRef >= _entries.size())
		return false;

	string      = readString(_entries[strRef]);
	soundResRef = _entries[strRef].soundResRef;

	return true;
}

bool TalkTable_TLK::getEntry(uint32 strRef, Common::UString &string, Common::UString &soundResRef,
                             uint32 &volumeVariance, uint32 &pitchVariance, float &soundLength,
                             uint32 &soundID) const {

	if (strRef >= _entries.size())
		return false;

	const Entry &entry = _entries[strRef];

	string      = readString(entry);
	soundResRef = entry.soundResRef;

	volumeVariance = entry.volumeVariance;
	pitchVariance  = entry.pitchVariance;
	soundLength    = entry.soundLength;

	soundID = entry.soundID;

	return true;
}

void TalkTable_TLK::setEntry(uint32 strRef, const Common::UString &string, const Common::UString &soundResRef,
                             uint32 volumeVariance, uint32 pitchVariance, float soundLength,
                             uint32 soundID) {

	if (strRef >= _entries.size()) {
		for (size_t i = _entries.size(); i < strRef; i++)
			_strRefs.push_back(i);

		_strRefs.sort();
		_strRefs.unique();

		_entries.resize(strRef + 1);
	}

	Entry &entry = _entries[strRef];

	entry.text        = string;
	entry.soundResRef = soundResRef;

	entry.volumeVariance = volumeVariance;
	entry.pitchVariance  = pitchVariance;
	entry.soundLength    = soundLength;

	entry.soundID = soundID;

	entry.length = 0;
	entry.offset = 0xFFFFFFFF;

	entry.flags = 0;
	if (!entry.text.empty())
		entry.flags |= kFlagTextPresent;
	if (!entry.soundResRef.empty())
		entry.flags |= kFlagSoundPresent;
	if (entry.soundLength > 0.0f)
		entry.flags |= kFlagSoundLengthPresent;
}

uint32 TalkTable_TLK::getLanguageID(Common::SeekableReadStream &tlk) {
	uint32 id, version;
	bool utf16le;

	AuroraBase::readHeader(tlk, id, version, utf16le);

	if ((id != kTLKID) || ((version != kVersion3) && (version != kVersion4)))
		return kLanguageInvalid;

	return tlk.readUint32LE();
}

uint32 TalkTable_TLK::getLanguageID(const Common::UString &file) {
	Common::ReadFile tlk;
	if (!tlk.open(file))
		return kLanguageInvalid;

	return getLanguageID(tlk);
}

} // End of namespace Aurora
