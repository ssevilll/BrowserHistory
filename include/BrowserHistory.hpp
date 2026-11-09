#pragma once

#if __cplusplus < 202100L
#error "BrowserHistory requires compilation in C++23 mode (use -std=c++23 or CMake)."
#endif

#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <optional>
#include <string>
#include <vector>

namespace bh {

struct PageInfo {
    std::uint64_t id{};
    std::string url;
    std::string title;
    std::int64_t visitedAt{}; // seconds since Unix epoch (UTC)
};

// Nodes and both links are created manually: no standard linked-list container.
struct PageNode {
    PageInfo page;
    PageNode* prev = nullptr;
    PageNode* next = nullptr;
    explicit PageNode(PageInfo value) : page(std::move(value)) {}
};

class DoublyLinkedList {
public:
    DoublyLinkedList() = default;
    ~DoublyLinkedList();
    DoublyLinkedList(const DoublyLinkedList&) = delete;
    DoublyLinkedList& operator=(const DoublyLinkedList&) = delete;
    void swap(DoublyLinkedList& other) noexcept;

    PageNode* head() const noexcept { return head_; }
    PageNode* tail() const noexcept { return tail_; }
    std::size_t size() const noexcept { return size_; }
    bool empty() const noexcept { return size_ == 0; }
    void append(PageNode* node);
    void insertAfter(PageNode* anchor, PageNode* node);
    void insertBefore(PageNode* anchor, PageNode* node);
    void erase(PageNode* node);
    void eraseAfter(PageNode* anchor);
    void clear() noexcept;
    PageNode* findId(std::uint64_t id) const noexcept;
    PageNode* atIndex(std::size_t index) const;
    std::vector<PageInfo> snapshot() const;
    void validate() const;

private:
    PageNode* head_ = nullptr;
    PageNode* tail_ = nullptr;
    std::size_t size_ = 0;
    void requireMember(PageNode* node) const;
    static void requireDetached(PageNode* node);
};

// A separate custom singly linked LIFO stack for deleted URLs.
class DeletedHistoryStack {
public:
    struct Entry {
        PageInfo page;
        std::uint64_t previousId = 0; // 0: neighbor absent
        std::uint64_t nextId = 0;
        std::size_t originalIndex = 0;
    };
    DeletedHistoryStack() = default;
    ~DeletedHistoryStack();
    DeletedHistoryStack(const DeletedHistoryStack&) = delete;
    DeletedHistoryStack& operator=(const DeletedHistoryStack&) = delete;
    void swap(DeletedHistoryStack& other) noexcept;
    void push(Entry entry);
    std::optional<Entry> pop();
    void clear() noexcept;
    std::size_t size() const noexcept { return count_; }
private:
    struct StackNode {
        Entry value;
        StackNode* below;
        StackNode(Entry entry, StackNode* oldTop) : value(std::move(entry)), below(oldTop) {}
    };
    StackNode* top_ = nullptr;
    std::size_t count_ = 0;
};

class BrowserHistory {
public:
    static constexpr std::size_t DefaultCapacity = 50;
    explicit BrowserHistory(std::string homepage, std::size_t capacity = DefaultCapacity);
    BrowserHistory(const BrowserHistory&) = delete;
    BrowserHistory& operator=(const BrowserHistory&) = delete;

    void visit(const std::string& url, const std::string& title = "");
    std::optional<std::string> back(int steps);
    std::optional<std::string> forward(int steps);
    std::optional<PageInfo> current_page() const;
    std::string historyDisplay() const;
    void display_history(std::ostream& out) const;
    bool delete_url(const std::string& url);
    bool restore_deleted();
    void delete_history();
    void Delete_histroy() { delete_history(); } // handout's exact spelling
    void export_history(const std::string& path) const;
    void import_history(const std::string& path);
    std::size_t size() const noexcept { return pages_.size(); }
    std::size_t capacity() const noexcept { return capacity_; }
    std::size_t deletedCount() const noexcept { return deleted_.size(); }
    std::vector<PageInfo> history() const { return pages_.snapshot(); }
    void validateInvariants() const;

private:
    DoublyLinkedList pages_;
    DeletedHistoryStack deleted_;
    PageNode* current_ = nullptr;
    std::uint64_t nextId_ = 1;
    std::size_t capacity_ = DefaultCapacity;
    void trimOldest();
    void swap(BrowserHistory& other) noexcept;
};

} // namespace bh
