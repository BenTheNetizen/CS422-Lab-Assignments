#pragma once

#include <cstddef>
#include <iostream>

class ReferenceCounter
{
    int count; // Reference count
public:
    void increment() {
        count++;
    }
    int decrement() {
        return --count;
    }
    int getCount() const {
        return count;
    }
};

template <typename T>
class SmartPointer
{
    // private instance variables for dumb pointer and ReferenceCounter
    T* ptr;
    ReferenceCounter* refCounter;

public:
    SmartPointer(T *pValue) {
        // initialize dumb pointer
        // set up and increment reference counter
        ptr = pValue;
        refCounter = new ReferenceCounter();
        refCounter->increment();
        // print ref count
        std::cout << "constructor ref count: " << refCounter->getCount() << std::endl;
    }

    // Copy constructor
    SmartPointer(const SmartPointer<T> &sp) {
        // Copy the data and reference pointer
        // increment the reference count
        ptr = sp.ptr;
        refCounter = sp.refCounter;
        refCounter->increment();
        std::cout << "copy constructor ref count: " << refCounter->getCount() << std::endl;
    }

    // Destructor
    ~SmartPointer() {
        // Decrement the reference count
        // if reference become zero delete the data
        std::cout << "destructor ref count: " << refCounter->getCount() << std::endl;
        if (refCounter->decrement() == 0) {
            delete ptr;
            delete refCounter;
            // should we also free the underlying data?
        }
    }

    T& operator*() {
        // delegate
        return *ptr;
    }

    T* operator->() {
        // delegate
        return ptr;
    }

    // Assignment operator
    SmartPointer<T> &operator=(const SmartPointer<T> &sp)
    {
        std::cout << "assignment old ref count: " << refCounter->getCount() << std::endl;
        // Deal with old SmartPointer that is being overwritten
        if (refCounter->decrement() == 0) {
            delete ptr;
            delete refCounter;
        }
        // Copy sp into this (similar to copy constructor)
        ptr = sp.ptr;
        refCounter = sp.refCounter;
        refCounter->increment();
        std::cout << "assignment new ref count: " << refCounter->getCount() << std::endl;
        // return this
        return *this;
    }

    // Check equal to nullptr
    bool operator==(std::nullptr_t rhs) const
    {
        return ptr == rhs;   
    }
};