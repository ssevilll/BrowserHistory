#include "BrowserHistory.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace bh {
namespace {
constexpr const char* FileHeader = "BROWSER_HISTORY_CPP_V1";
std::int64_t nowUtc() {
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}
bool blank(const std::string& s) {
    return std::all_of(s.begin(), s.end(), [](unsigned char c){ return std::isspace(c) != 0; });
}
void checkSteps(int steps) {
    if (steps < 0) throw std::invalid_argument("Steps cannot be negative");
}
} // namespace

DoublyLinkedList::~DoublyLinkedList() { clear(); }
void DoublyLinkedList::swap(DoublyLinkedList& other) noexcept {
    std::swap(head_, other.head_);
    std::swap(tail_, other.tail_);
    std::swap(size_, other.size_);
}
void DoublyLinkedList::requireDetached(PageNode* node) {
    if (!node || node->prev || node->next)
        throw std::invalid_argument("New page node is null or already linked");
}
void DoublyLinkedList::requireMember(PageNode* node) const {
    if (!node || (node != head_ && !node->prev) || (node != tail_ && !node->next))
        throw std::invalid_argument("Node is not linked into this list");
}
void DoublyLinkedList::append(PageNode* node) {
    requireDetached(node);
    node->prev = tail_;
    if (tail_) tail_->next = node;
    else head_ = node;
    tail_ = node;
    ++size_;
}
void DoublyLinkedList::insertAfter(PageNode* anchor, PageNode* node) {
    if (!anchor) { insertBefore(head_, node); return; }
    requireMember(anchor);
    requireDetached(node);
    node->prev = anchor;
    node->next = anchor->next;
    if (anchor->next) anchor->next->prev = node;
    else tail_ = node;
    anchor->next = node;
    ++size_;
}
void DoublyLinkedList::insertBefore(PageNode* anchor, PageNode* node) {
    if (!anchor) { append(node); return; }
    requireMember(anchor);
    requireDetached(node);
    node->next = anchor;
    node->prev = anchor->prev;
    if (anchor->prev) anchor->prev->next = node;
    else head_ = node;
    anchor->prev = node;
    ++size_;
}
void DoublyLinkedList::erase(PageNode* node) {
    requireMember(node);
    if (node->prev) node->prev->next = node->next;
    else head_ = node->next;
    if (node->next) node->next->prev = node->prev;
    else tail_ = node->prev;
    node->next = node->prev = nullptr;
    delete node;
    --size_;
}
void DoublyLinkedList::eraseAfter(PageNode* anchor) {
    requireMember(anchor);
    while (anchor->next) erase(anchor->next);
}
void DoublyLinkedList::clear() noexcept {
    PageNode* node = head_;
    while (node) {
        PageNode* next = node->next;
        delete node;
        node = next;
    }
    head_ = tail_ = nullptr;
    size_ = 0;
}
PageNode* DoublyLinkedList::findId(std::uint64_t id) const noexcept {
    for (PageNode* n = head_; n; n = n->next)
        if (n->page.id == id) return n;
    return nullptr;
}
PageNode* DoublyLinkedList::atIndex(std::size_t index) const {
    if (index > size_) throw std::out_of_range("Page index out of range");
    PageNode* n = head_;
    for (std::size_t i = 0; i < index; ++i) n = n->next;
    return n;
}
std::vector<PageInfo> DoublyLinkedList::snapshot() const {
    std::vector<PageInfo> result;
    result.reserve(size_);
    for (PageNode* n = head_; n; n = n->next) result.push_back(n->page);
    return result;
}
void DoublyLinkedList::validate() const {
    if ((head_ == nullptr) != (tail_ == nullptr) ||
        (head_ && head_->prev) || (tail_ && tail_->next))
        throw std::logic_error("Bad history endpoints");
    std::size_t count = 0;
    PageNode* prev = nullptr;
    for (PageNode* n = head_; n; n = n->next) {
        if (n->prev != prev || ++count > size_) throw std::logic_error("Broken prev link or forward cycle");
        prev = n;
    }
    if (count != size_ || prev != tail_) throw std::logic_error("Forward traversal mismatch");
    count = 0;
    PageNode* next = nullptr;
    for (PageNode* n = tail_; n; n = n->prev) {
        if (n->next != next || ++count > size_) throw std::logic_error("Broken next link or reverse cycle");
        next = n;
    }
    if (count != size_ || next != head_) throw std::logic_error("Reverse traversal mismatch");
}

