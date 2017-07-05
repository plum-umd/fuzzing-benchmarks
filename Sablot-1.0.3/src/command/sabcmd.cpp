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
 * Portions created by Ginger Alliance are Copyright (C) 2000-2003
 * Ginger Alliance Ltd. All Rights Reserved.
 * 
 * Contributor(s): Christian Lefebvre, Stefan Behnel
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

/*****************************************************************

  s a b c m d

*****************************************************************/

/*****************************************************************
defines
*****************************************************************/

#define BARE_SIGN '@'

#define MAX_BARE        64
#define MAX_EQ          16
#define MAX_ARG_OR_PAR  MAX_EQ

/*****************************************************************
includes
*****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sablot.h>

// include for time measurement
#include <time.h>      // needed by <sys/timeb.h> for definition of time_t

#if defined(HAVE_CONFIG_H)
#include <config.h>
#endif

#ifdef SABLOT_DEBUGGER
#include <sabdbg.h>
#endif

#if defined(HAVE_SYS_TIMEB_H) || defined(WIN32)
#include <sys/timeb.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h> // needed too in Windows
#endif

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

/*****************************************************************
vars to hold the results of switch parsing
*****************************************************************/

int batchMode = 0,
  numberTimes = 0,
  numberMode = 0,
  numberMeasure = 0,
  commandFlags = 0;

char * stringBase = NULL,
    * stringLog = NULL;

/*****************************************************************
switch definitions
*****************************************************************/

typedef enum 
{
  ID_NONE,
  ID_SPS, ID_SPF, ID_SPS_ON_FILES, ID_BASE, ID_TIMES,
  ID_LOG, ID_HELP, ID_DEBUG, ID_DEBUGGER, ID_DBG_HELP, ID_FLAGS, 
  ID_VERSION, ID_MEASURE, ID_BATCH_XML, ID_BATCH_XSL,
  ID_CHAIN_XSL
} SwitchId;

struct SwitchData
{
  SwitchId id;
  const char *longName;
  char shortName;
  char type;  // Integer, String, Ordinal
  void *destination;
};

struct SwitchData switches[] =
{
    {ID_SPS, "use-SPS", 'S', 'O', &numberMode},   
    {ID_SPF, "use-SPF", 'F', 'O', &numberMode},   
    {ID_SPS_ON_FILES, "use-SPS-on-files", 0, 'O', &numberMode},
    {ID_BASE, "base", 'b', 'S', &stringBase},            
    {ID_TIMES, "times", 't', 'N', &numberTimes},          
    {ID_LOG, "log-file", 'L', 'S', &stringLog},         
    {ID_HELP, "help", '?', 'O', &numberMode},            
    {ID_DEBUG, "debug", 0, 'O', &numberMode},
    {ID_DEBUGGER, "debugger", 0, 'O', &numberMode},
    {ID_DBG_HELP, "debug-options", 0, 'O', &numberMode},
    {ID_FLAGS, "flags", 'f', 'N', &commandFlags},
    {ID_VERSION, "version", 'v', 'O', &numberMode},
    {ID_MEASURE, "measure", 'm', 'O', &numberMeasure},
    {ID_BATCH_XML, "batch-xml", 'x', 'O', &batchMode},
    {ID_BATCH_XSL, "batch-xsl", 's', 'O', &batchMode},
    {ID_CHAIN_XSL, "chain-xsl", 'c', 'O', &batchMode},
    {ID_NONE, NULL, 0, 0, NULL }
};

/*****************************************************************
texts
*****************************************************************/

