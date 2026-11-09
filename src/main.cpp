#include "BrowserHistory.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

namespace {
std::string prompt(const std::string& message) {
    std::cout << message << std::flush;
    std::string value;
    if (!std::getline(std::cin, value)) throw std::runtime_error("Input ended");
    return value;
}
int steps() {
    const std::string value = prompt("Number of steps: ");
    std::size_t pos = 0;
    try {
        int result = std::stoi(value, &pos);
        if (pos != value.size()) throw std::invalid_argument("trailing characters");
        return result;
    } catch (const std::exception&) {
        throw std::invalid_argument("Enter an integer number of steps");
    }
}
std::string shown(const std::optional<std::string>& url) {
    return url ? *url : "(history is empty)";
}
void menu() {
    std::cout << "\n=== MENU ===\n"
              << "1. Visit a page\n2. Back\n3. Forward\n"
              << "4. Current page and metadata\n5. Display history\n"
              << "6. Delete a URL\n7. Restore last deleted URL\n"
              << "8. Clear all history\n9. Export history to TXT\n"
              << "10. Import history from TXT\n11. Show history statistics\n"
              << "0. Exit\nChoice: " << std::flush;
}
}
int main(int argc, char** argv) {
    const std::string homepage = argc > 1 ? argv[1] : "https://example.com";
    bh::BrowserHistory browser(homepage);
    std::cout << "Browser History Manager (C++23) | starting page: " << homepage << '\n';
    for (;;) {
        menu();
        std::string choice;
        if (!std::getline(std::cin, choice)) return 0;
        try {
            if (choice == "0") { std::cout << "Goodbye!\n"; return 0; }
            else if (choice == "1") {
                const auto url = prompt("URL: ");
                const auto title = prompt("Title (optional): ");
                browser.visit(url, title);
                std::cout << "Visited: " << browser.current_page()->url << '\n';
            } else if (choice == "2") std::cout << "Now at: " << shown(browser.back(steps())) << '\n';
            else if (choice == "3") std::cout << "Now at: " << shown(browser.forward(steps())) << '\n';
            else if (choice == "4") {
                const auto page = browser.current_page();
                if (!page) std::cout << "History is empty\n";
                else std::cout << "Current: #" << page->id << " | URL: " << page->url
                               << " | title: " << page->title << " | visitedAt (UTC epoch): "
                               << page->visitedAt << '\n';
            } else if (choice == "5") browser.display_history(std::cout);
            else if (choice == "6") {
                const auto url = prompt("Exact URL to delete: ");
                std::cout << (browser.delete_url(url) ? "URL deleted and pushed to restore stack"
                                                        : "URL not found") << '\n';
            } else if (choice == "7")
                std::cout << (browser.restore_deleted() ? "Restored latest deleted URL"
                                                      : "Restore stack is empty") << '\n';
            else if (choice == "8") { browser.delete_history(); std::cout << "History and restore stack cleared\n"; }
            else if (choice == "9") {
                auto path = prompt("Export file path (.txt): ");
                browser.export_history(path);
                std::cout << "Exported to " << path << '\n';
            } else if (choice == "10") {
                auto path = prompt("Import file path (.txt): ");
                browser.import_history(path);
                std::cout << "Imported " << browser.size() << " page(s)\n";
            } else if (choice == "11")
                std::cout << "Pages: " << browser.size() << '/' << browser.capacity()
                          << " | Undo stack: " << browser.deletedCount() << '\n';
            else std::cout << "Unknown menu option\n";
            browser.validateInvariants();
        } catch (const std::exception& ex) {
            std::cout << "Error: " << ex.what() << '\n';
        }
    }
}
