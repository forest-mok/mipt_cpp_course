#include "event_list.h"

namespace nano_edr {

EventList::~EventList() {
    while (head != nullptr) {
        EventNode* next = head->next;
        delete head;
        head = next;
    }
}

void ListPushBack(EventList* list, const Event* event) {

    if (list->capacity != 0 && list->size == list->capacity) {
        ListPopFront(list);
    }

    EventNode* node = new EventNode{*event, nullptr};

    if (list->tail == nullptr) {

        list->head = node;
        list->tail = node;
    } else {

        list->tail->next = node;
        list->tail = node;
    }

    ++list->size;
}

void ListPopFront(EventList* list) {

    if (list->head == nullptr) {
        return;
    }

    EventNode* old_head = list->head;
    list->head = old_head->next;  
    delete old_head;
    --list->size;


    if (list->head == nullptr) {
        list->tail = nullptr;
    }
}

void ListClear(EventList* list) {

    while (list->head != nullptr) {
        ListPopFront(list);
    }
}

}  