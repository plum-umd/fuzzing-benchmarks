/* 
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is the Sablotron XSLT Processor.
 * 
 * The Initial Developer of the Original Code is Ginger Alliance Ltd.
 * Portions created by Ginger Alliance are Copyright (C) 2000-2002
 * Ginger Alliance Ltd. All Rights Reserved.
 * 
 * Contributor(s):
 * 
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 or later (the
 * "GPL"), in which case the provisions of the GPL are applicable 
 * instead of those above.  If you wish to allow use of your 
 * version of this file only under the terms of the GPL and not to
 * allow others to use your version of this file under the MPL,
 * indicate your decision by deleting the provisions above and
 * replace them with the notice and other provisions required by
 * the GPL.  If you do not delete the provisions above, a recipient
 * may use your version of this file under either the MPL or the
 * GPL.
 */

#ifndef SituaHIncl
#define SituaHIncl

// GP: clean

#include "base.h"
#include "error.h"
#include "sxpath.h"

//enum ErrCode;

/*
Mode constants for the situation object.
*/

enum MsgType
{
    MT_ERROR,
        MT_WARN,
        MT_LOG
};

enum MsgDestination
{
    MD_NONE,
        MD_FILE,
        MD_CHERROR
};

/*
The following class keeps track of the current situation: which vertex
is currently being parsed/processed etc. This is used solely for error reporting.
*/

class XSLElement;
class Vertex;
#include "datastr.h"

class Processor;
class DOMProvider;
class DOMProviderStandard;
class DOMProviderUniversal;

class SituaInfo
{
public:
    MsgCode pending;
    Vertex *currV;
    Str currFile;
    Str currMsg;
    int currLine;
    int currSAXLine;
    int SDOMExceptionCode;
    SituaInfo()
    {
        pending = E_OK;
	currLine = 0;
	currSAXLine = 0;
	SDOMExceptionCode = 0;
    }
    SituaInfo& operator= (const SituaInfo& other);
    void clear();
};

class Recoder;

class Situation
{
public:
    Situation();
    ~Situation();
    
    Recoder& recoder() const;
    
    void setFlag(SablotFlag f);
    Bool hasFlag(SablotFlag f);
    void resetFlag(SablotFlag f);
    void setFlags(int f);
    int getFlags();

    // set vertex information for the current message 
    void setCurrV(Vertex *v);
    
    // same, including the document identification 
    void setCurrVDoc(Vertex *v);
    
    // set file (URI) information for the current message 
    void setCurrFile(const Str &);
    
    // set line number information for the current message 
    void setCurrLine(int);

    // set line number information while SAX parsing is in progress
    void setCurrSAXLine(int);
    int getCurrSAXLine();
    
    // set pointer to associated Processor (will be NULL if not processing)
    void setProcessor(Processor *P) {proc = P;}
    
    // get pointer to associated Processor
    Processor* getProcessor() const {return proc;}
    
    // common messaging routine for reporting errors, warnings and log messages
    // (distinguished by 'type')
    void message(MsgType type, MsgCode code, const Str &arg1, const Str &arg2);
    
    // reset the situation's state
    void clear();
    
    // clear the error-related information
    void clearError();
    
    // detect pending error
    Bool isError();
    
    // get pending error code
    int getError() const;
    
    // set output files for errors and for log messages
    eFlag msgOutputFile(char *_errwfn, char *_logfn);
    
    // erase the log
    eFlag eraseLog(char *newLogFile);
    
    // return current time
    Str timeStr();
    
    // open the default output files
    eFlag openDefaultFiles();
    
    // close files in use
    eFlag closeFiles();
    
    // return the pending error message (with all substitutions)
    const Str& getMessage();
    
    void setSXPOptions(unsigned long options);
    unsigned long getSXPOptions();
    void setSXPMaskBit(int mask);
    int getSXPMaskBit();

    void setSDOMExceptionCode(int code);
    
    int getSDOMExceptionCode() const;
    
    void getSDOMExceptionExtra(MsgCode& theCode, 
			       Str& theMessage, Str& theDocument, 
			       int& theLine) const;
    
    void swapProcessor(void *&proc_);
    
    const Str& findBaseURI(const Str& unmappedBase);
    
    void setDOMProvider(DOMHandler *domh, void* udata);
    
    DOMProvider& dom() {return *(DOMProvider*)theProvider; }
    
    Bool domExternal(NodeHandle n) {return (unsigned long)n & 1;}
private:
    Processor* proc;
    SituaInfo info, infoDOM;
    
    void generateMessage(MsgType, MsgCode, const Str &arg1, const Str &arg2,
        Str &theMessage);
    
    // report error (in this case to self)
    void report(Situation *S, MsgType, MsgCode, const Str &, const Str &);
    FILE *logfile, *errwfile;
    int flags;
    unsigned long sxpOptions;
    Recoder *theRecoder;
    DOMProviderUniversal *theProvider;
};

typedef Situation & Sit;

#endif // SituaHIncl
