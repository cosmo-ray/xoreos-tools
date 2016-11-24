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
 *  Tool to convert TLK files into XML.
 */

#include <cstring>
#include <cstdio>

#include "src/common/version.h"
#include "src/common/ustring.h"
#include "src/common/util.h"
#include "src/common/strutil.h"
#include "src/common/error.h"
#include "src/common/platform.h"
#include "src/common/readfile.h"
#include "src/common/writefile.h"
#include "src/common/stdoutstream.h"
#include "src/common/encoding.h"
#include "src/common/cli.h"

#include "src/aurora/types.h"
#include "src/aurora/language.h"

#include "src/xml/tlkdumper.h"

bool parseCommandLine(const std::vector<Common::UString> &argv, int &returnValue,
                      Common::UString &inFile, Common::UString &outFile,
                      Common::Encoding &encoding, Aurora::GameID &game);

void dumpTLK(const Common::UString &inFile, const Common::UString &outFile, Common::Encoding encoding);

int main(int argc, char **argv) {
	try {
		std::vector<Common::UString> args;
		Common::Platform::getParameters(argc, argv, args);

		Common::Encoding encoding = Common::kEncodingInvalid;
		Aurora::GameID   game     = Aurora::kGameIDUnknown;

		int returnValue = 1;
		Common::UString inFile, outFile;

		if (!parseCommandLine(args, returnValue, inFile, outFile, encoding, game))
			return returnValue;

		LangMan.declareLanguages(game);

		dumpTLK(inFile, outFile, encoding);
	} catch (...) {
		Common::exceptionDispatcherError();
	}

	return 0;
}

