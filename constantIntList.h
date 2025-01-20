#pragma once
#include <cstdlib>
#include <string.h>

namespace vkengine
{
    /// <summary>
    /// - represents an object as a list of integers that are stored from first_index until first_index + num_fields
    /// 
    /// - a constant index list -> indices dont change on removal of items, the free items
    /// are reused on later inserts, they are linked together as a linked list starting at 
    /// free_element
    /// 
    /// - can optionally use stack from caller and expand into heap if needed
    /// 
    /// </summary>
    class ConstantIndexIntList
    {
    private:
        const int initial_capacity = 128;

        // data pointer, may point to heap
        int* data;

        // Stores how many integer fields each element has.
        int num_fields;

        // Stores the number of elements in the list.
        int num;

        // Stores the capacity of the array.
        int cap;

        // Stores an index to the free element or -1 if the free list is empty.
        int free_element;

        bool on_stack;

    public:
        int size() const { return num; }

        /// <summary>
        /// capacity in number of object == internal array capacity / num_fields 
        /// </summary>
        int capacity() const { return cap / num_fields; }
        bool isAllocated() const { data != nullptr && !on_stack; }

        void init(int numfields, int* stack_ptr = nullptr, int stack_size = 0)
        {
            num = 0;
            cap = stack_ptr == nullptr ? initial_capacity : stack_size;
            num_fields = numfields;
            free_element = -1;
            on_stack = stack_ptr != nullptr;
            data =
                stack_ptr == nullptr
                ?
                (int*)malloc(initial_capacity * sizeof(int))
                :
                stack_ptr;
        }

        void destroy()
        {
            if (isAllocated)
            {
                free(data);
            }
        }

        void clear()
        {
            num = 0;
            free_element = -1;
        }

        inline int get(int n, int field) const
        {
            return data[n * num_fields + field];
        }

        inline void set(int n, int field, int val)
        {
            data[n * num_fields + field] = val;
        }

        int pushBack()
        {
            int new_pos = (num + 1) * num_fields;

            if (new_pos > cap)
            {
                int new_cap = cap * 2;

                if (on_stack)
                {
                    // allocate from heap and copy segment from stack to it
                    int* new_data = (int*)malloc(new_cap * sizeof(int));
                    memcpy(new_data, data, cap * sizeof(int));
                    data = new_data;
                    // not on stack anymore
                    on_stack = false;
                }
                else
                {
                    // reallocate the heap buffer to the new size.
                    int* new_data = (int*)malloc(new_cap * sizeof(int));
                    memcpy(new_data, data, cap * sizeof(int));
                    free(data);
                    data = new_data;
                }
                // Set the old capacity to the new capacity.
                cap = new_cap;
            }

            int old = num;
            num++;
            return old;
        }

        void popBack()
        {
            // Just decrement the list size.
            num = num - 1;
        }

        int insert()
        {
            // If there's a free index in the free list, pop that and use it.
            if (free_element != -1)
            {
                int index = free_element;
                int pos = index * num_fields;

                // Set the free index to the next free index.
                free_element = data[pos];

                // Return the free index.
                return index;
            }

            // Otherwise insert to the back of the array.
            return pushBack();
        }

        void erase(int n)
        {
            // Push the element to the free list.
            int pos = n * num_fields;
            data[pos] = free_element;
            free_element = n;
            // note: we never decrease the size of data but instead leave a linked list of open elements
        }
    }

}