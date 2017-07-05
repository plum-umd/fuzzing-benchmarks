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

#ifndef SXPathHIncl
#define SXPathHIncl

/* basic types needed in sablot.h */
typedef void *SXP_Node;
typedef void *SXP_Document;
typedef void *SXP_NodeList;
typedef SXP_Node NodeHandle;

#include "sablot.h"

typedef enum
{
    ELEMENT_NODE = 1,
    ATTRIBUTE_NODE = 2,
    TEXT_NODE = 3,
    PROCESSING_INSTRUCTION_NODE = 7,
    COMMENT_NODE = 8,
    DOCUMENT_NODE = 9,
    NAMESPACE_NODE = 13
} SXP_NodeType;

typedef enum
{
    SXP_NONE,
    SXP_NUMBER,
    SXP_STRING,
    SXP_BOOLEAN,
    SXP_NODESET
} SXP_ExpressionType;

typedef char SXP_char;

typedef void *QueryContext;

/*option constants */
typedef enum 
{
  SXPF_DISPOSE_NAMES = 0x1,
  SXPF_DISPOSE_VALUES = 0x2,
  SXPF_SUPPORTS_UNPARSED_ENTITIES =0x4
} SXPFlags;

/*
 *    DOM handler functions
 *    This handler is registered with the Situation rather than the Processor
 */
 
/*****************************************************************
DOMHandler

  Handler providing information about a DOM tree to the XPath 
  processor
*****************************************************************/

typedef SXP_NodeType     DOMH_getNodeType(SXP_Node n);
typedef SXP_NodeType     DOMH_getNodeTypeExt(SXP_Node n, void *userData);

typedef const SXP_char*  DOMH_getNodeName(SXP_Node n);
typedef const SXP_char*  DOMH_getNodeNameExt(SXP_Node n, void *userData);

typedef const SXP_char*  DOMH_getNodeNameURI(SXP_Node n);
typedef const SXP_char*  DOMH_getNodeNameURIExt(SXP_Node n, void *userData);

typedef const SXP_char*  DOMH_getNodeNameLocal(SXP_Node n);
typedef const SXP_char*  DOMH_getNodeNameLocalExt(SXP_Node n, void *userData);

typedef const SXP_char*  DOMH_getNodeValue(SXP_Node n);
typedef const SXP_char*  DOMH_getNodeValueExt(SXP_Node n, void *userData);

typedef SXP_Node         DOMH_getNextSibling(SXP_Node n);
typedef SXP_Node         DOMH_getNextSiblingExt(SXP_Node n, void *userData);

typedef SXP_Node         DOMH_getPreviousSibling(SXP_Node n);
typedef SXP_Node         DOMH_getPreviousSiblingExt(SXP_Node n, void *userData);

typedef SXP_Node         DOMH_getNextAttrNS(SXP_Node n);
typedef SXP_Node         DOMH_getNextAttrNSExt(SXP_Node n, void *userData);

typedef SXP_Node         DOMH_getPreviousAttrNS(SXP_Node n);
typedef SXP_Node         DOMH_getPreviousAttrNSExt(SXP_Node n, void *userData);

typedef int              DOMH_getChildCount(SXP_Node n);
typedef int              DOMH_getChildCountExt(SXP_Node n, void *userData);

typedef int              DOMH_getAttributeCount(SXP_Node n);
typedef int              DOMH_getAttributeCountExt(SXP_Node n, void *userData);

typedef int              DOMH_getNamespaceCount(SXP_Node n);
typedef int              DOMH_getNamespaceCountExt(SXP_Node n, void *userData);

typedef SXP_Node         DOMH_getChildNo(SXP_Node n, int ndx);
typedef SXP_Node         DOMH_getChildNoExt(SXP_Node n, int ndx, void *userData);

typedef SXP_Node         DOMH_getAttributeNo(SXP_Node n, int ndx);
typedef SXP_Node         DOMH_getAttributeNoExt(SXP_Node n, int ndx, void *userData);

typedef SXP_Node         DOMH_getNamespaceNo(SXP_Node n, int ndx);
typedef SXP_Node         DOMH_getNamespaceNoExt(SXP_Node n, int ndx, void *userData);

typedef SXP_Node         DOMH_getParent(SXP_Node n);
typedef SXP_Node         DOMH_getParentExt(SXP_Node n, void *userData);

typedef SXP_Document     DOMH_getOwnerDocument(SXP_Node n);
typedef SXP_Document     DOMH_getOwnerDocumentExt(SXP_Node n, void *userData);

typedef int              DOMH_compareNodes(SXP_Node n1, SXP_Node n2);
typedef int              DOMH_compareNodesExt(SXP_Node n1, SXP_Node n2, void *userData);

typedef SXP_Document     DOMH_retrieveDocument(const SXP_char* uri, 
					       void* udata);
typedef SXP_Document     DOMH_retrieveDocumentExt(const SXP_char* uri, 
						 const SXP_char* baseUri, 
					         void* udata);

typedef SXP_Node         DOMH_getNodeWithID(SXP_Document doc, const SXP_char* id);
typedef SXP_Node         DOMH_getNodeWithIDExt(SXP_Document doc, const SXP_char* id, void *userData);

typedef void             DOMH_freeBuffer(SXP_char *buff);
typedef void             DOMH_freeBufferExt(SXP_Node n, SXP_char *buff, void *userData);


