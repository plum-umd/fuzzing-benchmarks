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

#include <time.h>
#include "base.h"
#include "situa.h"
#include "verts.h"
#include "encoding.h"
#include "tree.h"
#include "domprovider.h"
#include "platform.h"

// GP: clean

SituaInfo& SituaInfo::operator=(const SituaInfo& other)
{
    pending = other.pending;
    currV = other.currV;
    currFile = other.currFile;
    currMsg = other.currMsg;
    currLine = other.currLine;
    SDOMExceptionCode = other.SDOMExceptionCode;
    return *this;
}

void SituaInfo::clear()
{
    pending = E_OK;
    currV = NULL;
    currFile.empty();
    currMsg.empty();
    currLine = 0;
    SDOMExceptionCode = 0;
}


/*****************************************************************

    S   i   t   u   a   t   i   o   n 

*****************************************************************/

Situation::Situation()
{
    theRecoder = new Recoder;
    clear();
    proc = NULL;
    logfile = errwfile = NULL;
    openDefaultFiles();
    flags = 0;

// define this symbol to disable adding the meta tag
#ifdef SABLOT_DISABLE_ADDING_META
    flags |= SAB_DISABLE_ADDING_META;
#endif

//define thos symbol to avoid errors in document() when not found
#ifdef SABLOT_DISABLE_DOC_ERRORS
    flags |= SAB_IGNORE_DOC_NOT_FOUND;
#endif

    sxpOptions = 0;
    theProvider = new DOMProviderUniversal();
}

Situation::~Situation()
{
    theRecoder -> clear(*this);
    cdelete(theRecoder);
    if (logfile) stdclose(logfile);
    if (errwfile) stdclose(errwfile);
    cdelete(theProvider);
}

Recoder& Situation::recoder() const
{
    return *theRecoder;
}

void Situation::setFlag(SablotFlag f)
{
    flags |= f;
}

Bool Situation::hasFlag(SablotFlag f)
{
  return flags & f;
}

void Situation::resetFlag(SablotFlag f)
{
    flags &= ~f;
}

void Situation::setFlags(int f)
{
    flags = f;
}

int Situation::getFlags()
{
  return flags;
}

Str Situation::timeStr()
{
    time_t currtime;
    time(&currtime);
    return asctime(localtime(&currtime));
}

void Situation::setCurrV(Vertex *v)
{
    info.currV = v;
    if (v)
      {
	info.currLine = v -> lineno;
      }
}

void Situation::setCurrVDoc(Vertex *v)
{    
    setCurrV(v);
    if (v)
    {
	info.currFile = v -> getSubtreeInfo() -> getBaseURI();
    }
}

void Situation::setCurrLine(int lno)
{
    info.currLine = lno;
}

void Situation::setCurrSAXLine(int lno)
{
    info.currSAXLine = lno;
}

int Situation::getCurrSAXLine()
{
  return info.currSAXLine;
}

void Situation::setCurrFile(const Str& fname)
{
    info.currFile = fname;
};

eFlag Situation::openDefaultFiles()
{
    E( msgOutputFile((char *) "/__stderr", NULL) );
    return OK;
}

eFlag Situation::closeFiles()
{
    if (logfile)
        stdclose(logfile);
    logfile = NULL;
    if (errwfile)
        stdclose(errwfile);
    errwfile = NULL;
    return OK;
}

eFlag Situation::eraseLog(char *newLogFile)
{
    if (logfile)
        stdclose(logfile);
    logfile = NULL;
    if (newLogFile)
    {
        if (!(logfile = stdopen(newLogFile,"w")))
          Err1(this, E_FILE_OPEN, newLogFile);
	setlinebuf__(logfile);
    }
    return OK;
}

eFlag Situation::msgOutputFile(char *_errwfn, char *_logfn)
{
    E( closeFiles() );
    if (_logfn)
    {
        if (!(logfile = stdopen(_logfn,"a")))
          Err1(this, E_FILE_OPEN, _logfn);
	setlinebuf__(logfile);
    }
    if (_errwfn)
    {
        if (!(errwfile = stdopen(_errwfn,"w")))
          Err1(this, E_FILE_OPEN, _errwfn);
	setlinebuf__(errwfile);
    }
    return OK;
}

// constructMsgFields
// called to transform a List of Str's to a NULL-terminated array
// of char*'s. Stores just POINTERS so make sure the strings don't change.
// Dispose of the return value using delete[].

char** constructMsgFields(PList<DStr*>& strings)
{
    int len = strings.number();
    char **p = new char*[len + 1];
    p[len] = NULL;
    int i;
    for (i = 0; i < len; i++)
        p[i] = (char*)(*strings[i]);
    return p;
}