char usage[] = 
"\nUsage for single XSLT:\n"\
"\tsabcmd [options] <stylesheet> [<input> [<output>]] [assignments]\n"\
"\nUsage in batch processing:\n"\
"    chain multiple stylesheets, starting with a single input file:\n"\
"    \tsabcmd [options] --chain-xsl <input> <output> [<stylesheet>]+ [assignments]\n"\
"    run multiple stylesheets on a single input file:\n"\
"    \tsabcmd [options] --batch-xml <input> [<stylesheet> <output>]+ [assignments]\n"\
"    run a single stylesheet on multiple input files:\n"\
"    \tsabcmd [options] --batch-xsl <stylesheet> [<input> <output>]+ [assignments]\n"\
"Options:\n"\
"\t--chain-xsl, -c      single input file, multiple chained stylesheets\n"\
"\t--batch-xml, -x      single input file, multiple stylesheets\n"\
"\t--batch-xsl, -s      multiple input files, single stylesheet\n"\
"\t--base=NAME, -b      set the hard base URI to NAME\n"\
"\t--debug-options      display information on debugging options\n"\
"\t--help, -?           display this help message\n"\
"\t--log-file=NAME, -L  set the log file, turn logging on\n"\
"\t--measure, -m        measure the processing time\n"\
"\t--version, -v        display version information\n"\
"Defaults:\n\t<input> = stdin, <output> = stdout\n"\
"\tlogging off, no hard base URI\n"\
"Notes:\n"\
"\t- assignments define named buffers (name=value)\n"\
"\t  and top-level params ($name=value).\n"\
"\t- to specify value in an option, use -b=NAME or -b NAME\n"\
"\t  (correspondingly for the equivalent long options)";

char askhelp[] = "Type sabcmd --help to display a help message.\n";
char msgConflict[] = "conflict with preceding switches: ";
char version_txt[] = "\nsabcmd "SAB_VERSION" ("SAB_DATE")\n"\
    "copyright (C) 2000 - 2005 Ginger Alliance (www.gingerall.com)\n";
char dbg_usage[] = "\nDebugging options:\n"\
    "\t--debug\t\t\tdisplay results of the command line parse\n"\
    "\t--debugger\t\trun the xslt debugger\n"\
    "\t--times=COUNT, -t\trun sabcmd the specified number of times\n"\
    "\t--flags=FLAGS, -f\tset processor flags for the processing\n"\
    "\t--use-SPF, -F\t\tuse SablotProcessFiles().\n"\
    "\t--use-SPS, -S\t\tuse SablotProcessStrings(). Give 2 args\n"\
    "\t\t\t\t(stylesheet, input). Precede each by @.\n" \
    "\t--use-SPS-on-files\tuse SablotProcessStrings() on the contents\n"\
    "\t\t\t\tof the given files.\n";
/* removed the GPL notice: 
char startup_licence[]="sabcmd and Sablotron come with ABSOLUTELY NO WARRANTY. "\
    "They are free software,\n"\
    "and you are welcome to redistribute them under certain conditions.\n"\
    "For details on warranty and the redistribution terms, see the README file.\n";
*/
char startup_licence[] = "The Sablotron XSLT Processor comes with NO WARRANTY.\n"\
    "It is subject to the Mozilla Public License Version 1.1.\n"\
    "Alternatively, you may use Sablotron under the GNU General Public License.\n";

/*****************************************************************
handy functions
*****************************************************************/

void saberr(const char *msg1, const char *msg2)
{
    fprintf(stderr, "Error: %s", msg1);
    if (msg2) fputs(msg2, stderr);
    fprintf(stderr, "\n%s", askhelp);
    exit(EXIT_FAILURE);
}

void saberrn(const char *msg, int num)
{
    char buf[10];
    sprintf(buf,"%d",num);
    saberr(msg, buf);
}

int chrpos(const char *text, char c)
{
    const char *p = strchr(text, c);
    return p ? (int)(p - text) : -1;
}

void freefirst(char **array)
{
    for (; *array; array += 2)
        free(*array);
}

void copyAssignment(char **array, int index, char *text, int split)
{
    char *p;
    if (index >= MAX_ARG_OR_PAR)
      saberr("too many assignments", NULL);
    array[2 * index + 1] = text + split + 1;
    p = array[2 * index] = (char*) malloc(split + 1);
    memcpy(p, text, split);
    p[split] = 0;
}

