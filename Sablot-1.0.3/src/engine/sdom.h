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

/* sdom.h */

#ifndef SDomHIncl
#define SDomHIncl

//used for DISABLE_DOM only anything else is DANGEROUS
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifndef DISABLE_DOM

#include "sablot.h"

/**
 **
 **  types
 **
 **/

typedef void* SDOM_Node;
typedef void* SDOM_NodeList;
/* typedef void* SDOM_Document; */

typedef enum
{
  SDOM_ELEMENT_NODE = 1,
  SDOM_ATTRIBUTE_NODE = 2,
  SDOM_TEXT_NODE = 3,
  SDOM_CDATA_SECTION_NODE = 4,
  SDOM_ENTITY_REFERENCE_NODE = 5,
  SDOM_ENTITY_NODE = 6,
  SDOM_PROCESSING_INSTRUCTION_NODE = 7,
  SDOM_COMMENT_NODE = 8,
  SDOM_DOCUMENT_NODE = 9,
  SDOM_DOCUMENT_TYPE_NODE = 10,
  SDOM_DOCUMENT_FRAGMENT_NODE = 11,
  SDOM_NOTATION_NODE = 12,
  SDOM_OTHER_NODE    /* not in spec */
} 
SDOM_NodeType;

/*
 *  we define DOM_char as char, although the spec says strings should be
 *  UTF_16. Needs check.
 */
typedef char SDOM_char;


/*
 *  DomException
 *  will be an enum of all the values given in the spec.
 */
 
/*
  INDEX_SIZE_ERR, 1
  STRING_SIZE_ERR, 2
  HIERARCHY_REQUEST_ERR, 3
  WRONG_DOCUMENT_ERR, 4
  INVALID_CHARACTER_ERR, 5
  NO_DATA_ALLOWED_ERR, 6
  NO_MODIFICATION_ALLOWED_ERR, 7
  NOT_FOUND_ERR, 8
  NOT_SUPPORTED_ERR, 9
  INUSE_ATTRIBUTE_ERR, 10
  INVALID_STATE_ERR, 11
  SYNTAX_ERR, 12
  INVALID_MODIFICATION_ERR, 13
  NAMESPACE_ERR, 14
  INVALID_ACCESS_ERR, 15
*/

typedef enum 
{
    SDOM_OK,
    SDOM_INDEX_SIZE_ERR = 1,
    SDOM_DOMSTRING_SIZE_ERR = 2,
    SDOM_HIERARCHY_REQUEST_ERR = 3,
    SDOM_WRONG_DOCUMENT_ERR = 4,
    SDOM_INVALID_CHARACTER_ERR = 5,
    SDOM_NO_DATA_ALLOWED_ERR = 6,
    SDOM_NO_MODIFICATION_ALLOWED_ERR = 7,
    SDOM_NOT_FOUND_ERR = 8,
    SDOM_NOT_SUPPORTED_ERR = 9,
    SDOM_INUSE_ATTRIBUTE_ERR = 10,
    SDOM_INVALID_STATE_ERR = 11,
    SDOM_SYNTAX_ERR = 12,
    SDOM_INVALID_MODIFICATION_ERR = 13,
    SDOM_NAMESPACE_ERR = 14,
    SDOM_INVALID_ACCESS_ERR = 15,
    /* not in spec below this point: */
    SDOM_INVALID_NODE_TYPE, /* eg passing a non-element for an element */
    SDOM_QUERY_PARSE_ERR,
    SDOM_QUERY_EXECUTION_ERR,
    SDOM_NOT_OK
} SDOM_Exception;

/**
 **
 **  Node
 **  n is always the node the function is to operate on (kind of 'this')
 **
 **/

/*
    createElement
    Creates a new element Node with NULL parent and specified owner, 
    and returns it in *pn.
    Raises:
 */
Declare(
SDOM_Exception SDOM_createElement(
				  SablotSituation s, 
				  SDOM_Document d, 
				  SDOM_Node *pn, 
				  const SDOM_char *tagName);
)	

//_JP_ v
/*
    createElementNS
    Creates a new element Node with NULL parent and specified owner, 
    and returns it in *pn.
    Raises:
    SDOM_NAMESPACE_ERR or SDOM_INVALID_CHARACTER_ERR when qName or uri malformed
 */