#define __numargs 3
void safeFormat(char* dest, int size, const char* format, 
		const char* arg1, const char* arg2, const char* arg3)
{
  int l[__numargs];
  char *b[__numargs];
  const char* a[__numargs];
  int i;
  int total = 0, num = 0;

  a[0] = arg1;
  a[1] = arg2;
  a[2] = arg3;
  memset(l, 0, 3 * sizeof(int));
  memset(b, 0, 3 * sizeof(char*));
  num = 0;

  //read lengths
  for (i = 0; i < __numargs; i++)
    if (a[i]) { l[i] = strlen(a[i]); num++; total += l[i];}

  //replace too long buffers
  for (i = 0; i < __numargs; i++)
    {
      if (l[i] > size / __numargs)
	{
	  b[i] = new char[size / __numargs + 1];
	  strcpy(b[i], "...");
	  strcpy(b[i] + 3, a[i] + (l[i] - size / __numargs + 3));
	}
    }
  //format
  sprintf(dest, format, 
	  b[0] ? b[0] : a[0], 
	  b[1] ? b[1] : a[1], 
	  b[2] ? b[2] : a[2]);

  //delete allocated
  for (i = 0; i < __numargs; i++)
    if (b[i]) delete[] b[i];
}

#define __MSG_BUFSIZE 512
void Situation::generateMessage(MsgType type, MsgCode code, 
				const Str& arg1, const Str& arg2,
				Str& theMessage)
{
    char buf[__MSG_BUFSIZE];
    PList<DStr*> out;
    void *messengerUD = NULL;
    MessageHandler *messenger = NULL;
    if (proc)
      messenger = proc -> getMessageHandler(&messengerUD);
    if (messenger)
      {
        out.append(new DStr("msgtype:"));
        switch(type) {
	case MT_ERROR: *(out[0]) += "error"; break;
	case MT_WARN: *(out[0]) += "warning"; break;
	case MT_LOG: *(out[0]) += "log"; break;
	};
      }
    if (type != MT_LOG)
      {
        sprintf(buf,"code:%d",code);
        out.append(new DStr(buf));
      }
    if (messenger)
      out.append(new DStr("module:Sablotron"));
    if (!info.currFile.isEmpty())
      {
	//check for buffer overflow
        //sprintf(buf,"URI:%s",(char*)(info.currFile));
	safeFormat(buf, __MSG_BUFSIZE - 5, 
		   "URI:%s", 
		   (char*)(info.currFile), 
		   NULL, NULL);
        out.append(new DStr(buf));
      }
    if (info.currLine && type != MT_LOG)
      {
        sprintf(buf,"line:%d",info.currLine);
        out.append(new DStr(buf));
      }
    if (info.currV && type != MT_LOG)
      {
	//may overflow for extremly (~500 bytes) long tag names
        DStr nameStr;
        info.currV -> speak(nameStr, SM_NAME);
        //sprintf(buf,"node:%s%s'%s'",
	//	vertexTypeNames[info.currV -> vt & VT_BASE],
	//	(info.currV -> vt == VT_VERTEX ? "" : " "),
	//	(char *) nameStr);
	safeFormat(buf, __MSG_BUFSIZE - 10, 
		   "node:%s%s'%s'",
		   vertexTypeNames[info.currV -> vt & VT_BASE],
		   (info.currV -> vt == VT_VERTEX ? "" : " "),
		   (char *) nameStr);
        out.append(new DStr(buf));
      }

    SabMsg *p = GetMessage(code);
    if (p -> text[0])
      {
	//check buffer overflow
        DStr msgText = messenger ? (char*)"msg:" : (char*)"";
        //sprintf(buf,p -> text,(char*)(Str&)arg1,(char*)(Str&)arg2);
        safeFormat(buf, __MSG_BUFSIZE - strlen(p -> text),
		   p -> text,
		   (char*)(Str&)arg1,
		   (char*)(Str&)arg2,
		   NULL);
        msgText += buf;
        out.append(new DStr(msgText));
      }
    
    if (messenger && !(flags & SAB_NO_ERROR_REPORTING))
      {
        // construct the message fields
        char **msgFields = constructMsgFields(out);
        MH_ERROR externalCode = 
	  messenger -> makeCode(messengerUD, proc,
				type == MT_ERROR ? 1 : 0, 
                                MH_FACILITY_SABLOTRON, (unsigned short)code);
        
        // FIXME: casting to MH_LEVEL -- necessary?
        switch(type) {
	case MT_ERROR:
	  {
	    messenger -> error(messengerUD, proc,
			       externalCode, (MH_LEVEL) MH_LEVEL_ERROR, 
			       msgFields); 
	  }; break;
	case MT_WARN:
	  {
	    messenger -> log(messengerUD, proc,
			   externalCode, (MH_LEVEL) MH_LEVEL_WARN, 
			     msgFields); 
	  }; break;
	case MT_LOG:
	  {
	    messenger -> log(messengerUD, proc,
			   externalCode, (MH_LEVEL) MH_LEVEL_INFO, 
			     msgFields); 
	  }; break;
        }
        delete[] msgFields;
        // note that the strings pointed at by msgFields members are deleted
        // upon destruction of 'out'
      };
    
    // in any case, construct the message and return it in theMessage
    // construct the message
    DStr fullout;
    if (type != MT_LOG)
      {
        fullout = GetMessage((MsgCode)(MSG_ERROR + type)) -> text;
        fullout += " ";
        int outnum = out.number();
        for (int j = 0; j < outnum; j++)
	  {
            if (j < outnum - 1)
	      fullout += "[";
            fullout += *out[j];
            if (j < outnum - 1)
	      fullout += "] ";
            if (j == outnum-2)
	      fullout += "\n  ";
	  }
      }
    else
      {
        if (out.number())
	  fullout = *(out.last());
      }
    
    if (!messenger && !(type == MT_ERROR && (flags & SAB_NO_ERROR_REPORTING)))
      {
        // display the message yourself
        FILE *thefile = (type == MT_LOG ? logfile : errwfile);
        if (thefile)
	  fprintf(thefile,"%s\n",(char*) fullout);
      }
    
    theMessage = fullout;
    
    // dispose of the temp string array
    out.freeall(FALSE);
}

