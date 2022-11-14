#include "SmartPointer.hpp"
#include "LinkedList.hpp"

int main() {
    SmartPointer<Node<int>> node1 = new Node<int>(1);
    SmartPointer<Node<int>> node2 = new Node<int>(2);
    SmartPointer<Node<int>> node3 = new Node<int>(3);

    node1->setNext(node2);
    node2->setNext(node3);
    node3->setNext(node1);

    /*
        Memory leak is created by the cycle because the SmartPointer destructor is only called when the pointer goes out of scope. 
        In this program, there are two SmartPointers for nodes 'node1', 'node2', and 'node3'. For each node, there's a SmartPointer when 
        the node was instantiated and a SmartPointer when the node was added to the linked list. The SmartPointer destructor is only called 
        when the pointer goes out of scope, so the destructor is only called for the three SmartPointers that pointed to the instantiated nodes.
        The destructor is not called for the node's 'next' pointers that mutually point to each other, because these nodes are allocated on the heap,
        so the nodes are keeping each other alive in their circular reference.
    */

}