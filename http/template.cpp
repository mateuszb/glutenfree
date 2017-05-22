#include <http/template.hpp>
#include <fstream>
#include <regex>
#include <cassert>

#include <xqilla/xqilla-dom3.hpp>
#include <iostream>

using namespace http::templates;
using namespace std;
using namespace xercesc;
using icu::UnicodeString;

string http::templates::slurp(const UnicodeString& fname)
{
    ostringstream ssfname;
    ssfname << fname;
    ifstream in(ssfname.str());
    stringstream sstr;
    sstr << in.rdbuf();
    return std::move(sstr.str());
}

void http::templates::init()
{
    XQillaPlatformUtils::initialize();
}

void http::templates::cleanup()
{
    XQillaPlatformUtils::terminate();
}

static XMLCh* xmlCharFromUnicode(const UnicodeString& str)
{
    ostringstream oss;
    oss << str;
    const auto x = oss.str();

    return XMLString::transcode(x.c_str());
}

string render_file(const UnicodeString& fname)
{
    return slurp(fname);
}

HTMLTemplate::HTMLTemplate(const icu::UnicodeString & ustr)
{
    pimpl = DOMImplementationRegistry::getDOMImplementation(X("XPath2 3.0"));
    parser = pimpl->createLSParser(DOMImplementationLS::MODE_SYNCHRONOUS, 0);

    parser->getDomConfig()->setParameter(XMLUni::fgDOMNamespaces, false);
    parser->getDomConfig()->setParameter(XMLUni::fgXercesSchema, false);
    parser->getDomConfig()->setParameter(XMLUni::fgDOMValidateIfSchema, false);
    parser->getDomConfig()->setParameter(XMLUni::fgDOMValidate, false);
    parser->getDomConfig()->setParameter(XMLUni::fgXercesLoadExternalDTD, false);
    parser->getDomConfig()->setParameter(XMLUni::fgXercesDisableDefaultEntityResolution, true);
    parser->getDomConfig()->setParameter(XMLUni::fgDOMEntities, true);

    // transcode the data to XMLCh
    inputData = pimpl->createLSInput();
    xml = xmlCharFromUnicode(ustr);
    assert(xml != nullptr);

    inputData->setStringData(xml);
    document = parser->parse(inputData);
}

HTMLTemplate::~HTMLTemplate()
{
    document->release();
    inputData->release();
    XMLString::release(&xml);
}

icu::UnicodeString HTMLTemplate::render() const
{
    AutoRelease<DOMLSSerializer> serializer(pimpl->createLSSerializer());

    auto coded = serializer->writeToString(document);
    auto xcoded = XMLString::transcode(coded);
    auto unicode = UnicodeString(xcoded);

    XMLString::release(&coded);
    XMLString::release(&xcoded);

    return unicode;
}

void HTMLTemplate::replace(
    const UnicodeString& path,
    const UnicodeString& subst)
{
    replace(document, path, subst);
}

void HTMLTemplate::replace(
    DOMNode *node,
    const UnicodeString& path,
    const UnicodeString& subst)
{
    auto xpath = xmlCharFromUnicode(path);
    AutoRelease<DOMXPathExpression> expression(document->createExpression(xpath, 0));
    AutoRelease<DOMXPathResult> result(
        expression->evaluate(node, DOMXPathResult::ITERATOR_RESULT_TYPE, 0));

    auto val = xmlCharFromUnicode(subst);
    
    try {
        while (result->iterateNext()) {
            auto node = result->getNodeValue();
            node->setNodeValue(val);
        }
    }
    catch (const DOMXPathException& e)
    {
        auto code = e.code;
        auto errorMessage = XMLString::transcode(e.getMessage());
        cerr << errorMessage << endl;
        XMLString::release(&errorMessage);
    }

    XMLString::release(&val);
    XMLString::release(&xpath);
}

void HTMLTemplate::remove(const UnicodeString& xpath)
{
    auto fragment = extract(xpath);
    if (fragment != nullptr) {
        fragment->release();
    }
}

void HTMLTemplate::removeAll(const UnicodeString& xpath)
{
    auto elemname = xmlCharFromUnicode(xpath);

    auto findNext = [&]() {
        AutoRelease<DOMXPathExpression> expression(document->createExpression(elemname, 0));
        AutoRelease<DOMXPathResult> result(
            expression->evaluate(document, DOMXPathResult::ITERATOR_RESULT_TYPE, 0));
        try {
            while (result->iterateNext()) {
                auto node = result->getNodeValue();
                auto range = document->createRange();
                range->selectNode(node);
                auto fragment = range->extractContents();
                //fragment->release();
                range->release();
                return true;
            }
        }
        catch (DOMException& e) {
            return false;
        }

        return false;
    };

    
    while (findNext());

    XMLString::release(&elemname);
}

DOMNode* HTMLTemplate::find(const UnicodeString& xpath)
{
    try {
        auto elemname = xmlCharFromUnicode(xpath);
        AutoRelease<DOMXPathExpression> expression(document->createExpression(elemname, 0));
        AutoRelease<DOMXPathResult> result(
            expression->evaluate(document, DOMXPathResult::ITERATOR_RESULT_TYPE, 0));

        if (result->iterateNext()) {
            auto node = result->getNodeValue();
            return node;
        }

        XMLString::release(&elemname);
    }
    catch (DOMXPathException& e)
    {
        char* msg = XMLString::transcode(e.msg);
        std::cout << "Exception message is: " << msg << std::endl;
        XMLString::release(&msg);
    }

    return nullptr;

}
DOMDocumentFragment* HTMLTemplate::extract(const UnicodeString& xpath)
{
    try {
        auto elemname = xmlCharFromUnicode(xpath);
        AutoRelease<DOMXPathExpression> expression(document->createExpression(elemname, 0));
        AutoRelease<DOMXPathResult> result(
            expression->evaluate(document, DOMXPathResult::ITERATOR_RESULT_TYPE, 0));

        if (result->iterateNext()) {
            auto node = result->getNodeValue();
            auto range = document->createRange();
            range->selectNode(node);
            auto fragment = range->extractContents();

            range->release();
            return fragment;
        }

        XMLString::release(&elemname);
    }
    catch (DOMXPathException& e)
    {
        char* msg = XMLString::transcode(e.msg);
        std::cout << "Exception message is: " << msg << std::endl;
        XMLString::release(&msg);
    }

    return nullptr;
}


void HTMLTemplate::insert(DOMNode* fragment, const icu::UnicodeString& path)
{
    if (fragment == nullptr) {
        return;
    }
    auto xpath = xmlCharFromUnicode(path);

    try {
        AutoRelease<DOMXPathExpression> expression(document->createExpression(xpath, 0));
        AutoRelease<DOMXPathResult> result(
            expression->evaluate(document, DOMXPathResult::ITERATOR_RESULT_TYPE, 0));

        if (result->iterateNext()) {
            auto node = result->getNodeValue();
            node->appendChild(fragment);
        }
    }
    catch (const DOMException& e) {
        auto msg = XMLString::transcode(e.getMessage());
        cerr << "DOM exception:" << string(msg) << endl;
        XMLString::release(&msg);
    }
    XMLString::release(&xpath);
}