void Situation::message(MsgType type, MsgCode code, 
                 const Str& arg1, const Str& arg2)
{
    if (code == E2_SDOM)
    {
        infoDOM = info;
	    info.clear();
    }
    else 
    {
        if (type == MT_ERROR)
            infoDOM.clear();
	}

    Str temp;
    if (type == MT_ERROR)
        info.pending = code;
    generateMessage(type, code, arg1, arg2, temp);
    info.currMsg = temp;

    // only log errors and warnings if we are using our own reporter
    if ((type == MT_ERROR || type == MT_WARN) && 
        (!proc || !proc -> getMessageHandler(NULL)))
        generateMessage(MT_LOG, code, arg1, arg2, temp);

#ifdef SABLOT_ABORT_ON_ERROR
    if (type == MT_ERROR) abort();
#endif

}

void Situation::report(Situation *sit, MsgType type, MsgCode code, const Str &str1, const Str &str2)
{
    message(type, code, str1, str2);
}


Bool Situation::isError()
{
    return (Bool) (info.pending != E_OK);
};

int Situation::getError() const
{
    return info.pending;
}

void Situation::clearError()
{
    info.pending = E_OK;
    info.currMsg.empty();
}

void Situation::clear()
{
    info.clear();
    infoDOM.clear();
}

void Situation::setSDOMExceptionCode(int code)
{
    info.SDOMExceptionCode = code;
}

int Situation::getSDOMExceptionCode() const
{
    // the exception code was moved to infoDOM by message()
    return infoDOM.SDOMExceptionCode;
}

void Situation::getSDOMExceptionExtra(MsgCode& theCode, 
    Str& theMessage, Str& theDocument, int& theLine) const
{
    theCode = infoDOM.pending;
    theMessage = infoDOM.currMsg;
    theDocument = infoDOM.currFile;
    theLine = infoDOM.currLine;    
}

void Situation::swapProcessor(void *& proc_)
{
    void *temp = proc;
    proc = (Processor*) proc_;
    proc_ = temp;
}

const Str& Situation::findBaseURI(const Str& unmappedBase)
{
    if (proc)
      return proc -> findBaseURI(*this, unmappedBase);
    else
      return unmappedBase;
}

void Situation::setSXPOptions(unsigned long options)
{
  theProvider -> setOptions(options);
}

unsigned long Situation::getSXPOptions()
{
  return theProvider -> getOptions();
}

void Situation::setSXPMaskBit(int mask)
{
  theProvider -> setMaskBit(mask);
}

int Situation::getSXPMaskBit()
{
  return theProvider -> getMaskBit();
}

void Situation::setDOMProvider(DOMHandler *domh, void* udata)
{
  theProvider -> setExtProvider(domh, udata);
}

