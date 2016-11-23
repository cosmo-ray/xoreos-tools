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
 *  Tool to convert GFF files into XML.
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

#include "src/xml/gffdumper.h"

typedef std::map<uint32, Common::Encoding> EncodingOverrides;

void printUsage(FILE *stream, const Common::UString &name);
bool parseCommandLine(const std::vector<Common::UString> &argv, int &returnValue,
                      Common::UString &inFile, Common::UString &outFile,
                      Common::Encoding &encoding, Aurora::GameID &game,
                      EncodingOverrides &encOverrides, bool &nwnPremium);

bool parseEncodingOverride(const Common::UString &arg, EncodingOverrides &encOverrides);

void dumpGFF(const Common::UString &inFile, const Common::UString &outFile,
             Common::Encoding encoding, bool nwnPremium);

int main(int argc, char **argv) {
	try {
		std::vector<Common::UString> args;
		Common::Platform::getParameters(argc, argv, args);

		Common::Encoding encoding = Common::kEncodingUTF16LE;
		Aurora::GameID   game     = Aurora::kGameIDUnknown;

		EncodingOverrides encOverrides;

		bool nwnPremium = false;

		int returnValue = 1;
		Common::UString inFile, outFile;

		if (!parseCommandLine(args, returnValue, inFile, outFile, encoding, game, encOverrides, nwnPremium))
			return returnValue;

		LangMan.declareLanguages(game);

		for (EncodingOverrides::const_iterator e = encOverrides.begin(); e != encOverrides.end(); ++e)
			LangMan.overrideEncoding(e->first, e->second);

		dumpGFF(inFile, outFile, encoding, nwnPremium);
	} catch (...) {
		Common::exceptionDispatcherError();
	}

	return 0;
}

bool parseEncodingOverride(const Common::UString &arg, EncodingOverrides &encOverrides) {
	Common::UString::iterator sep = arg.findFirst('=');
	if (sep == arg.end())
		return false;

	uint32 id = 0xFFFFFFFF;
	try {
		Common::parseString(arg.substr(arg.begin(), sep), id);
	} catch (...) {
	}

	if (id == 0xFFFFFFFF)
		return false;

	Common::Encoding encoding = Common::parseEncoding(arg.substr(++sep, arg.end()));
	if (encoding == Common::kEncodingInvalid) {
		status("Unknown encoding \"%s\"", arg.substr(sep, arg.end()).c_str());
		return false;
	}

	encOverrides[id] = encoding;
	return true;
}

bool parseCommandLine(const std::vector<Common::UString> &argv, int &returnValue,
                      Common::UString &inFile, Common::UString &outFile,
                      Common::Encoding &encoding, Aurora::GameID &game,
                      EncodingOverrides &encOverrides, bool &nwnPremium) {

	inFile.clear();
	outFile.clear();
	using Common::Cli::NoOption;
	using Common::Cli::kContinueParsing;
	using Common::Cli::Parser;
	using Common::Cli::ValGetter;
	using Common::Cli::Callback;
	using Common::Cli::ValAssigner;
	using Common::Cli::makeEndArgs;
	using Common::Cli::makeAssigners;
	using Aurora::GameID;

	Common::UString encodingStr;
	bool optionsEnd = false;
	std::vector<Common::Cli::Getter *> getters;
	NoOption inFileOpt("input file", false, new ValGetter<Common::UString &>(inFile));
	NoOption outFileOpt("output file", true, new ValGetter<Common::UString &>(outFile));
	Parser parser(argv[0], "BioWare GFF to XML converter",
		      "If no output file is given, the output is written to stdout.\n\n"
		      "Depending on the game, LocStrings in GFF files might be encoded in various\n"
		      "ways and there's no way to autodetect how. If a game is specified, the\n"
		      "encoding tables for this game are used. Otherwise, gff2xml tries some\n"
		      "heuristics that might fail for certain strings.\n\n"
		      "Additionally, the --encoding parameter can be used to override the encoding\n"
		      "for a specific language ID. The string has to be of the form n=encoding,\n"
		      "for example 0=cp-1252 to override the encoding of the (ungendered) language\n"
		      "ID 0 to be Windows codepage 1252. To override several encodings, specify\n"
		      "the --encoding parameter multiple times.\n",
		      &returnValue,
		      makeEndArgs(&inFileOpt, &outFileOpt));


	parser.setOption("cp1252", "Read GFF4 strings as Windows CP-1252", kContinueParsing,
			 makeAssigners(new ValAssigner<Common::Encoding>(Common::kEncodingCP1252,
									 encoding)));
	parser.setOption("nwnpremium", "This is a broken GFF from a Neverwinter Nights premium module",
			 kContinueParsing,
			 makeAssigners(new ValAssigner<bool>(true, nwnPremium)));
	parser.setOption("nwn", "Use Neverwinter Nights encodings", kContinueParsing,
			 makeAssigners(new ValAssigner<GameID>(Aurora::kGameIDNWN, game)));
	parser.setOption("nwn2", "Use Neverwinter Nights 2 encodings", kContinueParsing,
			 makeAssigners(new ValAssigner<GameID>(Aurora::kGameIDNWN2, game)));
	parser.setOption("kotor", "Use Knights of the Old Republic encodings", kContinueParsing,
			 makeAssigners(new ValAssigner<GameID>(Aurora::kGameIDKotOR, game)));
	parser.setOption("kotor2", "Use Knights of the Old Republic II encodings", kContinueParsing,
			 makeAssigners(new ValAssigner<GameID>(Aurora::kGameIDKotOR2, game)));
	parser.setOption("jade", "Use Jade Empire encodings", kContinueParsing,
			 makeAssigners(new ValAssigner<GameID>(Aurora::kGameIDJade, game)));
	parser.setOption("witcher", "Use The Witcher encodings", kContinueParsing,
			 makeAssigners(new ValAssigner<GameID>(Aurora::kGameIDWitcher, game)));
	parser.setOption("dragonage", "Use Dragon Age encodings", kContinueParsing,
			 makeAssigners(new ValAssigner<GameID>(Aurora::kGameIDDragonAge, game)));
	parser.setOption("dragonage2", "Use Dragon Age II encodings", kContinueParsing,
			 makeAssigners(new ValAssigner<GameID>(Aurora::kGameIDDragonAge2, game)));
	parser.setOption("dragonage2", "Use Dragon Age II encodings", kContinueParsing,
			 makeAssigners(new ValAssigner<GameID>(Aurora::kGameIDDragonAge2, game)));
	parser.setOption("encoding", "Override an encoding", kContinueParsing,
			 new Callback<EncodingOverrides &>(parseEncodingOverride, encOverrides));

	return parser.process(argv);
}


void dumpGFF(const Common::UString &inFile, const Common::UString &outFile,
             Common::Encoding encoding, bool nwnPremium) {

	Common::SeekableReadStream *gff = new Common::ReadFile(inFile);

	XML::GFFDumper *dumper = 0;
	try {
		dumper = XML::GFFDumper::identify(*gff, nwnPremium);
	} catch (...) {
		delete gff;
		throw;
	}

	Common::WriteStream *out = 0;
	try {

		if (!outFile.empty())
			out = new Common::WriteFile(outFile);
		else
			out = new Common::StdOutStream;

		dumper->dump(*out, gff, encoding, nwnPremium);

	} catch (...) {
		delete dumper;
		delete out;
		throw;
	}

	out->flush();

	if (!outFile.empty())
		status("Converted \"%s\" to \"%s\"", inFile.c_str(), outFile.c_str());

	delete dumper;
	delete out;
}