Declare(
SDOM_Exception SDOM_createElementNS(
				    SablotSituation s, 
				    SDOM_Document d, 
				    SDOM_Node *pn, 
				    const SDOM_char *uri,
				    const SDOM_char *qName);
)	
//_JP_ ^

/*
    createAttribute
    Creates a new attribute Node and returns it in *pn.
    Raises:
 */

Declare
(
SDOM_Exception SDOM_createAttribute(
    SablotSituation s, 
    SDOM_Document d, 
    SDOM_Node *pn, 
    const SDOM_char *name);
)

//_JP_ v
/*
    createAttributeNS
    Creates a new attribute Node and returns it in *pn.
    Raises:
    SDOM_NAMESPACE_ERR or SDOM_INVALID_CHARACTER_ERR when qName or uri malformed
 */
Declare(
SDOM_Exception SDOM_createAttributeNS(
    SablotSituation s, 
    SDOM_Document d, 
    SDOM_Node *pn, 
    const SDOM_char *uri,
    const SDOM_char *qName);
)	
//_JP_ ^


/*
    createTextNode
    Raises:
 */

Declare
(
SDOM_Exception SDOM_createTextNode(
    SablotSituation s, 
    SDOM_Document d, 
    SDOM_Node *pn, 
    const SDOM_char *data);
)

/*
    createAttribute
    Creates a new attribute Node and returns it in *pn.
    Raises:
 */

Declare
(
SDOM_Exception SDOM_createCDATASection(
    SablotSituation s, 
    SDOM_Document d, 
    SDOM_Node *pn,
    const SDOM_char *data);
)

/*
    createComment
    Raises:
 */

Declare
(
SDOM_Exception SDOM_createComment(
    SablotSituation s, 
    SDOM_Document d, 
    SDOM_Node *pn, 
    const SDOM_char *data);
)

/*
    createProcessingInstruction
    Raises:
 */

Declare
(
SDOM_Exception SDOM_createProcessingInstruction(
    SablotSituation s, 
    SDOM_Document d, 
    SDOM_Node *pn, 
    const SDOM_char *target,
    const SDOM_char *data);
)

/*
    disposeNode
    Frees all memory used by the node.
    Raises:
    !SPEC: Not in the spec.
 */

Declare
(
SDOM_Exception SDOM_disposeNode(
    SablotSituation s,
    SDOM_Node n);
)


/*
    getNodeType
    Returns the node type in *pType.
    Raises:
 */

Declare
(
SDOM_Exception SDOM_getNodeType(
    SablotSituation s,
    SDOM_Node n, 
    SDOM_NodeType *pType);
)


/*
    getNodeName
    Returns the pointer to the name of the node in *pName.
*/

Declare
(
SDOM_Exception SDOM_getNodeName(
    SablotSituation s, 
    SDOM_Node n, 
    SDOM_char **pName);
)

Declare
(
SDOM_Exception SDOM_getNodeNSUri(
    SablotSituation s, 
    SDOM_Node n, 
    SDOM_char **pName);
)

Declare
(
SDOM_Exception SDOM_getNodePrefix(
    SablotSituation s, 
    SDOM_Node n, 
    SDOM_char **pName);
)

Declare
(
SDOM_Exception SDOM_getNodeLocalName(
    SablotSituation s, 
    SDOM_Node n, 
    SDOM_char **pName);
)

/*
    setNodeName
    Sets the node name.
*/
Declare(
SDOM_Exception SDOM_setNodeName(
    SablotSituation s,
    SDOM_Node n, 
    const SDOM_char *name);
)

/*
    getNodeValue
    Returns in *pValue the string value of the node.
*/
Declare(
SDOM_Exception SDOM_getNodeValue(
    SablotSituation s,
    SDOM_Node n, 
    SDOM_char **pValue);
)

/*
    setNodeValue
    Sets the node value.
*/
Declare(
SDOM_Exception SDOM_setNodeValue(
    SablotSituation s,
    SDOM_Node n, 
    const SDOM_char *value);
)

/*
    getParentNode
    Returns the parent in *pParent.
*/
Declare(
SDOM_Exception SDOM_getParentNode(
    SablotSituation s,
    SDOM_Node n, 
    SDOM_Node *pParent);
)