typedef struct
{
    DOMH_getNodeType         *getNodeType;
    DOMH_getNodeName         *getNodeName;
    DOMH_getNodeNameURI      *getNodeNameURI;
    DOMH_getNodeNameLocal    *getNodeNameLocal;
    DOMH_getNodeValue        *getNodeValue;
    DOMH_getNextSibling      *getNextSibling;
    DOMH_getPreviousSibling  *getPreviousSibling;
    DOMH_getNextAttrNS       *getNextAttrNS;
    DOMH_getPreviousAttrNS   *getPreviousAttrNS;
    DOMH_getChildCount       *getChildCount;
    DOMH_getAttributeCount   *getAttributeCount;
    DOMH_getNamespaceCount   *getNamespaceCount;
    DOMH_getChildNo          *getChildNo;
    DOMH_getAttributeNo      *getAttributeNo;
    DOMH_getNamespaceNo      *getNamespaceNo;
    DOMH_getParent           *getParent;
    DOMH_getOwnerDocument    *getOwnerDocument;
    DOMH_compareNodes        *compareNodes;
    DOMH_retrieveDocument    *retrieveDocument;
    DOMH_getNodeWithID       *getNodeWithID;
  /*optional entries - driven by sxpOptions */
    DOMH_freeBuffer          *freeBuffer;

    DOMH_getNodeTypeExt         *getNodeTypeExt;
    DOMH_getNodeNameExt         *getNodeNameExt;
    DOMH_getNodeNameURIExt      *getNodeNameURIExt;
    DOMH_getNodeNameLocalExt    *getNodeNameLocalExt;
    DOMH_getNodeValueExt        *getNodeValueExt;
    DOMH_getNextSiblingExt      *getNextSiblingExt;
    DOMH_getPreviousSiblingExt  *getPreviousSiblingExt;
    DOMH_getNextAttrNSExt       *getNextAttrNSExt;
    DOMH_getPreviousAttrNSExt   *getPreviousAttrNSExt;
    DOMH_getChildCountExt       *getChildCountExt;
    DOMH_getAttributeCountExt   *getAttributeCountExt;
    DOMH_getNamespaceCountExt   *getNamespaceCountExt;
    DOMH_getChildNoExt          *getChildNoExt;
    DOMH_getAttributeNoExt      *getAttributeNoExt;
    DOMH_getNamespaceNoExt      *getNamespaceNoExt;
    DOMH_getParentExt           *getParentExt;
    DOMH_getOwnerDocumentExt    *getOwnerDocumentExt;
    DOMH_compareNodesExt        *compareNodesExt;
    DOMH_retrieveDocumentExt    *retrieveDocumentExt;
    DOMH_getNodeWithIDExt       *getNodeWithIDExt;
  /*optional entries - driven by sxpOptions */
    DOMH_freeBufferExt          *freeBufferExt;

} DOMHandler; 

Declare
(
    void SXP_registerDOMHandler(SablotSituation S, 
				DOMHandler *domh, void* udata);
)

Declare
(
    void SXP_unregisterDOMHandler(SablotSituation S);
)

/*
 *
 *    QueryContext functions
 *
 */

/*  options setter getter */
Declare
(
    void SXP_setOptions(SablotSituation S, unsigned long options);
)

Declare
(
    void SXP_setMaskBit(SablotSituation S, int mask);
)

Declare
(
    unsigned long SXP_getOptions(SablotSituation S);
)

Declare
(
    int SXP_createQueryContext(SablotSituation S, QueryContext *Q);
)

Declare
(
    int SXP_addVariableBinding(QueryContext Q, 
        const SXP_char* name, QueryContext source);
)
    
Declare
(
    int SXP_addVariableNumber(QueryContext Q, 
        const SXP_char* name, double value);
)

Declare
(
    int SXP_addVariableString(QueryContext Q, 
        const SXP_char* name, const SXP_char* value);
)

Declare
(
    int SXP_addVariableBoolean(QueryContext Q, 
        const SXP_char* name, int value);
)
    
Declare
(
    int SXP_addNamespaceDeclaration(QueryContext Q, 
        const SXP_char* prefix, const SXP_char* uri);
)
    
Declare
(
    int SXP_query(QueryContext Q, const SXP_char* query,
		  SXP_Node n, int contextPosition, int contextSize);
)

Declare
(
    int SXP_destroyQueryContext(QueryContext Q);
)

/*
 *
 *    Functions to retrieve the query result and its type
 *
 */

Declare
(
    int SXP_getResultType(QueryContext Q, SXP_ExpressionType *type);
)

Declare
(
    int SXP_getResultNumber(QueryContext Q, double *result);
)

Declare
(
    int SXP_getResultBool(QueryContext Q, int *result);
)

Declare
(
    int SXP_getResultString(QueryContext Q, const char **result);
)

Declare
(
    int SXP_getResultNodeset(QueryContext Q, SXP_NodeList *result);
)

/*
 *
 *    NodeList manipulation
 *
 */

Declare
(
    int SXP_getNodeListLength(SXP_NodeList l);
)

Declare
(
    SXP_Node SXP_getNodeListItem(QueryContext Q, SXP_NodeList l, int index);
)


#endif /* SXPathHIncl */
