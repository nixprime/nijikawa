using System;
using System.Collections.Generic;

namespace Nijikawa
{
    /// Maximum priority queue implemented by a binary heap.
    class PriorityQueue<T> where T : IComparable<T>
    {
        private List<T> container;

        public int Count
        {
            get { return container.Count; }
        }

        public PriorityQueue()
        {
            container = new List<T>();
        }

        public void Add(T item)
        {
            container.Add(item);
            // Bubble-up
            int child = container.Count - 1;
            while (true)
            {
                if (child == 0)
                {
                    // Reached top of heap
                    break;
                }
                int parent = (child - 1) / 2;
                if (container[parent].CompareTo(container[child]) >= 0)
                {
                    // Heap property satisfied
                    break;
                }
                // Swap parent and child and continue
                T temp = container[parent];
                container[parent] = container[child];
                container[child] = temp;
                child = parent;
            }
        }

        public void Remove()
        {
            container[0] = container[container.Count - 1];
            container.RemoveAt(container.Count - 1);
            // Bubble-down
            int parent = 0;
            while (true)
            {
                int left = 2 * parent + 1;
                int right = 2 * parent + 2;
                int biggest = parent;
                if (left < container.Count &&
                        container[left].CompareTo(container[biggest]) >= 0)
                {
                    biggest = left;
                }
                if (right < container.Count &&
                        container[right].CompareTo(container[biggest]) >= 0)
                {
                    biggest = right;
                }
                if (biggest == parent)
                {
                    // Heap property satisfied
                    break;
                }
                // Swap parent and biggest child and continue
                T temp = container[parent];
                container[parent] = container[biggest];
                container[biggest] = temp;
                parent = biggest;
            }
        }

        public T Top()
        {
            return container[0];
        }
    }
}
