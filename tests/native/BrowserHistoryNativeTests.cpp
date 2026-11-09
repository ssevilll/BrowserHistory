#include "BrowserHistory.hpp"

#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using bh::BrowserHistory;
using Test = std::pair<const char*, std::function<void()>>;

#define CHECK(expr) do { if (!(expr)) throw std::runtime_error(std::string("CHECK failed: ") + #expr + \
    " (line " + std::to_string(__LINE__) + ")"); } while (false)

template<class Exception, class F> void throws(F action) {
    try { action(); } catch (const Exception&) { return; }
    throw std::runtime_error("Expected exception was not thrown");
}
static std::string url(const BrowserHistory& history) {
    auto p = history.current_page(); return p ? p->url : "";
}
static std::filesystem::path tmp(const char* name) {
    return std::filesystem::temp_directory_path() / (std::string("bh_cpp_test_") + name + ".txt");
}
static void expectedUrls(const BrowserHistory& h, std::vector<std::string> values) {
    auto pages = h.history();
    CHECK(pages.size() == values.size());
    for (std::size_t i = 0; i < values.size(); ++i) CHECK(pages[i].url == values[i]);
    h.validateInvariants();
}
int main() {
    std::vector<Test> cases;
    const auto add = [&](const char* label, std::function<void()> fn){ cases.emplace_back(label, std::move(fn)); };

    add("initial homepage", [] { BrowserHistory h("home"); CHECK(h.size()==1); CHECK(url(h)=="home"); h.validateInvariants(); });
    add("default capacity 50", [] { BrowserHistory h("home"); CHECK(h.capacity()==50); });
    add("custom capacity validation", [] { throws<std::invalid_argument>([]{ BrowserHistory h("x",0); }); });
    add("visit updates current", [] { BrowserHistory h("home"); h.visit("a","A"); CHECK(url(h)=="a"); CHECK(h.current_page()->title=="A"); });
    add("visit preserves oldest-to-newest order", [] { BrowserHistory h("h"); h.visit("a"); h.visit("b"); expectedUrls(h,{"h","a","b"}); });
    add("back one", [] { BrowserHistory h("h"); h.visit("a"); CHECK(h.back(1)=="h"); });
    add("forward one", [] { BrowserHistory h("h"); h.visit("a"); h.back(1); CHECK(h.forward(1)=="a"); });
    add("back clamps at oldest", [] { BrowserHistory h("h"); h.visit("a"); CHECK(h.back(100)=="h"); });
    add("forward clamps at newest", [] { BrowserHistory h("h"); h.visit("a"); CHECK(h.forward(999)=="a"); });
    add("zero steps do not navigate", [] { BrowserHistory h("h"); h.visit("a"); CHECK(h.back(0)=="a"); CHECK(h.forward(0)=="a"); });
    add("negative navigation rejected", [] { BrowserHistory h("h"); throws<std::invalid_argument>([&]{h.back(-1);}); throws<std::invalid_argument>([&]{h.forward(-2);}); });
    add("branching drops forward pages", [] { BrowserHistory h("h"); h.visit("a"); h.visit("b"); h.back(1); h.visit("c"); expectedUrls(h,{"h","a","c"}); CHECK(h.forward(5)=="c"); });
    add("branching from oldest", [] { BrowserHistory h("h"); h.visit("a"); h.visit("b"); h.back(99); h.visit("new"); expectedUrls(h,{"h","new"}); });
    add("current page query does not move cursor", [] { BrowserHistory h("h"); h.visit("a"); auto p=h.current_page(); CHECK(p->url=="a"); CHECK(url(h)=="a"); });
    add("display shows current marker", [] { BrowserHistory h("h"); h.visit("a"); h.back(1); CHECK(h.historyDisplay().contains("[CURRENT] #1")); });
    add("delete middle URL", [] { BrowserHistory h("h"); h.visit("a"); h.visit("b"); CHECK(h.delete_url("a")); expectedUrls(h,{"h","b"}); CHECK(h.deletedCount()==1); });
    add("delete oldest URL", [] { BrowserHistory h("h"); h.visit("a"); CHECK(h.delete_url("h")); expectedUrls(h,{"a"}); });
    add("delete newest current URL", [] { BrowserHistory h("h"); h.visit("a"); CHECK(h.delete_url("a")); CHECK(url(h)=="h"); expectedUrls(h,{"h"}); });
    add("delete active head selects next", [] { BrowserHistory h("h"); h.visit("a"); h.back(1); CHECK(h.delete_url("h")); CHECK(url(h)=="a"); });
    add("delete nonexistent URL", [] { BrowserHistory h("h"); CHECK(!h.delete_url("missing")); CHECK(h.deletedCount()==0); });
    add("duplicate URL latest removed", [] { BrowserHistory h("h"); h.visit("a"); h.visit("a"); CHECK(h.delete_url("a")); CHECK(h.size()==2); CHECK(h.current_page()->id==2); });
    add("restore empty stack", [] { BrowserHistory h("h"); CHECK(!h.restore_deleted()); });
    add("restore reconnects middle", [] { BrowserHistory h("h"); h.visit("a"); h.visit("b"); h.delete_url("a"); CHECK(h.restore_deleted()); expectedUrls(h,{"h","a","b"}); });
    add("restore is LIFO", [] { BrowserHistory h("h"); h.visit("a"); h.visit("b"); h.delete_url("a"); h.delete_url("b"); CHECK(h.restore_deleted()); CHECK(h.history().back().url=="b"); CHECK(h.restore_deleted()); expectedUrls(h,{"h","a","b"}); });
    add("restore from fully empty history", [] { BrowserHistory h("h"); h.delete_url("h"); CHECK(!h.current_page()); CHECK(h.restore_deleted()); CHECK(url(h)=="h"); });
    add("capacity evicts oldest", [] { BrowserHistory h("h",3); h.visit("a"); h.visit("b"); h.visit("c"); expectedUrls(h,{"a","b","c"}); });
    add("capacity one", [] { BrowserHistory h("h",1); h.visit("a"); h.visit("b"); expectedUrls(h,{"b"}); CHECK(h.back(10)=="b"); });
    add("restore when capacity full", [] { BrowserHistory h("h",2); h.visit("a"); h.delete_url("a"); h.visit("b"); CHECK(h.restore_deleted()); CHECK(h.size()==2); CHECK(h.history().back().url=="b"); h.validateInvariants(); });
    add("clear empties list and stack", [] { BrowserHistory h("h"); h.visit("a"); h.delete_url("a"); h.delete_history(); CHECK(h.size()==0); CHECK(h.deletedCount()==0); CHECK(!h.current_page()); CHECK(!h.back(10)); CHECK(!h.forward(10)); CHECK(h.historyDisplay()=="(History is empty)"); h.validateInvariants(); });
    add("visit after clear starts new history", [] { BrowserHistory h("h"); h.delete_history(); h.visit("x"); expectedUrls(h,{"x"}); });
    add("reject blank URL", [] { BrowserHistory h("h"); throws<std::invalid_argument>([&]{h.visit("   ");}); expectedUrls(h,{"h"}); });
    add("export import full roundtrip", [] { auto f=tmp("roundtrip"); BrowserHistory h("h",4); h.visit("https://x/?a=1&b=2", "title \"quotes\" / Ü"); h.visit("end"); h.back(1); auto old=h.current_page(); h.export_history(f.string()); BrowserHistory i("different"); i.import_history(f.string()); expectedUrls(i,{"h","https://x/?a=1&b=2","end"}); CHECK(i.current_page()->id==old->id); CHECK(i.current_page()->visitedAt==old->visitedAt); CHECK(i.capacity()==4); CHECK(i.history()[1].title=="title \"quotes\" / Ü"); std::filesystem::remove(f); });
    add("export import empty history", [] { auto f=tmp("empty"); BrowserHistory h("h"); h.delete_history(); h.export_history(f.string()); BrowserHistory i("x"); i.import_history(f.string()); CHECK(i.size()==0); CHECK(!i.current_page()); std::filesystem::remove(f); });
    add("invalid import never changes state", [] { auto f=tmp("malformed"); {std::ofstream s(f);s<<"garbage\n";} BrowserHistory h("h"); h.visit("a"); throws<std::runtime_error>([&]{h.import_history(f.string());}); expectedUrls(h,{"h","a"}); CHECK(url(h)=="a"); std::filesystem::remove(f); });
    add("invalid cursor never changes state", [] { auto f=tmp("cursor"); {std::ofstream s(f);s<<"BROWSER_HISTORY_CPP_V1\nCAPACITY 3\nCURRENT 17\nPAGES 1\n1 \"x\" \"title\" 123\n";} BrowserHistory h("h"); throws<std::runtime_error>([&]{h.import_history(f.string());}); CHECK(url(h)=="h"); std::filesystem::remove(f); });
    add("import clears undo stack", [] { auto f=tmp("undo"); BrowserHistory h("h"); h.visit("a"); h.export_history(f.string()); h.delete_url("a"); CHECK(h.deletedCount()==1); h.import_history(f.string()); CHECK(h.deletedCount()==0); std::filesystem::remove(f); });
    add("deterministic 1500-operation pointer stress", [] {
        BrowserHistory h("home",20); std::mt19937 rng(12345);
        for (int i=0;i<1500;i++) {
            int op=static_cast<int>(rng()%6);
            if (op<2) h.visit("u"+std::to_string(rng()%27));
            else if (op==2) h.back(static_cast<int>(rng()%50));
            else if (op==3) h.forward(static_cast<int>(rng()%50));
            else if (op==4) h.delete_url("u"+std::to_string(rng()%27));
            else h.restore_deleted();
            h.validateInvariants();
        }
        CHECK(h.size()<=20);
    });

    int passed=0, failed=0;
    for (const auto& test : cases) {
        try { test.second(); ++passed; std::cout << "[PASS] " << test.first << '\n'; }
        catch (const std::exception& e) { ++failed; std::cout << "[FAIL] " << test.first << " :: " << e.what() << '\n'; }
    }
    std::cout << "RESULT: " << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