/*****************************************************************
    switch support
*****************************************************************/

// globals to hold the names and assignments

char* arrayBare[MAX_BARE];
char* arrayEq[MAX_EQ]; 
int indexBare = 0, indexEq = 0;

/*****************************************************************
findSwitch
*****************************************************************/

int max_strncmp(const char *s1, const char *s2, int len1)
{
    int len2 = strlen(s2);
    return strncmp(s1, s2, len1 >= len2 ? len1 : len2);
}

int findSwitch(char *text)
{
    int islong = (text[1] == '-');
    char *stripped = text + (islong ? 2 : 1);
    int eqpos = chrpos(stripped, '=');
    int index = 0;
    while (switches[index].id != ID_NONE)
    {
        if ((eqpos >= 0 && islong && !max_strncmp(stripped, switches[index].longName, eqpos)) ||
            (eqpos < 0 && islong && !strcmp(stripped, switches[index].longName)) ||
            (stripped[0] == switches[index].shortName && ! islong && (eqpos == 1 || !stripped[1])))
            return index + 1;
        index++;
    }
    return 0;
}

/*****************************************************************
applySwitch
ord is the 1-based index of the switch!
*****************************************************************/

#define cassign(WHERE, WHAT, TEXT) \
{ if (WHERE) saberr(msgConflict, (TEXT)); else WHERE = (WHAT); }


int applySwitch(int ord, char *text, char *following)
{
    char *value = NULL;
    char type = switches[--ord].type;       // make ord 0-based

    int skip = 0;
    int eqpos = chrpos(text, '=');
    if (eqpos >= 0)
        value = text + eqpos + 1;
    else
    {
        if ((type == 'S' || type == 'N') && following && *following != '-')
        {
            value = following;
            skip = 1;
        }
    }
    switch (switches[ord].type)
    {
    case 'S': 
        {
            if (!value) saberr("switch needs a string value: ", text);
            cassign(*(char**)(switches[ord].destination), value, text);
        }; break;
    case 'N':
        {
            char *stopper;
            long number = 0;
            if (value)
                number = strtol(value, &stopper, 0);
            if (!value || *stopper)
                saberr("switch needs a number value: ", text);
            cassign(*(int*)(switches[ord].destination), (int) number, text);
        }; break;
    case 'O':
        {
            if (value)
                saberr("value not recognized in switch: ", text);
            cassign(*(int*)(switches[ord].destination), switches[ord].id, text);
        }; break;
    default:
        saberr("error processing the switches", NULL);
    }
    return skip;
}

/*****************************************************************
readSwitches
*****************************************************************/

void readSwitches(int argc, char** argv)
{
    int i, ord;
    for (i = 1; i < argc; i++)
    {
        if (argv[i][0] == '-')
        {
            if (!(ord = findSwitch(argv[i])))
                saberr("invalid switch ", argv[i]);
            else
                i += applySwitch(ord, argv[i], 
                    (i < argc-1) ? argv[i+1] : NULL);
                continue;
        }
        else
        {
            if (((chrpos(argv[i], '=')) != -1) && (argv[i][0] != BARE_SIGN))
            {
                if (indexEq >= MAX_EQ)
                    saberrn("too many assignments, allowing ", MAX_EQ);
                else
                    arrayEq[indexEq++] = argv[i];
            }
            else
            {
                if (indexEq >= MAX_BARE)
                    saberrn("too many names, allowing ", MAX_BARE);
                else
                    arrayBare[indexBare++] = argv[i] + (argv[i][0] == BARE_SIGN);
            }
        }
    }
};

/*****************************************************************
xformToPairs
*****************************************************************/

