#include "pacCommand.h"
#include "pacIntrinsicCmd.h"
#include "pacIntrinsicArgHandler.h"
#include "pacArgHandler.h"
#include "pacException.h"
#include "pacLogger.h"
#include "pacStringUtil.h"
#include <boost/regex.hpp>

namespace pac {

//------------------------------------------------------------------------------
Command::Command(const std::string& name, const std::string& ahName /* = ""*/)
    : mName(name), mArgHandler(0) {
  boost::regex re("\\W");
  if (boost::regex_search(mName, re))
    PAC_EXCEPT(
        Exception::ERR_INVALIDPARAMS, "illegal character in\"" + mName + "\" ");

  if (!ahName.empty()) mArgHandler = sgArgLib.createArgHandler(ahName);
}

//------------------------------------------------------------------------------
Command::Command(const Command& rhs)
    : mName(rhs.getName()), mArgHandler(rhs.mArgHandler->clone()) {}

//------------------------------------------------------------------------------
Command::~Command() {
  if (mArgHandler == 0)
    PAC_EXCEPT(Exception::ERR_INVALID_STATE, "0 arg handler");
  delete mArgHandler;
  mArgHandler = 0;
}

//------------------------------------------------------------------------------
void Command::prompt() { mArgHandler->prompt(mArgs); }

//------------------------------------------------------------------------------
bool Command::execute() {
  // right trim
  std::string args = mArgs;
  StringUtil::trim(args, false, true);
  if (mArgHandler->validate(args)) {
    bool res = this->doExecute();
    return res;
  } else {
    outputErrMessage(args);
    return false;
  }
}

//------------------------------------------------------------------------------
void Command::outputErrMessage(const std::string& args) {
  mArgHandler->outputErrMessage(args);
}

//------------------------------------------------------------------------------
void Command::setArgsAndOptions(const std::string& v) {
  boost::smatch m;
  // check if - after nonspace
  boost::regex reInvalid("\\S-");
  if (boost::regex_search(v, reInvalid))
    PAC_EXCEPT(Exception::ERR_INVALIDPARAMS,
        v + " is illegal, - after nonspace character");

  mArgs.clear();
  mOptions.clear();
  // extract options
  boost::regex reOptions("-([a-z]+)\\s*");

  // get options
  std::string::const_iterator start = v.begin();
  std::string::const_iterator end = v.end();
  while (boost::regex_search(start, end, m, reOptions)) {
    mOptions += m[1];
    start = m[0].second;
  }
  // remove options to get args
  mArgs = boost::regex_replace(v, reOptions, "");
  StringUtil::trim(mArgs, true, false);
}

//------------------------------------------------------------------------------
ArgHandler* Command::getArgHandler() const { return mArgHandler; }

//------------------------------------------------------------------------------
bool Command::hasOption(char c) {
  return mOptions.find_first_of(c) != std::string::npos;
}

//------------------------------------------------------------------------------
Command* Command::init() {
  bool isHandlerCreated = mArgHandler;
  if (buildArgHandler()) {
    if (isHandlerCreated) {
      // safety check
      PAC_EXCEPT(Exception::ERR_INVALID_STATE,
          "you should not specify ah name in ctor if you want to build "
          "specific arghandler for command " +
              mName);
      sgArgLib.registerArgHandler(mArgHandler->clone());
    }
  }

  if (!mArgHandler)
    PAC_EXCEPT(Exception::ERR_INVALID_STATE,
        "found 0 handler in " + mName +
            ", you need to pass ahName to Command::Command() or override "
            "Command::buildArgHandler()");

  return this;
}

//------------------------------------------------------------------------------
CommandLib::~CommandLib() {
  // clean command
  std::for_each(mCmdMap.begin(), mCmdMap.end(),
      [&](CmdMap::value_type& v) -> void { delete v.second; });
  mCmdMap.clear();
}

//------------------------------------------------------------------------------
Command* CommandLib::createCommand(const std::string& cmdName) {
  CmdMap::iterator iter = mCmdMap.find(cmdName);
  if (iter != mCmdMap.end()) {
    return iter->second->clone();
  } else {
    return 0;
  }
}

//------------------------------------------------------------------------------
void CommandLib::registerCommand(Command* cmdProto) {
  sgLogger.logMessage("register command " + cmdProto->getName());
  // init it, let it build it's own arg handler
  cmdProto->init();
  // check if it's already registerd
  CmdMap::iterator iter = std::find_if(mCmdMap.begin(), mCmdMap.end(),
      [&](CmdMap::value_type& v)
          -> bool { return v.first == cmdProto->getName(); });

  if (iter == mCmdMap.end()) {
    mCmdMap[cmdProto->getName()] = cmdProto;
  } else {
    PAC_EXCEPT(Exception::ERR_DUPLICATE_ITEM,
        cmdProto->getName() + " already registed!");
  }
}

//------------------------------------------------------------------------------
void CommandLib::init() {
  // register intrinsic commands
  registerCommand(new LsCmd());
  registerCommand(new PwdCmd());
  registerCommand(new CdCmd());
  registerCommand(new SetCmd());
  registerCommand(new GetCmd());
  registerCommand(new SzCmd());
  registerCommand(new CtdCmd());
}
//------------------------------------------------------------------------------
CommandLib::CmdMap::const_iterator CommandLib::beginCmdMapIterator() const {
  return mCmdMap.begin();
}

//------------------------------------------------------------------------------
CommandLib::CmdMap::const_iterator CommandLib::endCmdMapIterator() const {
  return mCmdMap.end();
}

template <>
CommandLib* Singleton<CommandLib>::msSingleton = 0;
}
