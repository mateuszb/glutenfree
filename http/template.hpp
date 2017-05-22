#pragma once

#define U_CHARSET_IS_UTF8 1 

#include <map>
#include <sstream>
#include <memory>
#include <unicode/unistr.h>
#include <unicode/schriter.h>
#include <unicode/chariter.h>
#include <unicode/ustream.h>
#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOM.hpp>
#include <xercesc/framework/StdOutFormatTarget.hpp>
#include <xercesc/framework/MemBufFormatTarget.hpp>
#include <xercesc/dom/DOM.hpp>
#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMDocumentType.hpp>
#include <xercesc/dom/DOMElement.hpp>
#include <xercesc/dom/DOMImplementation.hpp>
#include <xercesc/dom/DOMImplementationLS.hpp>
#include <xercesc/dom/DOMNodeIterator.hpp>
#include <xercesc/dom/DOMNodeList.hpp>
#include <xercesc/dom/DOMText.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xqilla/xqilla-dom3.hpp>

namespace http
{
namespace templates
{
using map_t = std::map<icu::UnicodeString, icu::UnicodeString>;

std::string render_file(const icu::UnicodeString& in);
std::string slurp(const icu::UnicodeString& path);

void init();
void cleanup();

class HTMLTemplate {
public:
    HTMLTemplate(const icu::UnicodeString& ustr);

    icu::UnicodeString render() const;
    void replace(xercesc::DOMNode* node, const UnicodeString& path, const UnicodeString& subst);
    void replace(const UnicodeString& path, const UnicodeString& subst);

    xercesc::DOMNode* find(const icu::UnicodeString& xpath);
    void remove(const icu::UnicodeString& elem);
    void removeAll(const icu::UnicodeString& elem);
    void insert(xercesc::DOMNode *fragment, const icu::UnicodeString& xpath);
    xercesc::DOMDocumentFragment* extract(const icu::UnicodeString& elem);

    ~HTMLTemplate();
private:
    template<typename T>
    struct XercesDeleter
    {
	void operator()(T* x)
	{
	    x->release();
	}
    };

    template<typename T>
    struct XMLDeleter
    {
	void operator()(T* x)
	{
	    xercesc::XMLString::release(&x);
	}
    };

    xercesc::DOMDocument* document;
    xercesc::DOMImplementation *pimpl;
    xercesc::DOMLSParser* parser;
    xercesc::DOMLSInput* inputData;
    XMLCh* xml;
};
}
}