void xformToPairs(char** array, int count,
    char** _params, int* _paramsC,
    char** _args, int* _argsC)
{
    int i, pos;
    *_paramsC = *_argsC = 0;
    for (i = 0 ; i < count; i++)
    {
        pos = chrpos(array[i], '=');
        if (array[i][0] == '$')
            copyAssignment(_params, (*_paramsC)++, array[i] + 1, pos-1);
        else
            copyAssignment(_args, (*_argsC)++, array[i], pos);
    };
    _args[2 * *_argsC] = NULL;
    _params[2 * *_paramsC] = NULL;
}

/*****************************************************************
debug()
for debugging sabcmd itself
*****************************************************************/

#define safe(PTR) (PTR? PTR : "[null]")

void dumparray(char* caption, char** array, int count, int asPairs)
{
    char **p = array;
    printf("%s (%d)\n",caption, count);
    while(*p)
    {
        printf("\t%s",*p);
        if (asPairs)
        {
            printf(" = %s\n",safe(p[1]));
            p += 2;
        }
        else 
        {
            puts("");
            p++;
        }
    }
}

void debug(int cParams, char **params, int cArgs, char **args)
{
    puts("\nCommand line parse results:");
    dumparray("Names", arrayBare, indexBare, 0);
    dumparray("Parameters", params, cParams, 1);
    dumparray("Named buffers", args, cArgs, 1);   
    printf("Settings \n\tTimes\t\t%d \n\tMode\t\t%d\n",
        numberTimes, numberMode);
    printf("\tBase\t\t%s \n\tLog\t\t%s\n", safe(stringBase), safe(stringLog));
}

double getExactTime()
{
    double ret;
#if defined (WIN32)
    struct _timeb theTime;
    _ftime(&theTime);
    ret = theTime.time + theTime.millitm/1000.0;
#elif defined (HAVE_FTIME)
    struct timeb theTime;
    ftime(&theTime);
    ret = theTime.time + theTime.millitm/1000.0;
#elif defined (HAVE_GETTIMEOFDAY)
    timeval theTime;
    gettimeofday(&theTime, NULL);
    ret = theTime.tv_sec + theTime.tv_usec/1000000.0;
#else
#error "Can't find function ftime() or similar"
#endif
    return ret;
}


/*****************************************************************
main
*****************************************************************/

void full_version_txt()
{
    puts(version_txt);
    puts(startup_licence);
}


char* readToBuffer(char *filename)
{
    long length;
    int count;
    char *buf;
    FILE *f = fopen(filename, "rt");
    if (!f)
        saberr("cannot open file ", filename);
    length = 0xffff;
    buf = (char*) malloc(length + 1);
    count = fread(buf, 1, length + 1, f);
    fclose(f);
    if (count > length)
        saberr("file too large (64k limit)", NULL);
    buf[count] = 0;
    return buf;
}


int runSingleXSLT(const char **arrayParams, const char **arrayArgs, 
		  char **resultArg)
{
   void *theProcessor;
   char *resultURI = NULL;
   int ecode;
   void *situa = NULL;
   
   if (ecode = SablotCreateSituation(&situa))
     return ecode;

   if (commandFlags)
     SablotSetOptions(situa, commandFlags);

   if (ecode = SablotCreateProcessorForSituation(situa, &theProcessor))
      return ecode;

   if (stringLog)
      SablotSetLog(theProcessor, stringLog, 0);

   if (stringBase)
      SablotSetBase(theProcessor, stringBase);

   if (indexBare <= 2)
      resultURI = (char*)"file:///__stdout";
   else
      resultURI = arrayBare[2];

   ecode = SablotRunProcessor(theProcessor,
	arrayBare[0],
	indexBare <= 1  ? (char*)"file:///__stdin" : arrayBare[1],
	resultURI,
	arrayParams, arrayArgs);

   if (ecode == 0)
      SablotGetResultArg(theProcessor, resultURI, resultArg);
                
   if (theProcessor)
      SablotDestroyProcessor(theProcessor);

   if (situa)
     SablotDestroySituation(situa);

   return ecode;
}