/*
    getFirstChild
    Returns the first child in *pFirstChild.
*/
Declare(
SDOM_Exception SDOM_getFirstChild(
    SablotSituation s,
    SDOM_Node n, 
    SDOM_Node *pFirstChild);
)

/*
    getLastChild
    Returns the last child in *pLastChild.
*/
Declare(
SDOM_Exception SDOM_getLastChild(
    SablotSituation s,
    SDOM_Node n, 
    SDOM_Node *pLastChild);
)

/*
    getPreviousSibling
    Returns the previous sibling in *pPreviousSibling.
*/
Declare(
SDOM_Exception SDOM_getPreviousSibling(
    SablotSituation s,
    SDOM_Node n, 
    SDOM_Node *pPreviousSibling);
)

/*
    getNextSibling
    Returns the next sibling in *pNextSibling.
*/
Declare(
SDOM_Exception SDOM_getNextSibling(
    SablotSituation s,
    SDOM_Node n, 
    SDOM_Node *pNextSibling);
)

/*
    getChildNode
    Returns the child node in specified index or NULL.
*/
Declare(
SDOM_Exception SDOM_getChildNodeIndex(
    SablotSituation s,
    SDOM_Node n,
    int index,
    SDOM_Node *pChildNode);
)

/*
    getChildNodeCount
    Returns the count of child nodes.
*/
Declare(
SDOM_Exception SDOM_getChildNodeCount(
    SablotSituation s,
    SDOM_Node n,
    int *count);
)

/*
    getOwnerDocument
    Returns, in *pOwnerDocument, the Document owning the node.
    !SPEC: If the node is standalone, returns NULL.
*/
Declare(
SDOM_Exception SDOM_getOwnerDocument(
    SablotSituation s,
    SDOM_Node n, 
    SDOM_Document *pOwnerDocument);
)

/*
    insertBefore
    Inserts newChild as n's child, just before refChild.
*/
Declare(
SDOM_Exception SDOM_insertBefore(
    SablotSituation s, 
    SDOM_Node n, 
    SDOM_Node newChild, 
    SDOM_Node refChild);
)

/*
    removeChild
    Removes oldChild (a child of n) without deallocating it.
*/

Declare(
SDOM_Exception SDOM_removeChild(
    SablotSituation s, 
    SDOM_Node n, 
    SDOM_Node oldChild);
)

/*
    replaceChild
    Replaces oldChild (a child of n) by newChild.
*/
Declare(
SDOM_Exception SDOM_replaceChild(
    SablotSituation s,
    SDOM_Node n, 
    SDOM_Node newChild, 
    SDOM_Node oldChild);
)

/*
    appendChild
    Appends newChild as the last of n's children.
*/
Declare(
SDOM_Exception SDOM_appendChild(
    SablotSituation s,
    SDOM_Node n, 
    SDOM_Node newChild);
)

/*
    cloneNode
    Duplicates the node, returning the result in *clone.
    If deep is nonzero, the cloning process will be recursive.
*/

Declare(
SDOM_Exception SDOM_cloneNode(
    SablotSituation s,
    SDOM_Node n, 
    int deep, 
    SDOM_Node *clone);
)

/*
    cloneForeignNode
    Duplicates a node from a different doc, returning the result in *clone.
    If deep is nonzero, the cloning process will be recursive.
    As opposed to cloneNode, represents a Document method
*/

Declare(
SDOM_Exception SDOM_cloneForeignNode(
    SablotSituation s,
    SDOM_Document d,
    SDOM_Node n, 
    int deep, 
    SDOM_Node *clone);
)

/*
    getAttribute
    Returns, in *pValue, the contents of the attribute (of n) named 'name'.
*/
Declare(
SDOM_Exception SDOM_getAttribute(
    SablotSituation s,
    SDOM_Node n, 
    const SDOM_char *name, 
    SDOM_char **pValue);
)

/*
    getAttributeNS
    Returns, in *pValue, the contents of the attribute (of n) with 'uri' and 'local'.
*/
Declare(
SDOM_Exception SDOM_getAttributeNS(
    SablotSituation s, 
    SDOM_Node n, 
    SDOM_char *uri, 
    SDOM_char *local, 
    SDOM_char **pValue);
)