DeletedHistoryStack::~DeletedHistoryStack() { clear(); }
void DeletedHistoryStack::swap(DeletedHistoryStack& other) noexcept {
    std::swap(top_, other.top_);
    std::swap(count_, other.count_);
}
void DeletedHistoryStack::push(Entry entry) {
    top_ = new StackNode(std::move(entry), top_);
    ++count_;
}
std::optional<DeletedHistoryStack::Entry> DeletedHistoryStack::pop() {
    if (!top_) return std::nullopt;
    StackNode* old = top_;
    Entry value = std::move(old->value);
    top_ = old->below;
    delete old;
    --count_;
    return value;
}
void DeletedHistoryStack::clear() noexcept {
    while (top_) {
        StackNode* old = top_;
        top_ = top_->below;
        delete old;
    }
    count_ = 0;
}




// BrowserHistory implementation with a doubly linked list for the main history and a custom stack for deleted entries.
BrowserHistory::BrowserHistory(std::string homepage, std::size_t capacity)
    : capacity_(capacity) {
    if (capacity < 1) throw std::invalid_argument("Capacity must be at least 1");
    if (!blank(homepage)) visit(homepage, "Homepage");
}
void BrowserHistory::visit(const std::string& url, const std::string& title) {
    if (blank(url)) throw std::invalid_argument("URL cannot be blank");
    if (current_) pages_.eraseAfter(current_); // discard abandoned forward branch
    PageNode* node = new PageNode(PageInfo{nextId_++, url, title, nowUtc()});
    if (current_) pages_.insertAfter(current_, node);
    else pages_.append(node);
    current_ = node;
    trimOldest();
}
std::optional<std::string> BrowserHistory::back(int steps) {
    checkSteps(steps);
    for (int i = 0; current_ && current_->prev && i < steps; ++i) current_ = current_->prev;
    return current_ ? std::make_optional(current_->page.url) : std::nullopt;
}
std::optional<std::string> BrowserHistory::forward(int steps) {
    checkSteps(steps);
    for (int i = 0; current_ && current_->next && i < steps; ++i) current_ = current_->next;
    return current_ ? std::make_optional(current_->page.url) : std::nullopt;
}
std::optional<PageInfo> BrowserHistory::current_page() const {
    return current_ ? std::make_optional(current_->page) : std::nullopt;
}
std::string BrowserHistory::historyDisplay() const {
    if (pages_.empty()) return "(History is empty)";
    std::ostringstream out;
    for (PageNode* n = pages_.head(); n; n = n->next) {
        if (n != pages_.head()) out << "            <-->\n";
        out << (n == current_ ? "[CURRENT] " : "          ")
            << '#' << n->page.id << "  " << n->page.url;
        if (!n->page.title.empty()) out << "  (" << n->page.title << ')';
        if (n->next) out << '\n';
    }
    return out.str();
}
void BrowserHistory::display_history(std::ostream& out) const { out << historyDisplay() << '\n'; }
bool BrowserHistory::delete_url(const std::string& url) {
    PageNode* target = pages_.tail();
    while (target && target->page.url != url) target = target->prev;
    if (!target) return false;
    std::size_t index = 0;
    for (PageNode* n = pages_.head(); n != target; n = n->next) ++index;
    DeletedHistoryStack::Entry record{target->page,
        target->prev ? target->prev->page.id : 0,
        target->next ? target->next->page.id : 0,
        index};
    deleted_.push(std::move(record));
    if (current_ == target) current_ = target->prev ? target->prev : target->next;
    pages_.erase(target);
    return true;
}
bool BrowserHistory::restore_deleted() {
    auto record = deleted_.pop();
    if (!record) return false;
    if (pages_.size() >= capacity_) {
        PageNode* oldest = pages_.head();
        if (current_ == oldest) current_ = oldest->next;
        pages_.erase(oldest);
    }
    PageNode* prev = record->previousId ? pages_.findId(record->previousId) : nullptr;
    PageNode* next = record->nextId ? pages_.findId(record->nextId) : nullptr;
    PageNode* restored = new PageNode(record->page);
    if (prev) pages_.insertAfter(prev, restored);
    else if (next) pages_.insertBefore(next, restored);
    else {
        // A deleted page may outlive both its former neighbors (e.g. capacity
        // pruning). Surviving newer IDs are normally later visits; put this
        // older entry before the first of them instead of appending it last.
        PageNode* newer = pages_.head();
        while (newer && newer->page.id < restored->page.id) newer = newer->next;
        if (newer) pages_.insertBefore(newer, restored);
        else pages_.insertBefore(pages_.atIndex(std::min(record->originalIndex, pages_.size())), restored);
    }
    if (!current_) current_ = restored;
    return true;
}
void BrowserHistory::delete_history() {
    pages_.clear();
    deleted_.clear();
    current_ = nullptr;
}
void BrowserHistory::trimOldest() {
    while (pages_.size() > capacity_) {
        PageNode* oldest = pages_.head();
        if (current_ == oldest) current_ = oldest->next;
        pages_.erase(oldest);
    }
}
void BrowserHistory::swap(BrowserHistory& other) noexcept {
    pages_.swap(other.pages_);
    deleted_.swap(other.deleted_);
    std::swap(current_, other.current_);
    std::swap(capacity_, other.capacity_);
    std::swap(nextId_, other.nextId_);
}
void BrowserHistory::export_history(const std::string& path) const {
    std::ofstream out(path, std::ios::binary);
    if (!out) throw std::runtime_error("Cannot open export file: " + path);
    out << FileHeader << '\n'
        << "CAPACITY " << capacity_ << '\n'
        << "CURRENT " << (current_ ? static_cast<std::int64_t>(current_->page.id) : -1) << '\n'
        << "PAGES " << pages_.size() << '\n';
    for (const auto& item : pages_.snapshot())
        out << item.id << ' ' << std::quoted(item.url) << ' '
            << std::quoted(item.title) << ' ' << item.visitedAt << '\n';
    if (!out) throw std::runtime_error("Could not write export file");
}
void BrowserHistory::import_history(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("Cannot open import file: " + path);
    std::string header, capacityTag, cursorTag, pagesTag;
    std::size_t newCapacity = 0, count = 0;
    std::int64_t currentId = -1;
    if (!(in >> header >> capacityTag >> newCapacity >> cursorTag >> currentId >> pagesTag >> count)
        || header != FileHeader || capacityTag != "CAPACITY" || cursorTag != "CURRENT"
        || pagesTag != "PAGES" || !newCapacity || count > newCapacity)
        throw std::runtime_error("Malformed browser-history header");
    BrowserHistory loaded("", newCapacity);
    std::unordered_set<std::uint64_t> ids;
    std::uint64_t maxId = 0;
    for (std::size_t i = 0; i < count; ++i) {
        PageInfo page;
        if (!(in >> page.id >> std::quoted(page.url) >> std::quoted(page.title) >> page.visitedAt)
            || !page.id || blank(page.url) || !ids.insert(page.id).second)
            throw std::runtime_error("Malformed, empty, or duplicated page record");
        maxId = std::max(maxId, page.id);
        loaded.pages_.append(new PageNode(std::move(page)));
    }
    std::string unexpected;
    if (in >> unexpected) throw std::runtime_error("Extra content after expected page records");
    if (maxId == std::numeric_limits<std::uint64_t>::max())
        throw std::runtime_error("Page ID space exhausted");
    if (currentId == -1) {
        if (count != 0) throw std::runtime_error("Missing active cursor");
    } else {
        if (currentId <= 0 || !(loaded.current_ = loaded.pages_.findId(static_cast<std::uint64_t>(currentId))))
            throw std::runtime_error("Active cursor not found");
    }
    loaded.nextId_ = maxId + 1;
    loaded.validateInvariants();
    swap(loaded); // commit atomically only after full validation
}
void BrowserHistory::validateInvariants() const {
    pages_.validate();
    if (pages_.size() > capacity_) throw std::logic_error("Capacity exceeded");
    if ((current_ == nullptr) != pages_.empty()) throw std::logic_error("Invalid current page");
    if (current_ && pages_.findId(current_->page.id) != current_)
        throw std::logic_error("Current cursor not in history list");
}
} // namespace bh