int runBatchXSLT(const char **arrayParams, const char **arrayArgs, 
		 char **resultArg)
{
   void *theProcessor = NULL, *sabSit = NULL, *preparsed = NULL;
   char *resultURI = (char*)"file:///__stdout";
   char *input = NULL;
   int ecode;
   int argcount = 2, firstarg = 1;

   if ((batchMode != ID_BATCH_XML) &&
       (batchMode != ID_BATCH_XSL) &&
       (batchMode != ID_CHAIN_XSL))
      return runSingleXSLT(arrayParams, arrayArgs, resultArg);

   ecode = SablotCreateSituation(&sabSit);
   if (ecode != 0)
      return ecode;

   if (commandFlags)
     SablotSetOptions(sabSit, commandFlags);

   ecode = SablotCreateProcessorForSituation(sabSit, &theProcessor);

   if (ecode == 0)
   {
      if (stringLog)
	 SablotSetLog(theProcessor, stringLog, 0);

      if (stringBase)
	 SablotSetBase(theProcessor, stringBase);

      switch (batchMode)
      {
	 case ID_BATCH_XSL:
	    ecode = SablotParseStylesheet(sabSit, arrayBare[0], &preparsed);
	    break;
	 case ID_BATCH_XML:
	    ecode = SablotParse(sabSit, arrayBare[0], &preparsed);
	    break;
	 case ID_CHAIN_XSL:
	    input = arrayBare[0];
	    argcount = 1;
	    firstarg = 2;
	    break;
      }
   }

   for (int t = firstarg; (t <= indexBare-argcount) && (ecode == 0); t += argcount)
      {
	 if (arrayParams)
	 {
	    const char **params = arrayParams;
	    while ((params[0] != NULL) && (ecode == 0))
	    {
	       ecode = SablotAddParam(sabSit, theProcessor, params[0], params[1]);
	       params += 2;
	    }
	 }

	 if (arrayArgs)
	 {
	    const char **args = arrayArgs;
	    while ((args[0] != NULL) && (ecode == 0))
	    {
	       ecode = SablotAddArgBuffer(sabSit, theProcessor, args[0], args[1]);
	       args += 2;
	    }
	 }

	 if (ecode == 0)
	 {
	 switch (batchMode)
	    {
	    case ID_BATCH_XSL:
	    {
	       const char *document = arrayBare[t];
	       const char *output = arrayBare[t+1];
	       SablotAddArgTree(sabSit, theProcessor, "stylesheet", preparsed);
	       ecode = SablotRunProcessorGen(sabSit, theProcessor, "arg:stylesheet", document, output);
	    } break;
	    case ID_BATCH_XML:
	    {
	       const char *stylesheet = arrayBare[t];
	       const char *output = arrayBare[t+1];
	       SablotAddArgTree(sabSit, theProcessor, "source", preparsed);
	       ecode = SablotRunProcessorGen(sabSit, theProcessor, stylesheet, "arg:source", output);
	    } break;
	    case ID_CHAIN_XSL:
	    {
	       const char *stylesheet = arrayBare[t];
	       char *output;
	       if (t < indexBare-argcount)
		  output = (char*)"arg:output";
	       else
		  output = arrayBare[1];

	       ecode = SablotRunProcessorGen(sabSit, theProcessor, stylesheet, input, output);

	    if (ecode == 0)
	       {
		  if (input == arrayBare[0])
		     input = "arg:buffer";
		  else
		     SablotFree(*resultArg);

		  SablotGetResultArg(theProcessor, output, resultArg);
		  SablotAddArgBuffer(sabSit, theProcessor, "buffer", *resultArg);
	       }
	    } break;
	 }
      }
   }

   if ((ecode == 0) && (batchMode != ID_CHAIN_XSL))
      SablotGetResultArg(theProcessor, resultURI, resultArg);

   if (preparsed)
      SablotDestroyDocument(sabSit, preparsed);

   if (theProcessor)
      SablotDestroyProcessor(theProcessor);

   if (sabSit)
      SablotDestroySituation(sabSit);

   return ecode;
}