/*
    getAttributeNode
    Returns, in *attr, the attribute named 'name'.
*/
Declare(
SDOM_Exception SDOM_getAttributeNode(
    SablotSituation s,
    SDOM_Node n, 
    const SDOM_char *name, 
    SDOM_Node *attr);
)

/*
    getAttributeNodeNS
    Returns, in *attr, the n's attribute with uri and local.
*/
Declare(
SDOM_Exception SDOM_getAttributeNodeNS(
    SablotSituation s,
    SDOM_Node n, 
    SDOM_char *uri, 
    SDOM_char *local, 
    SDOM_Node *attr);
)

//_JP_ v
/*
    getAttributeNodeIndex
    Returns, in *attr, the index'th attribute, namespaces precede other atts.
*/
Declare(
SDOM_Exception SDOM_getAttributeNodeIndex(
    SablotSituation s, 
    SDOM_Node n, 
    const int index, 
    SDOM_Node *attr);
)

/*
    getAttributeNodeCount
    Returns, in *count, the count of atts, including namespaces in scope.
*/
Declare(
SDOM_Exception SDOM_getAttributeNodeCount(
    SablotSituation s, 
    SDOM_Node n, 
    int *count);
)
//_JP_ ^

/*
    setAttribute
    Assigns the given value to n's attribute named 'name'.
*/
Declare(
SDOM_Exception SDOM_setAttribute(
    SablotSituation s,
    SDOM_Node n, 
    const SDOM_char *name, 
    const SDOM_char *value);
)

/*
    setAttributeNS
    Assigns the given value to n's attribute with 'uri' and qualified name 'qName'.
*/
Declare(
SDOM_Exception SDOM_setAttributeNS(
    SablotSituation s,
    SDOM_Node n, 
    const SDOM_char *uri, 
    const SDOM_char *qName, 
    const SDOM_char *value);
)

/*
    setAttributeNode
    Assigns the given attnode as n's attribute.
    Returns replaced, if was replaced some node with the same nodeName.
*/
Declare(
SDOM_Exception SDOM_setAttributeNode(
    SablotSituation s, 
    SDOM_Node n, 
    SDOM_Node attnode, 
    SDOM_Node *replaced);
)

/*
    setAttributeNodeNS
    Assigns the given attnode to n's attribute with attnode.uri and attnode.localname.
    Returns replaced, if was replaced some node.
*/
Declare(
SDOM_Exception SDOM_setAttributeNodeNS(
    SablotSituation s, 
    SDOM_Node n, 
    SDOM_Node attnode, 
    SDOM_Node *replaced);
)

/*
    removeAttribute
    Removes the given attribute of n. 
*/
Declare(
SDOM_Exception SDOM_removeAttribute(
    SablotSituation s,
    SDOM_Node n, 
    const SDOM_char *name);
)

/*
    removeAttribute
    Removes the given attribute of n. 
*/
Declare(
SDOM_Exception SDOM_removeAttributeNode(
    SablotSituation s, 
    SDOM_Node n, 
    SDOM_Node attnode,
    SDOM_Node *removed);
)
/*
    attributeElement
    returns owner element of attribute specified
*/
Declare(
SDOM_Exception SDOM_getAttributeElement(
    SablotSituation s,
    SDOM_Node attr, 
    SDOM_Node *owner);
)

/*
    getAttributeList
    Returns, in *pAttrList, the list of all attributes of the element n.
    !SPEC: Not in spec.
*/
Declare(
SDOM_Exception SDOM_getAttributeList(
    SablotSituation s, 
    SDOM_Node n, 
    SDOM_NodeList *pAttrList);
)

/**
 **  END Node
 **/


/**
 **  Functions related to Document
 **/


/*
    docToString
    Serializes the document, returning the resulting string in
    *pSerialized, which is a Sablotron-allocated buffer.
    !SPEC: Not in spec.
*/ 
Declare(
SDOM_Exception SDOM_docToString(
    SablotSituation s,
    SDOM_Document d, 
    SDOM_char **pSerialized);
)

Declare(
SDOM_Exception SDOM_nodeToString(
    SablotSituation s,
    SDOM_Document d, 
    SDOM_Node n,
    SDOM_char **pSerialized);
)

/**
 **  END Document functions
 **/


/**
 **  NodeList
 **  An ordered collection of nodes, returned by xql.
 **/


