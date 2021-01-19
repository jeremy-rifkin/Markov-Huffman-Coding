#ifndef MIN_PQ_H
#define MIN_PQ_H

#include <vector>

template<typename W, typename I> class min_pq {
	struct entry {
		W weight;
		I item;
	};
	std::vector<entry> queue;
public:
	min_pq() = default;
	void insert(W weight, I item);
	I pop_min();
	bool empty();
	int size();
	std::vector<entry>& get_queue();
private:
	void swim(int i);
	void sink(int i);
};

#include "min_pq.tpp"

#endif
