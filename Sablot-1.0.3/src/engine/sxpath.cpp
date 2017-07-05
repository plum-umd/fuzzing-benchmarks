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

#define SablotAsExport
#include "sxpath.h"
#include "situa.h"
#include "domprovider.h"
#include "expr.h"
#include "context.h"
#include "guard.h"

#define SIT(S) (*(Situation*)S)
#define QC(Q) (*(QueryContextClass*)Q)

//
//    SablotRegDOMHandler
//

void SXP_registerDOMHandler(SablotSituation S, DOMHandler *domh, void* udata)
{
    SIT(S).setDOMProvider(domh, udata);
}

void SXP_unregisterDOMHandler(SablotSituation S)
{
    SIT(S).setDOMProvider(NULL, NULL);
}

/*
 *
 *    QueryContext functions
 *
 */

void SXP_setOptions(SablotSituation S, unsigned long options)
{
  SIT(S).setSXPOptions(options);
}

unsigned long SXP_getOptions(SablotSituation S)
{
  return SIT(S).getSXPOptions();
}

void SXP_setMaskBit(SablotSituation S, int mask)
{
  SIT(S).setSXPMaskBit(mask);
}

int SXP_createQueryContext(SablotSituation S, QueryContext *Q)
{
    *Q = new QueryContextClass(SIT(S));
    return OK;
}

int SXP_addVariableNumber(QueryContext Q, 
        const SXP_char* name, double value)
{
    GP( Expression ) e = QC(Q).getNewExpr();
    (*e).setAtom(Number(value)); 
    if (! QC(Q).addVariableExpr(name, e))
	e.keep();
    return QC(Q).getError();
}

int SXP_addVariableString(QueryContext Q, 
        const SXP_char* name, const SXP_char* value)
{
    GP( Expression ) e = QC(Q).getNewExpr();
    (*e).setAtom(Str((char*)value)); 
    if (! QC(Q).addVariableExpr(name, e))
	e.keep();
    return QC(Q).getError();
}

int SXP_addVariableBoolean(QueryContext Q, 
        const SXP_char* name, int value)
{
    GP( Expression ) e = QC(Q).getNewExpr();
    (*e).setAtom(value ? TRUE : FALSE); 
    if (! QC(Q).addVariableExpr(name, e))
	e.keep();
    return QC(Q).getError();
}

int SXP_addVariableBinding(QueryContext Q, 
    const SXP_char* name, QueryContext source)
{
    QC(Q).addVariableBinding(name, QC(source));
    return QC(Q).getError();
}
    
int SXP_addNamespaceDeclaration(QueryContext Q, 
    const SXP_char* prefix, const SXP_char* uri)
{
    QC(Q).addNamespaceDeclaration(prefix, uri);
    return QC(Q).getError();
}
    
int SXP_query(QueryContext Q, const SXP_char* queryText,
    SXP_Node n, int contextPosition, int contextSize)
{
    QC(Q).query(queryText, n, contextPosition, contextSize);
    return QC(Q).getError();
}

int SXP_destroyQueryContext(QueryContext Q)
{
    delete (QueryContextClass*)Q;
    return OK;
}

/*
 *
 *    Functions to retrieve the query result and its type
 *
 */

int SXP_getResultType(QueryContext Q, SXP_ExpressionType *type)
{
    *type = QC(Q).getType();
    return OK;
}

int SXP_getResultNumber(QueryContext Q, double *result)
{
    *result = (double)(*(QC(Q).getNumber()));
    return OK;
}

int SXP_getResultBool(QueryContext Q, int *result)
{
    *result = QC(Q).getBoolean();
    return OK;
}

int SXP_getResultString(QueryContext Q, const char **result)
{
    *result = (char*)(*(QC(Q).getString()));
    return OK;
}

int SXP_getResultNodeset(QueryContext Q, SXP_NodeList *result)
{
    *result = (SXP_NodeList)(QC(Q).getNodeset());
    return OK;
}

/*
 *
 *    NodeList manipulation
 *
 */

int SXP_getNodeListLength(SXP_NodeList l)
{
    return ((Context*)l) -> getSize();
}

SXP_Node SXP_getNodeListItem(QueryContext Q, SXP_NodeList l, int index)
{
  int count = SXP_getNodeListLength(l);
  if (index < 0 || index >= count)
    return NULL;
  else
    {
      NodeHandle n = (*((Context*)l))[index];
      int bit = QC(Q).getSituation().getSXPMaskBit();
      return SXP_UNMASK_LEVEL(n, bit);
    }
}