/*
    getNodeListLength
    Returns, in *pLength, the number of nodes in the list l. 
    !SPEC: Not in spec.
*/
Declare(
SDOM_Exception SDOM_getNodeListLength(
    SablotSituation s,
    SDOM_NodeList l, 
    int *pLength);
)

/*
    getNodeListItem
    Returns, in *pItem, the index'th member of the node list l.
    !SPEC: Not in spec.
*/
Declare(
SDOM_Exception SDOM_getNodeListItem(
    SablotSituation s,
    SDOM_NodeList l, 
    int index, 
    SDOM_Node *pItem);
)

/*
    disposeNodeList
    Destroys the node list l. The nodes themselves are NOT disposed. 
    !SPEC: Not in spec.
*/
Declare(
SDOM_Exception SDOM_disposeNodeList(
    SablotSituation s,
    SDOM_NodeList l);
)

/**
 **  END NodeList
 **/


/**
 **  Miscellaneous
 **/

/*
    xql
    Returns, in *pResult, the list of all nodes satisfying the XPath
    query given as a string in 'query'. For the evaluation of the query, the
    current node will be set to currentNode.
    Note that the query is necessarily rather restricted.
    After the contents of *pResult have been retrieved, the list should
    be freed using disposeNodeList.
    !SPEC: Not in spec.
*/ 
Declare(
SDOM_Exception SDOM_xql(
    SablotSituation s,
    const SDOM_char *query, 
    SDOM_Node currentNode, 
    SDOM_NodeList *pResult);
)

Declare(
SDOM_Exception SDOM_xql_ns(
    SablotSituation s,
    const SDOM_char *query, 
    SDOM_Node currentNode,
    char** nsmap,
    SDOM_NodeList *pResult);
)

/**
 **  END Miscellaneous
 **
 **
 **
 **  Exception retrieval
 **/

/*
    getExceptionCode
    returns the code of the pending exception
*/

Declare
(
    int SDOM_getExceptionCode(SablotSituation s);
)

/*
    getExceptionMessage
    returns the message for the pending exception
*/

Declare
(
    char* SDOM_getExceptionMessage(SablotSituation s);
)

/*
    getExceptionDetails
    returns extra information for the pending exception
    - on the code and message of the primary error
    - on the document and file line where the primary error occured
*/

Declare
(
    void SDOM_getExceptionDetails(
        SablotSituation s,
	    int *code,
	    char **message,
	    char **documentURI,
	    int *fileLine);
)

/**
 **    END Exception retrieval
 **
 **
 **    Internal functions
 **/

/*
    setNodeInstanceData
    saves a pointer in a node instance
*/

Declare(
void SDOM_setNodeInstanceData(SDOM_Node n, void *data);
)

/*
    getNodeInstanceData
    retrieves the saved pointer
*/
Declare(
void* SDOM_getNodeInstanceData(SDOM_Node n);
)
/*
    setDisposeNodeCallback
    sets callback to be called on every node disposal
*/

typedef void SDOM_NodeCallback(SDOM_Node n);

Declare
(
    void SDOM_setDisposeCallback(SDOM_NodeCallback *f);
)

Declare
(
    SDOM_NodeCallback* SDOM_getDisposeCallback();
)

/**
 **  FOR IMPLEMENTATION IN PERL
 **  None of these fuctions appear in the spec.
 **/


/* 
    ArrayRef getChildNodes(Node n)
    Returns the array of n's children.
    Implement using getFirstChild and getNextSibling.


    HashRef getAttributes(Node n)
    Returns the hash of n's attributes.
    Implement using getAttributeList, getNodeListLength, 
    getNodeListItem, getNodeName and getNodeValue.


    setAttributes(Node n, HashRef atts)
    Sets n's attributes to atts, keeping the old ones alive but making
    them standalone.
    Implement using getAttributeList, getNodeListLength, removeAttribute
    and setAttribute (not too efficient perhaps).
*/

     /* _TH_ v */
Declare
(
 void SDOM_tmpListDump(SDOM_Document doc, int p);
)

Declare
(
 SDOM_Exception SDOM_compareNodes(
    SablotSituation s, 
    SDOM_Node n1,
    SDOM_Node n2,
    int *res);
)

#endif /* DISABLE_DOM */

#endif /* SDomHIncl */
