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

#ifndef COMMON_CLI_H
#define COMMON_CLI_H

#include <cstdio>
#include <cstdio>
#include <stdarg.h>
#include <stdlib.h>

#include "src/common/version.h"
#include "src/common/ustring.h"

namespace Common {

namespace Cli {

class Getter;
struct NoOption;

enum OptionRet {
	kContinueParsing = 0,
	kEndSucess,
	kEndFail
};

class Assigner {
public:
	Assigner() {}
	virtual ~Assigner() {}

	virtual void assign() = 0;
};

struct Getter {
public:
	Getter() {}
	virtual ~Getter() {}

	virtual bool get(const UString &arg) = 0;
};


struct CallbackBase {
public:
	virtual ~CallbackBase(){}
	virtual bool process(const UString& str) = 0;
};

template <typename U>
struct Callback : public CallbackBase {
public:
	Callback(bool (*aCallback)(const UString&, U), U anArg) : callback(aCallback),
								  callbackArg(anArg) {}
	virtual ~Callback(){}

	bool (*callback)(const UString& str, U arg);
	bool process(const UString& str) {
		if (callback)
			callback(str, callbackArg);
	}

	U callbackArg;
};

template <typename T>
struct ValGetter : public Getter {
public:
	ValGetter(T aVal) : val(aVal) {}
	virtual ~ValGetter() {}

	T val;
	bool get(const UString &arg);
};

template <typename T>
class ValAssigner : public Assigner {
public:
	ValAssigner(T val, T &target) :
		_val(val), _target(target) {};

	void assign() {_target = _val;}

private:
	T _val;
	T &_target;
};

struct Printer {
	Printer() : vPrinter(NULL), printer(NULL), printerStr(useless) {}
	Printer(const Printer &other) : vPrinter(other.vPrinter), printer(other.printer),
					printerStr(other.printerStr) {}
	Printer(void (*aPrinter)()) : vPrinter(aPrinter), printer(NULL), printerStr(useless) {}
	Printer(void (*aPrinter)(Common::UString &), Common::UString &str) :vPrinter(NULL),
									    printer(aPrinter),
									    printerStr(str) {}
	void (*vPrinter)();
	void (*printer)(Common::UString &);

	UString useless;
	UString &printerStr;
};

struct NoOption {
	NoOption(const char *aName, bool optional, Getter *aGetter) :
		name(aName), isOptional(optional), getter(aGetter) {}

	void free();
	bool isOptional;
	Getter *getter;
	const char *name;
};

struct Option {
public:
	Option(const char *aName, char aShortName,
	       const char *anHelp, OptionRet ret, void (*aPrinter)())
		: type(kPrinter), name(aName), shortName(aShortName),
		  help(anHelp), returnVal(ret),
		  getter(NULL), printer(aPrinter) {}
	Option(const char *aName, char aShortName,
	       const char *anHelp, OptionRet ret, void (*aPrinter)(Common::UString &),
	       Common::UString &printerStr)
		: type(kPrinter), name(aName), shortName(aShortName), help(anHelp),
		  returnVal(ret), getter(NULL), printer(aPrinter, printerStr) {}

	Option(const char *aName, char aShortName,
	       const char *anHelp, OptionRet ret, std::vector<Assigner *> &anAssigners)
		: type(kAssigner), name(aName), shortName(aShortName), help(anHelp),
		  returnVal(ret), assigners(anAssigners), getter(NULL) {}

	Option(const char *aName, char aShortName, const char *anHelp, OptionRet ret, Getter *aGetter)
		: type(kAssigner), name(aName), shortName(aShortName), help(anHelp),
		  returnVal(ret), getter(aGetter) {}

	Option(const char *aName, char aShortName, const char *anHelp,
	       OptionRet ret, CallbackBase *aCallback)
		: type(kAssigner), name(aName), shortName(aShortName), help(anHelp),
		  returnVal(ret), getter(NULL), callback(aCallback) {}

	~Option() {}

	enum Type {
		kAssigner,
		kPrinter,
		kGetter,
		kCallback
	};

	void doOption(const std::vector<Common::UString> &args, int i, int size);
	void assign();
	void free();

	char shortName;
	std::string name;
	std::string help;
	OptionRet returnVal;
	Type type;
	Getter *getter;
	CallbackBase *callback;
	std::vector<Assigner *> assigners;
	Printer printer;
};

class Parser {
public:

	Parser(const UString &name, const char *description, const char *bottom, int *returnVal,
	       std::vector<NoOption> endCli);
	~Parser();

