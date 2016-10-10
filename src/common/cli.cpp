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

#include <cstring>
#include "src/common/cli.h"

namespace Common {

namespace Cli {

namespace {
void parsetool_set_help(Common::UString &helpStr, const char *longName,
			char shortName, const char *help) {

	helpStr += "  ";
	if (shortName) {
		helpStr += '-';
		helpStr += shortName;
	} else {
		helpStr += "  ";
	}
	helpStr += "      --";
	helpStr += longName;
	helpStr += " ";
	for (int i = (17 - strlen(longName)); i > 0; --i) {
		helpStr += " ";		
	}
	helpStr += help;
	helpStr += "\n";
}
}

template<>
bool ValGetter<int *>::get(const UString &arg) {
	const char *cstr = arg.c_str();
	for (int i = 0; cstr[i]; ++i) {
		if (cstr[i] < '0' || cstr[i] > '9')
			return false;
	}
	*this->val = atoi(arg.c_str());
	return true;
}

template<>
bool ValGetter<UString &>::get(const UString &arg) {
	this->val = arg;
	return true;
}

void Option::doOption(const std::vector<Common::UString> &args, int i, int size) {
	switch (type) {
	case kAssigner:
		assign();
		break;
	case kPrinter:
		if (this->printer.printer) {
			this->printer.printer(this->printer.printerStr);
		} else {
			this->printer.vPrinter();
		}
		break;
	case kCallback:
		if (i + 1 < size)
			this->callback->process(args[i + 1]);
	default:
		break;
	}
}

void Option::assign() {
	for (int i = 0, end = this->assigners.size(); i < end; ++i)
		this->assigners[i]->assign();
}

void Option::free() {
	switch (this->type) {
	case kGetter:
		delete this->getter;
		break;
	case kAssigner:
		for (int i = 0, end = this->assigners.size(); i < end; ++i)
			delete this->assigners[i];
	default:
		break;
	}
}

void NoOption::free() {
	delete this->getter;
}

Parser::Parser(const Common::UString &name, const char *description, const char *bottom, int *returnVal,
	       std::vector<NoOption> endCli)
	: _helpStr(description), _bottom(bottom), _returnVal(returnVal), _noOptions(endCli) {
	_helpStr += "\n\nUsage: ";
	_helpStr += name;
	_helpStr += " [<options>]";
	for (unsigned int i = 0; i < endCli.size(); ++i) {
		if (!endCli[i].isOptional) {
			_helpStr += " <";
			_helpStr += endCli[i].name;
			_helpStr += ">";
		} else {
			_helpStr += " [";
			_helpStr += endCli[i].name;
			_helpStr += "]";
		}
	}
	_helpStr += '\n';
	this->setOption("help", 'h', "This help text", kEndSucess, printUsage, _helpStr);
	this->setOption("version", 0, "Display version information", kEndSucess, printVersion);
}

Parser::~Parser() {
	for (int i = 0, end  = _options.size(); i < end; ++i) {
		if (_options[i])
			_options[i]->free();
		delete _options[i];
	}
	for (int i = 0, end  = _noOptions.size(); i < end; ++i) {
		_noOptions[i].free();
	}
}

void Parser::setOption(const char *longName, char shortName, const char *help, OptionRet ret, 
			  void (*printer)()) {
	parsetool_set_help(_helpStr, longName, shortName, help);
	_options.push_back(new Option(longName, shortName, help, ret, printer));
}

void Parser::setOption(const char *longName, char shortName, const char *help, OptionRet ret,
			  void (*printer)(Common::UString &), Common::UString &str) {
	parsetool_set_help(_helpStr, longName, shortName, help);
	_options.push_back(new Option(longName, shortName, help, ret, printer, str));
}

void Parser::setOption(const char *longName, char shortName, const char *help, OptionRet ret,
			  std::vector<Assigner *> assigners) {
	parsetool_set_help(_helpStr, longName, shortName, help);
	_options.push_back(new Option(longName, shortName, help, ret, assigners));
}

void Parser::setOption(const char *longName, char shortName, const char *help,
		       OptionRet ret, Getter *getter) {
	parsetool_set_help(_helpStr, longName, shortName, help);
	_options.push_back(new Option(longName, shortName, help, ret, getter));
}

void Parser::setOption(const char *longName, char shortName, const char *help,
		       OptionRet ret, CallbackBase *callback) {
	parsetool_set_help(_helpStr, longName, shortName, help);
	_options.push_back(new Option(longName, shortName, help, ret, callback));
}


#define PROCESS_HANDLE_OPTION(prefix, _name)				\
	for (int j = 0, size = _options.size();				\
	     j < size; ++j) {						\
		Option &option = *_options[j];				\
		if (Common::UString(prefix) + option._name == argv[i]) { \
			option.doOption(argv, i, size);			\
			if (option.returnVal == kContinueParsing)	\
				goto nextOption;			\
			*_returnVal = (option.returnVal == kEndFail);	\
			return false;					\
		}							\
	}								\
	this->printUsage(_helpStr);					\
	return false;							\


bool Parser::process(const std::vector<Common::UString> &argv) {
	
	_helpStr += _bottom;
	for ( size_t nbArgs = argv.size(), i = 1; i < nbArgs; ++i) {

		if (isShorOption(argv[i])) {
			PROCESS_HANDLE_OPTION('-', shortName);
		} else if (isLongOption(argv[i])) {
			PROCESS_HANDLE_OPTION("--", name);
		} else {
			for (int j = 0, size = _noOptions.size(); j < size; ++j) {
				_noOptions[0].getter->get(argv[i]);
				_noOptions[0].free();
				_noOptions.erase(_noOptions.begin());
				goto nextOption;
			}
			this->printUsage(_helpStr);
			return false;
		}
	nextOption:
		continue;
	}
	if (_noOptions.size() > 0 && !_noOptions[0].isOptional) {
		this->printUsage(_helpStr);
		return false;	
	}
	return true;
}

#undef PROCESS_HANDLE_OPTION

}

}