bool parseCommandLine(const std::vector<Common::UString> &argv, int &returnValue,
                      Common::UString &inFile, Common::UString &outFile,
                      Common::Encoding &encoding, Aurora::GameID &game) {

	inFile.clear();
	outFile.clear();
	
	using Common::Cli::NoOption;
	using Common::Cli::kContinueParsing;
	using Common::Cli::Parser;
	using Common::Cli::ValGetter;
	using Common::Cli::ValAssigner;
	using Common::Cli::makeEndArgs;
	using Common::Cli::makeAssigners;
	using Common::Encoding;
	using Aurora::GameID;

	bool optionsEnd = false;
	std::vector<Common::Cli::Getter *> getters;
	NoOption inFileOpt("input files", false, new ValGetter<Common::UString &>(inFile));
	NoOption outFileOpt("output files", true, new ValGetter<Common::UString &>(outFile));
	Parser parser(argv[0], "BioWare TLK to XML converter",
		      "There is no way to autodetect the encoding of strings in TLK files,\n"
		      "so an encoding must be specified. Alternatively, the game this TLK\n"
		      "is from can be given, and an appropriate encoding according to that\n"
		      "game and the language ID found in the TLK is used.\n",
		      &returnValue,
		      makeEndArgs(&inFileOpt, &outFileOpt));


	parser.setOption("cp1250", "Read TLK strings as Windows CP-1250", kContinueParsing,
			 makeAssigners(new ValAssigner<Encoding>(Common::kEncodingCP1250, encoding),
				       new ValAssigner<GameID>(Aurora::kGameIDUnknown, game)));
	parser.setOption("cp1251", "Read TLK strings as Windows CP-1251", kContinueParsing,
			 makeAssigners(new ValAssigner<Encoding>(Common::kEncodingCP1251, encoding),
				       new ValAssigner<GameID>(Aurora::kGameIDUnknown, game)));
	parser.setOption("cp1252", "Read TLK strings as Windows CP-1252", kContinueParsing,
			 makeAssigners(new ValAssigner<Encoding>(Common::kEncodingCP1252, encoding),
				       new ValAssigner<GameID>(Aurora::kGameIDUnknown, game)));
	parser.setOption("cp932", "Read TLK strings as Windows CP-932", kContinueParsing,
			 makeAssigners(new ValAssigner<Encoding>(Common::kEncodingCP932, encoding),
				       new ValAssigner<GameID>(Aurora::kGameIDUnknown, game)));
	parser.setOption("cp936", "Read TLK strings as Windows CP-936", kContinueParsing,
			 makeAssigners(new ValAssigner<Encoding>(Common::kEncodingCP936, encoding),
				       new ValAssigner<GameID>(Aurora::kGameIDUnknown, game)));
	parser.setOption("cp949", "Read TLK strings as Windows CP-949", kContinueParsing,
			 makeAssigners(new ValAssigner<Encoding>(Common::kEncodingCP949, encoding),
				       new ValAssigner<GameID>(Aurora::kGameIDUnknown, game)));
	parser.setOption("cp950", "Read TLK strings as Windows CP-950", kContinueParsing,
			 makeAssigners(new ValAssigner<Encoding>(Common::kEncodingCP950, encoding),
				       new ValAssigner<GameID>(Aurora::kGameIDUnknown, game)));
	parser.setOption("utf8", "Read TLK strings as UTF-8", kContinueParsing,
			 makeAssigners(new ValAssigner<Encoding>(Common::kEncodingUTF8, encoding),
				       new ValAssigner<GameID>(Aurora::kGameIDUnknown, game)));
	parser.setOption("utf16le", "Read TLK strings as little-endian UTF-16", kContinueParsing,
			 makeAssigners(new ValAssigner<Encoding>(Common::kEncodingUTF16LE, encoding),
				       new ValAssigner<GameID>(Aurora::kGameIDUnknown, game)));
	parser.setOption("utf16be", "Read TLK strings as big-endian UTF-16", kContinueParsing,
			 makeAssigners(new ValAssigner<Encoding>(Common::kEncodingUTF16BE, encoding),
				       new ValAssigner<GameID>(Aurora::kGameIDUnknown, game)));
	parser.setOption("nwn", "Use Neverwinter Nights encodings", kContinueParsing,
			 makeAssigners(new ValAssigner<Encoding>(Common::kEncodingInvalid, encoding),
				       new ValAssigner<GameID>(Aurora::kGameIDNWN, game)));
	parser.setOption("nwn2", "Use Neverwinter Nights 2 encodings", kContinueParsing,
			 makeAssigners(new ValAssigner<Encoding>(Common::kEncodingInvalid, encoding),
				       new ValAssigner<GameID>(Aurora::kGameIDNWN2, game)));
	parser.setOption("kotor", "Use Knights of the Old Republic encodings", kContinueParsing,
			 makeAssigners(new ValAssigner<Encoding>(Common::kEncodingInvalid, encoding),
				       new ValAssigner<GameID>(Aurora::kGameIDKotOR, game)));
	parser.setOption("kotor2", "Use Knights of the Old Republic II encodings", kContinueParsing,
			 makeAssigners(new ValAssigner<Encoding>(Common::kEncodingInvalid, encoding),
				       new ValAssigner<GameID>(Aurora::kGameIDKotOR2, game)));
	parser.setOption("jade", "Use Jade Empire encodings", kContinueParsing,
			 makeAssigners(new ValAssigner<Encoding>(Common::kEncodingInvalid, encoding),
				       new ValAssigner<GameID>(Aurora::kGameIDJade, game)));
	parser.setOption("witcher", "Use The Witcher encodings", kContinueParsing,
			 makeAssigners(new ValAssigner<Encoding>(Common::kEncodingInvalid, encoding),
				       new ValAssigner<GameID>(Aurora::kGameIDWitcher, game)));
	parser.setOption("dragonage", "Use Dragon Age encodings", kContinueParsing,
			 makeAssigners(new ValAssigner<Encoding>(Common::kEncodingInvalid, encoding),
				       new ValAssigner<GameID>(Aurora::kGameIDDragonAge, game)));
	parser.setOption("dragonage2", "Use Dragon Age II encodings", kContinueParsing,
			 makeAssigners(new ValAssigner<Encoding>(Common::kEncodingInvalid, encoding),
				       new ValAssigner<GameID>(Aurora::kGameIDDragonAge2, game)));

	return parser.process(argv);
}

void dumpTLK(const Common::UString &inFile, const Common::UString &outFile, Common::Encoding encoding) {
	Common::SeekableReadStream *tlk = new Common::ReadFile(inFile);

	Common::WriteStream *out = 0;
	try {
		if (!outFile.empty())
			out = new Common::WriteFile(outFile);
		else
			out = new Common::StdOutStream;

	} catch (...) {
		delete tlk;
		throw;
	}

	try {
		XML::TLKDumper::dump(*out, tlk, encoding);
	} catch (...) {
		delete out;
		throw;
	}

	out->flush();

	if (!outFile.empty())
		status("Converted \"%s\" to \"%s\"", inFile.c_str(), outFile.c_str());

	delete out;
}
