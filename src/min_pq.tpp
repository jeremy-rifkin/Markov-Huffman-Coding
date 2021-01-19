#include "min_pq.h"
#include <vector>

template<typename W, typename I> void min_pq<W, I>::insert(W weight, I item) {
		queue.push_back({ weight, item });
		swim(queue.size() - 1);
	}

template<typename W, typename I> I min_pq<W, I>::pop_min() {
	entry e = queue[0];
	queue[0] = queue[queue.size() - 1];
	queue.pop_back();
	sink(0);
	return e.item;
}

template<typename W, typename I> bool min_pq<W, I>::empty() {
	return queue.size() != 0;
}

template<typename W, typename I> int min_pq<W, I>::size() {
	return queue.size();
}

template<typename W, typename I> std::vector<typename min_pq<W, I>::entry>& min_pq<W, I>::get_queue() {
	return queue;
}

template<typename W, typename I> void min_pq<W, I>::swim(int i) {
	while(i != 0 && queue[(i - 1) / 2].weight > queue[i].weight) {
		entry tmp = queue[i];
		queue[i] = queue[(i - 1) / 2];
		queue[(i - 1) / 2] = tmp;
		i = (i - 1) / 2;
	}
}

template<typename W, typename I> void min_pq<W, I>::sink(int i) {
	while(true) {
		int l = 2 * i + 1;
		int r = 2 * i + 2;
		int target_i = r < queue.size() && queue[r].weight < queue[l].weight ? r : l;
		if(target_i < queue.size() && queue[target_i].weight < queue[i].weight) {
			entry tmp = queue[i];
			queue[i] = queue[target_i];
			queue[target_i] = tmp;
			i = target_i;
		} else {
			break;
		}
	}
}
