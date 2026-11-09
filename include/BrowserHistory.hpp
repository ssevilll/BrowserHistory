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

    //page information stored in the history
struct PageInfo {
    std::uint64_t id{};
    std::string url;
    std::string title;
    std::int64_t visitedAt{}; // seconds since Unix epoch (UTC)
};

// Nodes and both links are created manually
// page node for the doubly linked list
struct PageNode {
    PageInfo page;
    PageNode* prev = nullptr;
    PageNode* next = nullptr;
    explicit PageNode(PageInfo value) : page(std::move(value)) {} //convert PageInfo to PageNode
};

class DoublyLinkedList {
public:
    DoublyLinkedList() = default; //default constructor
    ~DoublyLinkedList(); //destructor, to prevent memory leaks, we need to delete all nodes in the list
    DoublyLinkedList(const DoublyLinkedList&) = delete; //delete copy constructor to prevent copying of the list
    DoublyLinkedList& operator=(const DoublyLinkedList&) = delete; //delete copy constructor and copy assignment operator to prevent copying of the list
    void swap(DoublyLinkedList& other) noexcept; //swap two lists, noexcept means it won't throw exceptions

    PageNode* head() const noexcept { return head_; }
    PageNode* tail() const noexcept { return tail_; }
    std::size_t size() const noexcept { return size_; }
    bool empty() const noexcept { return size_ == 0; }


    void append(PageNode* node); //append a node to the end of the list
    void insertAfter(PageNode* anchor, PageNode* node); //insert a node after the anchor node
    void insertBefore(PageNode* anchor, PageNode* node); //insert a node before the anchor node
    void erase(PageNode* node); //erase a node from the list
    void eraseAfter(PageNode* anchor); //erase the node after the anchor node
    void clear() noexcept; //clear the list, delete all nodes
    PageNode* findId(std::uint64_t id) const noexcept; //find a node by its id, return nullptr if not found
    PageNode* atIndex(std::size_t index) const; //find a node by its index, return nullptr if not found
    std::vector<PageInfo> snapshot() const; //return a vector of PageInfo of all nodes in the list
    void validate() const; //validate the list, check if the list is consistent, throw an exception if not

private:
    PageNode* head_ = nullptr;
    PageNode* tail_ = nullptr;
    std::size_t size_ = 0;

    void requireMember(PageNode* node) const; //check if the node is a member of the list, throw an exception if not
    static void requireDetached(PageNode* node); //check if the node is detached, throw an exception if not
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

    DeletedHistoryStack() = default; //default constructor
    ~DeletedHistoryStack(); //destructor, to prevent memory leaks, we need to delete all nodes in the stack
    DeletedHistoryStack(const DeletedHistoryStack&) = delete; //delete copy constructor to prevent copying of the stack
    DeletedHistoryStack& operator=(const DeletedHistoryStack&) = delete; //delete copy constructor and copy assignment operator to prevent copying of the stack
    void swap(DeletedHistoryStack& other) noexcept; //swap two stacks, noexcept means it won't throw exceptions

    void push(Entry entry); //push an entry to the top of the stack
    std::optional<Entry> pop(); //pop an entry from the top of the stack, return std::nullopt if the stack is empty
    void clear() noexcept; //clear the stack, delete all nodes
    std::size_t size() const noexcept { return count_; } //return the number of entries in the stack



    private:
    //internal node structure for the stack
    struct StackNode {
        Entry value;
        StackNode* below;
        StackNode(Entry entry, StackNode* oldTop) : value(std::move(entry)), below(oldTop) {} //constructor, initialize the node with an entry and a pointer to the node below it
    };
    StackNode* top_ = nullptr;
    std::size_t count_ = 0;
};


class BrowserHistory {
public:
    static constexpr std::size_t DefaultCapacity = 50;
    explicit BrowserHistory(std::string homepage, std::size_t capacity = DefaultCapacity); //constructor, initialize the history with a homepage and a capacity
    BrowserHistory(const BrowserHistory&) = delete; //delete copy constructor to prevent copying of the history
    BrowserHistory& operator=(const BrowserHistory&) = delete; //delete copy constructor and copy assignment operator to prevent copying of the history

    void visit(const std::string& url, const std::string& title = ""); //visit a new page, add it to the history, and set it as the current page
    std::optional<std::string> back(int steps); //go back a number of steps in the history, return the URL of the new current page, or std::nullopt if there is no such page
    std::optional<std::string> forward(int steps); //go forward a number of steps in the history, return the URL of the new current page, or std::nullopt if there is no such page
    std::optional<PageInfo> current_page() const; //return the PageInfo of the current page, or std::nullopt if there is no current page
    std::string historyDisplay() const; //return a string representation of the history, with each page on a new line
    void display_history(std::ostream& out) const; //display the history to an output stream, with each page on a new line
    bool delete_url(const std::string& url); //delete a page from the history by its URL, return true if the page was found and deleted, false otherwise
    bool restore_deleted(); //restore the most recently deleted page to its original position in the history, return true if a page was restored, false otherwise
    void delete_history(); //delete all pages from the history, and clear the deleted stack
    void Delete_histroy() { delete_history(); } // handout's exact spelling
    void export_history(const std::string& path) const; //export the history to a file at the given path
    void import_history(const std::string& path); //import the history from a file at the given path, replacing the current history
    std::size_t size() const noexcept { return pages_.size(); }
    std::size_t capacity() const noexcept { return capacity_; }
    std::size_t deletedCount() const noexcept { return deleted_.size(); }
    std::vector<PageInfo> history() const { return pages_.snapshot(); }
    void validateInvariants() const; //validate the invariants of the history, throw an exception if any invariant is violated

    
private:
    DoublyLinkedList pages_; // main history list
    DeletedHistoryStack deleted_; // stack of deleted entries
    PageNode* current_ = nullptr; // pointer to the current page in the history
    std::uint64_t nextId_ = 1; // next page ID to assign
    std::size_t capacity_ = DefaultCapacity; // maximum number of pages to keep in the history
    void trimOldest(); //trim the oldest pages from the history if the size exceeds the capacity
    void swap(BrowserHistory& other) noexcept; //swap two BrowserHistory objects, noexcept means it won't throw exceptions
};

} // namespace bh