void runDebugger()
{
#ifdef SABLOT_DEBUGGER
  debuggerInit();
  debuggerEnterIdle();
  debuggerDone();
#else
  printf("Debugegr is not supported in this build\n");
#endif
}

int main(int argc, char *argv[])
{
    int indexParams, indexArgs, ecode, i;
    char *arrayParams[2 * MAX_ARG_OR_PAR + 2],
      *arrayArgs[2 * MAX_ARG_OR_PAR + 2];
    char *resultArg = NULL;
    double timeZero = 0;

    // the following set the globals arrayBare and arrayEq
    // we get the number of words/assignments in indexBare/indexEq
    
    readSwitches(argc, argv);
    
    switch(numberMode)
      {
      case ID_HELP:
        {
	  puts(usage); return 0;
        }; break;
      case ID_DBG_HELP:
        {
	  puts(dbg_usage); return 0;
        }; break;
      case ID_VERSION:
        {
	  full_version_txt(); return 0;
        }; break;
      case ID_DEBUGGER:
	{
	  runDebugger(); return 0;
	}; break;
      }

    if (!indexBare && numberMode != ID_DEBUG)
    {
        full_version_txt();
        saberr("stylesheet not given", NULL);
    }

    xformToPairs(arrayEq, indexEq, 
        arrayParams, &indexParams,
        arrayArgs, &indexArgs);

    if (numberMode == ID_DEBUG)
    {   
        debug(indexParams, arrayParams, indexArgs, arrayArgs); 
        return 0;
    };

    if (!numberTimes)
        numberTimes = 1;
    if (numberMeasure)
        timeZero = getExactTime();

    for (i = 0; i < numberTimes; i++)
    {
        switch(numberMode)
        {
        case 0: 
	  {
	     if ((batchMode == ID_BATCH_XML) || (batchMode == ID_BATCH_XSL) || (batchMode == ID_CHAIN_XSL))
	      ecode = runBatchXSLT((const char**)arrayParams, 
		(const char**)arrayArgs, &resultArg);
	    else
	      ecode = runSingleXSLT((const char**)arrayParams, 
				    (const char**)arrayArgs, &resultArg);
	  }; break;
        case ID_SPS:
            {
                if (indexBare != 2)
                    saberr("SablotProcessStrings() needs exactly two arguments", NULL);
                ecode = SablotProcessStrings(arrayBare[0], arrayBare[1], &resultArg);
            }; break;
        case ID_SPS_ON_FILES:
            {
                char *sheetBuf, *inputBuf;
                if (indexBare != 2)
                    saberr("need exactly two arguments", NULL);
                sheetBuf = readToBuffer(arrayBare[0]);
                inputBuf = readToBuffer(arrayBare[1]);
                ecode = SablotProcessStrings(sheetBuf, inputBuf, &resultArg);
                free(sheetBuf);
                free(inputBuf);
            }; break;
        case ID_SPF: 
            ecode = SablotProcessFiles(arrayBare[0],
                indexBare <= 1  ? (char*)"file:///__stdin" : arrayBare[1],
                indexBare <= 2  ? (char*)"file:///__stdout" : arrayBare[2]);
            break;
        }
        
        if (ecode)
            return ecode;
        
        if (resultArg)
        {
            fprintf(stderr, "output buffer sent to stdout\n");
            puts(resultArg);
            SablotFree(resultArg);
            resultArg = NULL;
        }
    };

    if (numberMeasure)
        fprintf(stderr, "Total time: %4.3f seconds\n", getExactTime() - timeZero);
        
    freefirst(arrayArgs);
    freefirst(arrayParams);

    return 0;
}