	void setOption(const char *longName, char shortName, const char *help,
		       OptionRet ret, void (*printer)());
	void setOption(const char *longName, char shortName, const char *help,
		       OptionRet ret,
		       void (*printer)(Common::UString &), Common::UString &str);
	void setOption(const char *longName, char shortName, const char *help,
		       OptionRet ret, std::vector<Assigner *> assigners);
	void setOption(const char *longName, char shortName, const char *help,
		       OptionRet ret, Getter *getter);
	void setOption(const char *longName, char shortName, const char *help,
		       OptionRet ret, CallbackBase *callback);

	void setOption(const char *longName, const char *help, OptionRet ret,
		       void (*printer)()) {
		setOption(longName, 0, help, ret, printer);
	}
	void setOption(const char *longName, const char *help, OptionRet ret,
		       void (*printer)(Common::UString &), Common::UString &str) {
		setOption(longName, 0, help, ret, printer, str);
	}
	void setOption(const char *longName, const char *help, OptionRet ret,
		       std::vector<Assigner *> assigners) {
		setOption(longName, 0, help, ret, assigners);
	}
	void setOption(const char *longName, const char *help,
		       OptionRet ret, Getter *getter) {
		setOption(longName, 0, help, ret, getter);
	}
	void setOption(const char *longName, const char *help,
		       OptionRet ret, CallbackBase *callback) {
		setOption(longName, 0, help, ret, callback);
	}
	

	void setOption(char shortName, const char *help, OptionRet ret,
		       void (*printer)()) {
		setOption(NULL, shortName, help, ret, printer);
	}
	void setOption(char shortName, const char *help, OptionRet ret,
		       void (*printer)(Common::UString &), Common::UString &str) {
		setOption(NULL, shortName, help, ret, printer, str);
	}
	void setOption(char shortName, const char *help, OptionRet ret,
		       std::vector<Assigner *> assigners) {
		setOption(NULL, shortName, help, ret, assigners);
	}
	void setOption(char shortName, const char *help,
		       OptionRet ret, Getter *getter) {
		setOption(NULL, shortName, help, ret, getter);
	}
	void setOption(char shortName, const char *help,
		       OptionRet ret, CallbackBase *callback) {
		setOption(NULL, shortName, help, ret, callback);
	}

	bool process(const std::vector<Common::UString> &argv);

private:
	inline bool isShorOption(const Common::UString &arg) const {
		return arg.c_str()[0] == '-' && arg.c_str()[1] != '-';
	}

	inline bool isLongOption(const Common::UString &arg) const {
		return arg.c_str()[0] == '-' && arg.c_str()[1] == '-';
	}

	static inline void printUsage(Common::UString &str) {
		std::printf("%s\n", str.c_str());
	}

	int *_returnVal;
	Common::UString _helpStr;
	Common::UString _bottom;
	std::vector<Option *> _options;
	std::vector<NoOption> _noOptions;	
};




static inline std::vector<NoOption> makeEndArgs(NoOption *noOption) {
	std::vector<NoOption> ret;

	ret.push_back(*noOption);
	return ret;
}

static inline std::vector<NoOption> makeEndArgs(NoOption *noOption1, NoOption *noOption2) {
	std::vector<NoOption> ret;

	ret.push_back(*noOption1);
	ret.push_back(*noOption2);
	return ret;
}

static inline std::vector<NoOption> makeEndArgs(NoOption *noOption1, NoOption *noOption2,
						NoOption *noOption3) {
	std::vector<NoOption> ret;

	ret.push_back(*noOption1);
	ret.push_back(*noOption2);
	ret.push_back(*noOption3);
	return ret;
}

static inline std::vector<Assigner *> makeAssigners(Assigner *assigner) {
	std::vector<Assigner *> ret;

	ret.push_back(assigner);
	return ret;
}

static inline std::vector<Assigner *> makeAssigners(Assigner *assigner1, Assigner *assigner2) {
	std::vector<Assigner *> ret;

	ret.push_back(assigner1);
	ret.push_back(assigner2);
	return ret;
}

static inline std::vector<Assigner *> makeAssigners(Assigner *assigner1, Assigner *assigner2,
						    Assigner *assigner3) {
	std::vector<Assigner *> ret;

	ret.push_back(assigner1);
	ret.push_back(assigner2);
	ret.push_back(assigner3);
	return ret;
}

} // end of namespace Args

} // end of namespace common

#endif
